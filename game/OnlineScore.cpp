#include "OnlineScore.hpp"

#include "Trace.hpp"
#include "Tea.hpp"
#include "Tokenizer.hpp"

#include "hashMap.hpp"
#include "HashString.hpp"

#include <iomanip>
#include <string>
#include <list>
#include <sstream>
using namespace std;

#if 0 //def __APPLE__
#import <Foundation/Foundation.h>

@interface OnlineScoreTransfer : NSObject
{
    NSURL  *_url;
    NSString *_boardName;
}
@end

@implementation OnlineScoreTransfer
-(id)initWithURL:(NSURL*)url
{
    if( self = [super init])
    {
        _url = url;
        [_url retain];
        _boardName = nil;
    }
    return self;
}

-(id)initWithURL:(NSURL*)url andBoardName:(NSString *)boardName
{
    if( self = [super init])
    {
        _url = url;
        [_url retain];
        _boardName = boardName;
        [_boardName retain];
    }
    return self;
}

-(void)dealloc
{
    LOG_INFO << "OnlineScoreTransfer::dealloc\n";
    [_url release];
    [_boardName release];
    [super dealloc];
}

-(NSData *)sendRequest
{
    NSError *error = nil;
    NSURLResponse *response = nil;
    NSURLRequest *request = [NSURLRequest requestWithURL:_url
                                             cachePolicy:NSURLRequestReloadIgnoringCacheData
                                         timeoutInterval:5];
    NSData *data = [NSURLConnection sendSynchronousRequest:request
                                         returningResponse:&response
                                                     error:&error];
    return data;
}


-(void)sendScore
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSData *data = [self sendRequest];
    if( data)
    {
        NSString *result = [NSString stringWithCString:(const char *)[data bytes] length:[data length]];
        LOG_INFO << "OnlineScoreTransfer::sendScore " << [result UTF8String] << "\n";
    }
    else
    {
        LOG_INFO << "OnlineScoreTransfer::sendScore unable to get data\n";
    }

    [pool release];
}

-(void)getScores
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];

    NSData *data = [self sendRequest];
    if( data)
    {
        NSString *result = [NSString stringWithCString:(const char *)[data bytes] length:[data length]];

        std::string topN = [result UTF8String];
        LOG_INFO << "OnlineScoreTransfer::getScores " << [_boardName UTF8String] << "\n" << topN << "\n";

        if( topN != "Error")
        {
            OnlineScore::AddScoreBoardData( [_boardName UTF8String], topN);
        }
    }
    else
    {
        LOG_INFO << "OnlineScoreTransfer::getScores unable to get data\n";
    }

    [pool release];
}
@end


NSLock  *theLock = [NSLock new];
void GetLock()
{
    [theLock lock];
}

void ReleaseLock()
{
    [theLock unlock];
}
void SendScore( const std::string &url )
{
    NSURL *nsURL = [NSURL URLWithString:[NSString stringWithCString:url.c_str()
                                                           encoding:NSUTF8StringEncoding]];
    OnlineScoreTransfer *transfer = [[OnlineScoreTransfer alloc] initWithURL:nsURL];
    [transfer performSelectorInBackground:@selector(sendScore) withObject:nil];
    [transfer release];
}

void GetScores( const std::string &url, const std::string &boardName )
{
    NSURL *nsURL = [NSURL URLWithString:[NSString stringWithCString:url.c_str()
                                                           encoding:NSUTF8StringEncoding]];
    NSString *nsBoardName = [NSString stringWithCString:boardName.c_str()
                                               encoding:NSUTF8StringEncoding];
    OnlineScoreTransfer *transfer = [[OnlineScoreTransfer alloc] initWithURL:nsURL andBoardName:nsBoardName];
    [transfer performSelectorInBackground:@selector(getScores) withObject:nil];
    [transfer release];
}
#else
#include "SDL_thread.h"
#if 0
#include <curl/curl.h>
#endif

SDL_mutex *sdlLock = 0;

struct ScoreRequest
{
    ScoreRequest( const string &u, const string &b ):
	url(u),
	boardName(b)
    {
    }

    string url;
    string boardName;
    string result;
};
static list<ScoreRequest> scoreRequests;

void GetLock()
{
    if( !sdlLock)
    {
	sdlLock = SDL_CreateMutex();
    }
    SDL_LockMutex(sdlLock);
}

void ReleaseLock()
{
    if( sdlLock)
    {
        SDL_UnlockMutex(sdlLock);
    }
}

int ScoreRequestThread(void *data);

void SendScore( const string &url )
{
    GetLock();
    ScoreRequest scoreRequest(url, "");
    scoreRequests.push_back( scoreRequest );
    ReleaseLock();

    SDL_Thread *thread = SDL_CreateThread(ScoreRequestThread, "send-score", 0);
}

void GetScores( const string &url, const string &boardName )
{
    GetLock();
    ScoreRequest scoreRequest(url, boardName);
    scoreRequests.push_back( scoreRequest );
    ReleaseLock();

    SDL_Thread *thread = SDL_CreateThread(ScoreRequestThread, "get-scores", 0);
}

size_t receiveData(void *buffer, size_t size, size_t nmemb, void *data)
{
    char *tmpBuf = new char[size * nmemb +1];
    memcpy( tmpBuf, buffer, size * nmemb);
    tmpBuf[size * nmemb] = '\0';

    ScoreRequest *scoreRequest = (ScoreRequest*)data;
    scoreRequest->result += tmpBuf;
//    LOG_INFO << tmpBuf << endl;

    delete [] tmpBuf;

    return size*nmemb;
}


// #include "OSName.hpp"
int ScoreRequestThread(void *data)
{
    GetLock();
    ScoreRequest scoreRequest = scoreRequests.front();
    scoreRequests.pop_front();
    ReleaseLock();

#if 0
    CURL *handle = curl_easy_init();
    if( handle)
    {
	stringstream useragent;
	useragent << PACKAGE << " " << VERSION << " " << OSNAME() << " (" << __DATE__ << " " << __TIME__ << ")" << ends;
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, ::receiveData);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &scoreRequest);
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 30);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, useragent.str().c_str());
	curl_easy_setopt(handle, CURLOPT_URL, scoreRequest.url.c_str());
	//      curl_easy_setopt(handle, CURLOPT_REFERER, "None");

	LOG_INFO << "Score Request: " << scoreRequest.url << endl;

	CURLcode success = curl_easy_perform(handle);
	if( success == CURLE_OK)
	{
	    LOG_INFO << "Score Request OK\n";
	    if( scoreRequest.boardName == "" )
	    {
		//send score
	    }
	    else
	    {
		//get score
		LOG_INFO << "Score for " << scoreRequest.boardName << ": " << scoreRequest.result << "\n";

		if( scoreRequest.result != "Error")
		{
		    OnlineScore::AddScoreBoardData( scoreRequest.boardName, scoreRequest.result);
		}
	    }
	}
	else
	{
	    LOG_INFO << "Score Request Failed\n";
	}

	curl_easy_cleanup(handle);
    }
#endif
    return 0;
}

#endif

static hash_map< const std::string, std::string, hash<const std::string>, equal_to<const std::string> > pendindScoreBoardData;

bool OnlineScore::GetNextScoreBoardData( std::string &boardName, std::string &scoreData)
{
    bool hasNext = false;
    GetLock();
    if( pendindScoreBoardData.begin() != pendindScoreBoardData.end())
    {
        boardName = pendindScoreBoardData.begin()->first;
        scoreData = pendindScoreBoardData.begin()->second;

        pendindScoreBoardData.erase( pendindScoreBoardData.begin());

        hasNext = true;
    }
    ReleaseLock();
    return hasNext;
}

void OnlineScore::AddScoreBoardData( const std::string &boardName, const std::string &data)
{
    GetLock();
    pendindScoreBoardData[boardName] = data;
    ReleaseLock();
}

static inline
std::string stringToHex( const std::string &data)
{
    std::stringstream ss;

    for( size_t i=0; i<data.length(); i++)
    {
        unsigned char val = (unsigned char)data[i];
        ss << std::setw(2) << std::setfill('0') << hex;
        ss << (unsigned int)val;
    }
    return ss.str();
}

static inline
std::string hexToString( const std::string &strRep)
{
    std::stringstream ss;
    std::string result;

    for( size_t i=0; i<strRep.length(); i+=2)
    {
        ss.clear();
        ss << hex << strRep.substr(i, 2);
        unsigned int c;
        ss >> c;
        result += (unsigned char)c;
    }
    return result;
}

//TODO: add connectivity / reachability check
#ifdef DUMMYSCORES
const string onlineScoreURL = "";
#else
const string onlineScoreURL = "";
#endif

void OnlineScore::SendScore( int score, const std::string &scoreMsg )
{
    string message = scoreMsg;
    int pad = message.size() % 4;
    if( pad != 0)
    {
        message += string( 4-pad, ' ');
    }

    string cipher = Tea::encode( message);

    string hash = stringToHex(cipher);
    stringstream url;

    url << onlineScoreURL
        << "?Score=" << score
        << "&Hash=" << hash;

    ::SendScore( url.str() );
}

void OnlineScore::RequestTopScores( const std::string &boardName, const std::string &scoreMsg)
{
    string message = scoreMsg;
    int pad = message.size() % 4;
    if( pad != 0)
    {
        message += string( 4-pad, ' ');
    }

    string cipher = Tea::encode( message);

    string hash = stringToHex(cipher);
    stringstream url;

    url << onlineScoreURL
        << "?Board=" << boardName
        << "&Hash=" << hash;

    LOG_INFO << url.str() << "\n";

    ::GetScores( url.str(), boardName );
}

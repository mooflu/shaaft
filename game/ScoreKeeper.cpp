// Description:
//   Score keeper.
//
// Copyright (C) 2004-2009 Frank Becker
//
#include "ScoreKeeper.hpp"

#include "Trace.hpp"
#include "Config.hpp"
#include "Value.hpp"
#include "Tokenizer.hpp"
#include "zStream.hpp"
#include <RandomKnuth.hpp>

#include "GLBitmapFont.hpp"
#include "FontManager.hpp"

#include "GLBitmapCollection.hpp"
#include "BitmapManager.hpp"

#include "Constants.hpp"
#include "ParticleInfo.hpp"
#include "ParticleGroup.hpp"
#include "ParticleGroupManager.hpp"
#include "VideoBase.hpp"

#include "Tokenizer.hpp"
#include "OnlineScore.hpp"
#include "GameState.hpp"
#include "FindHash.hpp"
#include "StringUtils.hpp"

#ifdef min
#undef min
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <algorithm>
using namespace std;

const int LEADERBOARD_SIZE = 11;  //top-10 plus current score

static RandomKnuth _random;

void ScoreKeeper::mergeOnlineScores() {
    string boardName;
    string scoreData;
    while (OnlineScore::GetNextScoreBoardData(boardName, scoreData)) {
        //LOG_INFO << "Merging online score data for " << boardName << "\n";

        Tokenizer t(scoreData, false, "\n\r");
        string line;
        LeaderBoard onlineBoard;
        while ((line = t.next()) != "") {
            std::vector<std::string> csv = StringUtils(line).csvToVector();
            if (csv.size() == 5) {
                ScoreData sd;
                sd.name = csv[0];
                stringstream(csv[1]) >> sd.score;
                stringstream(csv[2]) >> sd.cubes;
                stringstream(csv[3]) >> sd.secondsPlayed;
                stringstream(csv[4]) >> sd.time;
                sd.online = true;

                onlineBoard.push_back(sd);
            }
        }

        if (onlineBoard.size() == 0) {
            return;
        }

        if (_leaderBoardName == boardName) {
            LOG_INFO << "Updating actice leaderboard\n";

            ScoreData currentScore = _leaderBoard[_currentIndex];

            //merge onlineBoard into _leaderBoard
            mergeLeaderBoard(_leaderBoard, onlineBoard);

            sort(_leaderBoard.begin(), _leaderBoard.end());
            _leaderBoard.resize(LEADERBOARD_SIZE);

            _currentIndex = LEADERBOARD_SIZE;
            for (size_t i = 0; i < _leaderBoard.size(); i++) {
                if (currentScore.score == _leaderBoard[i].score && currentScore.cubes == _leaderBoard[i].cubes &&
                    currentScore.secondsPlayed == _leaderBoard[i].secondsPlayed &&
                    currentScore.time == _leaderBoard[i].time) {
                    _currentIndex = (unsigned int)i;
                    break;
                }
            }

            if (_currentIndex == LEADERBOARD_SIZE) {
                _currentIndex = LEADERBOARD_SIZE - 1;
                _leaderBoard[LEADERBOARD_SIZE - 1] = currentScore;
                sortLeaderBoard();
            }

            if (boardName == _currentScoreboard->name) {
                _currentScoreboardAsLeaderBoard = _leaderBoard;
            }
        } else {
            LOG_INFO << "Updating dormant leaderboard\n";

            ScoreBoards::iterator i;
            for (i = _scoreBoards.begin(); i < _scoreBoards.end(); i++) {
                if (i->name == boardName) {
                    break;
                }
            }

            if (i != _scoreBoards.end()) {
                LeaderBoard lb;
                stringToLeaderBoard(i->data, lb);

                mergeLeaderBoard(lb, onlineBoard);

                sort(lb.begin(), lb.end());
                lb.resize(LEADERBOARD_SIZE);

                i->data = leaderBoardToString(lb, boardName);
            }

            if (boardName == _currentScoreboard->name) {
                stringToLeaderBoard(_currentScoreboard->data, _currentScoreboardAsLeaderBoard);
            }
        }
    }
}

void ScoreKeeper::mergeLeaderBoard(LeaderBoard& leaderBoard, const LeaderBoard& onlineBoard) {
    LeaderBoard::const_iterator oi;
    for (oi = onlineBoard.begin(); oi != onlineBoard.end(); oi++) {
        const ScoreData& sd = *oi;

        bool isDuplicate = false;
        LeaderBoard::iterator i;
        for (i = leaderBoard.begin(); i != leaderBoard.end(); i++) {
            if (sd.score == i->score && sd.cubes == i->cubes && sd.secondsPlayed == i->secondsPlayed &&
                sd.time == i->time) {
                isDuplicate = true;
                i->online = true;
            }
        }
        if (!isDuplicate) {
            leaderBoard.push_back(sd);
        }
    }
}

void ScoreKeeper::sendOnlineUpdateRequest(const std::string& boardName) {
    bool onlineScores = false;
    ConfigS::instance()->getBoolean("onlineScores", onlineScores);

    if (onlineScores) {
        time_t now = time(0);

        bool okToSend = true;
        if (_lastOnlineRequestTime.find(boardName) != _lastOnlineRequestTime.end()) {
            okToSend = (now - _lastOnlineRequestTime[boardName]) > 10;
        }

        if (okToSend) {
            stringstream topNRequest;
            topNRequest << "Shaaft," << boardName << ",10," << GameState::deviceId;
            OnlineScore::RequestTopScores(boardName, topNRequest.str());

            _lastOnlineRequestTime[boardName] = now;
        } else {
            LOG_INFO << "Recently sent request for top scores. Ignoring this one\n";
        }
    }
}

ScoreKeeper::ScoreKeeper(void) :
    _currentIndex(LEADERBOARD_SIZE - 1),
    _infoIndex(0),
    _leaderBoard(LEADERBOARD_SIZE),
    _practiceMode(true) {
    XTRACE();
    resetLeaderBoard(_leaderBoard);
    _currentScoreboard = _scoreBoards.end();
}

ScoreKeeper::~ScoreKeeper(void) {
    XTRACE();
    _scoreBoards.clear();
}

void ScoreKeeper::resetCurrentScore(void) {
    _currentIndex = LEADERBOARD_SIZE - 1;
    _leaderBoard[_currentIndex].score = 0;
    _leaderBoard[_currentIndex].name = "Anonymous";
    _leaderBoard[_currentIndex].time = 0;
    _leaderBoard[_currentIndex].cubes = 0;
    _leaderBoard[_currentIndex].secondsPlayed = 0;
    _leaderBoard[_currentIndex].sent = false;
    _leaderBoard[_currentIndex].online = false;
}

int ScoreKeeper::addToCurrentScore(int score, int cubes, int secs) {
    XTRACE();
    float multiplier = 1.0f;

    int newValue = (int)((float)score * multiplier);

    bool showScoreUpdates = true;
    if (!ConfigS::instance()->getBoolean("showScoreUpdates", showScoreUpdates)) {
        Value* v = new Value(showScoreUpdates);
        ConfigS::instance()->updateKeyword("showScoreUpdates", v);
    }

    if (showScoreUpdates && (newValue > 0)) {
        static ParticleGroup* effects = ParticleGroupManagerS::instance()->getParticleGroup(EFFECTS_GROUP1);

        VideoBase& video = *VideoBaseS::instance();
        int blockView = video.getHeight();
        int shaftOffset = (video.getWidth() - blockView * 4 / 3) / 2;

        ParticleInfo pi;
        pi.position.x = 375 + shaftOffset / 2.0f;
        pi.position.y = 375;
        pi.position.z = 0;
        char buf[10];
        sprintf(buf, "%d", newValue);
        pi.text = buf;

        if (cubes) {
            pi.color.x = 1.0f;
            pi.color.y = 1.0f;
            pi.color.z = 1.0f;
        } else {
            pi.color.x = 1.0f;
            pi.color.y = 0.0f;
            pi.color.z = 0.0f;
        }

        effects->newParticle("ScoreHighlight", pi);
    }

    if (!_practiceMode) {
        _leaderBoard[_currentIndex].score += newValue;
        _leaderBoard[_currentIndex].cubes += cubes;
        time(&_leaderBoard[_currentIndex].time);
        if (secs != 0) {
            _leaderBoard[_currentIndex].secondsPlayed = secs;
        }

        //LOG_INFO << "addToCurrentScore " << cubes << " " << secs << endl;

        sortLeaderBoard();
    }
    //return the real value;
    return newValue;
}

int ScoreKeeper::getCurrentScore(void) {
    return getScore(_currentIndex);
}

bool ScoreKeeper::currentIsTopTen(void) {
    return _currentIndex < (LEADERBOARD_SIZE - 1);
}

void ScoreKeeper::setNameForCurrent(const std::string& name) {
    _leaderBoard[_currentIndex].name = name;
}

void ScoreKeeper::setLeaderBoard(const std::string& scoreboardName) {
    ScoreBoards::iterator i;
    for (i = _scoreBoards.begin(); i < _scoreBoards.end(); i++) {
        if (i->name == scoreboardName) {
            break;
        }
    }

    if (i != _scoreBoards.end()) {
        stringToLeaderBoard(i->data, _leaderBoard);
    } else {
        resetLeaderBoard(_leaderBoard);
        string data = leaderBoardToString(_leaderBoard, scoreboardName);
        addScoreBoard(scoreboardName, data);
    }

    _leaderBoardName = scoreboardName;

    sendOnlineUpdateRequest(scoreboardName);
}

int ScoreKeeper::getScore(unsigned int index) {
    if (index < _leaderBoard.size()) {
        return _leaderBoard[index].score;
    }

    return 0;
}

int ScoreKeeper::getHighScore(void) {
    return getScore(0);
}

void ScoreKeeper::resetLeaderBoard(LeaderBoard& lb) {
    const int NUM_NAMES = 14;
    string names[NUM_NAMES] = {
        "AB", "MrT", "IB", "BBS", "MM", "ff", "Olli", "HSV", "Minden", "DrB", "DW", "SliQ", "Falo", "Howie",
    };

    int startIndex = _random.random();
    for (unsigned int i = 0; i < lb.size(); i++) {
        lb[i].score = 10 * i + 10;
        lb[i].name = names[(i + startIndex) % NUM_NAMES];
        time(&lb[i].time);
        lb[i].cubes = 0;
        lb[i].secondsPlayed = 0;
        lb[i].online = false;
    }

    sort(lb.begin(), lb.end());
}

void ScoreKeeper::sortLeaderBoard(void) {
    XTRACE();

    if (_currentIndex == 0) {
        return;  //we are leader, no update required
    }

    bool newPos = false;
    ScoreData tmpData = _leaderBoard[_currentIndex];
    while ((_currentIndex > 0) && (tmpData.score > _leaderBoard[_currentIndex - 1].score)) {
        _leaderBoard[_currentIndex] = _leaderBoard[_currentIndex - 1];
        _currentIndex--;
        newPos = true;
    }
    _leaderBoard[_currentIndex] = tmpData;

    //dumpLeaderBoard( _leaderBoard);
}

string ScoreKeeper::leaderBoardToString(LeaderBoard& lb, const string& leaderBoardName) {
    ostringstream ostr;
    ostr << leaderBoardName << "\001";

    for (unsigned int i = 0; i < lb.size(); i++) {
        if (lb[i].name == "") {
            lb[i].name = "Anonymous";
        }
        ostr << lb[i].name << "\002" << lb[i].score << "\002" << (long)lb[i].time << "\002" << lb[i].cubes << "\002"
             << lb[i].secondsPlayed << "\002" << lb[i].online << "\002";
    }

    LOG_INFO << "leaderBoardToString [" << ostr.str() << "]\n";

    return ostr.str();
}

void ScoreKeeper::stringToLeaderBoard(const string& lbString, LeaderBoard& lb) {
    Tokenizer t(lbString, false, "\001\002");

    string scoreboardName = t.next();
    if (scoreboardName != "") {
        lb.clear();

        bool done = false;
        while (!done) {
            ScoreData sData;
            sData.name = t.next();
            if (sData.name != "") {
                sData.score = atoi(t.next().c_str());
                sData.time = (time_t)atoi(t.next().c_str());

                string num = t.next();
                sData.cubes = 0;
                if (num.length()) {
                    sData.cubes = atoi(num.c_str());
                }

                num = t.next();
                sData.secondsPlayed = 0;
                if (num.length()) {
                    sData.secondsPlayed = atoi(num.c_str());
                }

                sData.online = false;
                num = t.next();
                if (num.length()) {
                    sData.online = atoi(num.c_str());
                }

                lb.push_back(sData);
            } else {
                done = true;
            }
        }
    }
    if (lb.size() < LEADERBOARD_SIZE) {
        lb.resize(LEADERBOARD_SIZE);
        resetLeaderBoard(lb);
    }

    //should already be sorted, but better be safe...
    sort(lb.begin(), lb.end());

    dumpLeaderBoard(lb);
}

void ScoreKeeper::dumpLeaderBoard(const LeaderBoard& lb) {
    LOG_INFO << "------LeaderBoard-----" << endl;
    for (unsigned int i = 0; i < lb.size(); i++) {
        LOG_INFO.width(30);
        LOG_VOID.fill('.');
        LOG_VOID.unsetf(ios::right);
        LOG_VOID.setf(ios::left);
        LOG_VOID << lb[i].name.c_str();

        LOG_VOID.width(10);
        LOG_VOID.unsetf(ios::left);
        LOG_VOID.setf(ios::right);
        LOG_VOID << lb[i].score;

        LOG_VOID.width(10);
        LOG_VOID.unsetf(ios::left);
        LOG_VOID.setf(ios::right);
        LOG_VOID << lb[i].cubes;

        LOG_VOID.width(10);
        LOG_VOID.unsetf(ios::left);
        LOG_VOID.setf(ios::right);
        LOG_VOID << lb[i].secondsPlayed;

        char buf[128];
        strftime(buf, 127, "%a %d-%b-%Y %H:%M", localtime(&lb[i].time));
        LOG_VOID << " : " << buf << endl;
    }
}

const string ScoreKeeper::getInfoText(unsigned int index) {
    string info = "";
    _infoIndex = index;

    if (index < _currentScoreboardAsLeaderBoard.size()) {
        info = _currentScoreboardAsLeaderBoard[index].name;
        info += " played on ";

        char buf[128];
        strftime(buf, 127, "%a %d-%b-%Y %H:%M", localtime(&_currentScoreboardAsLeaderBoard[index].time));

        info += buf;
    }

    return info;
}

void ScoreKeeper::updateScoreBoardWithLeaderBoard(void) {
    ScoreBoards::iterator i;
    for (i = _scoreBoards.begin(); i < _scoreBoards.end(); i++) {
        if (i->name == _leaderBoardName) {
            i->data = leaderBoardToString(_leaderBoard, _leaderBoardName);
            break;
        }
    }
}

static inline Point3Di nameToDimension(string name) {
    Tokenizer t(name, "x:");
    Point3Di dim;
    dim.x = atoi(t.next().c_str());
    dim.y = atoi(t.next().c_str());
    dim.z = atoi(t.next().c_str());
    return dim;
}

void ScoreKeeper::addScoreBoard(const string& scoreboardName, const string& data) {
    ScoreBoard newBoard;
    newBoard.name = scoreboardName;
    newBoard.dimension = ::nameToDimension(scoreboardName);
    newBoard.data = data;
    _scoreBoards.push_back(newBoard);

    sort(_scoreBoards.begin(), _scoreBoards.end());

    for (ScoreBoards::iterator i = _scoreBoards.begin(); i < _scoreBoards.end(); i++) {
        if (i->name == scoreboardName) {
            _currentScoreboard = i;
            break;
        }
    }
}

void ScoreKeeper::setActive(string scoreboardName) {
    updateScoreBoardWithLeaderBoard();

    if (scoreboardName == "") {
        scoreboardName = _leaderBoardName;
    }
    if (_currentScoreboard->name != scoreboardName) {
        _currentScoreboard = _scoreBoards.end();
        for (ScoreBoards::iterator i = _scoreBoards.begin(); i < _scoreBoards.end(); i++) {
            if (i->name == scoreboardName) {
                _currentScoreboard = i;
                break;
            }
        }
    }

    if (_currentScoreboard != _scoreBoards.end()) {
        stringToLeaderBoard(_currentScoreboard->data, _currentScoreboardAsLeaderBoard);
    } else {
        LOG_WARNING << "Board [" << scoreboardName << "] does not exist! Creating...\n";
        resetLeaderBoard(_currentScoreboardAsLeaderBoard);
        string data = leaderBoardToString(_currentScoreboardAsLeaderBoard, scoreboardName);
        addScoreBoard(scoreboardName, data);
    }

    sendOnlineUpdateRequest(scoreboardName);
}

void ScoreKeeper::nextBoard(void) {
    updateScoreBoardWithLeaderBoard();

    _currentScoreboard++;
    if (_currentScoreboard == _scoreBoards.end()) {
        _currentScoreboard = _scoreBoards.begin();
    }
    if (_currentScoreboard != _scoreBoards.end()) {
        stringToLeaderBoard(_currentScoreboard->data, _currentScoreboardAsLeaderBoard);
        sendOnlineUpdateRequest(_currentScoreboard->name);
    }
}

void ScoreKeeper::prevBoard(void) {
    updateScoreBoardWithLeaderBoard();

    if (_currentScoreboard == _scoreBoards.begin()) {
        _currentScoreboard = _scoreBoards.end();
    }
    _currentScoreboard--;
    if (_currentScoreboard != _scoreBoards.end()) {
        stringToLeaderBoard(_currentScoreboard->data, _currentScoreboardAsLeaderBoard);
        sendOnlineUpdateRequest(_currentScoreboard->name);
    }
}

void ScoreKeeper::load(void) {
    XTRACE();
    string scoreFilename = "leaderboard";
    LOG_INFO << "Loading hi-scores from " << scoreFilename << endl;

    int version = 1;
    ziStream infile(scoreFilename.c_str());
    if (infile.isOK())  //infile.good())
    {
        string line;
        while (!getline(infile, line).eof()) {
            LOG_INFO << "[" << line << "]" << endl;
            //explicitly skip comments
            if (line[0] == '#') {
                continue;
            }

            Tokenizer tv(line, false, " \t\n\r");
            string token = tv.next();
            if (token == "Version") {
                string versionString = tv.next();
                version = atoi(versionString.c_str());

                if (version != 1) {
                    LOG_ERROR << "Wrong version in score file!" << endl;
                }
            } else {
                Tokenizer t(line, false, "\001\002");
                string scoreboardName = t.next();

                if (scoreboardName != "") {
                    addScoreBoard(scoreboardName, line);
                }
            }
        }
    }
}

void ScoreKeeper::save(void) {
    XTRACE();
    string scoreFilename = "leaderboard";
    LOG_INFO << "Saving hi-scores to " << scoreFilename << endl;

    //Save scores in a compressed file to make it a bit tougher to cheat...
    zoStream outfile(scoreFilename.c_str());
    if (outfile.good()) {
        outfile << "#------LeaderBoard-----#" << endl;
        outfile << "Version 1" << endl;

        updateScoreBoardWithLeaderBoard();

        ScoreBoards::const_iterator ci;
        for (ci = _scoreBoards.begin(); ci != _scoreBoards.end(); ci++) {
            LOG_INFO << "[" << ci->data << "]" << endl;
            outfile << ci->data << endl;
        }
    }
}

void ScoreKeeper::draw(const Point2Di& offset) {
    static GLBitmapFont* fontWhite = FontManagerS::instance()->getFont("bitmaps/menuWhite");
    static GLBitmapFont* fontShadow = FontManagerS::instance()->getFont("bitmaps/menuShadow");

    static GLBitmapCollection* icons = BitmapManagerS::instance()->getBitmap("bitmaps/menuIcons");

    string scoreBoardHeader = _currentScoreboard->name + ":";
    fontShadow->setColor(1.0, 1.0, 1.0, 1.0);
    fontShadow->DrawString(scoreBoardHeader.c_str(), 90.0f + offset.x + 9.0f, 480.0f + offset.y - 9.0f, 1.0f, 1.0f);
    fontWhite->DrawString(scoreBoardHeader.c_str(), 90.0f + offset.x, 480.0f + offset.y, 1.0f, 1.0f);

    if (numBoards() > 1) {
        icons->bind();
        int slider = icons->getIndex("Slider");
        icons->Draw(slider, vec2i(offset.x + 40, offset.y + 487), vec2i(offset.x + 80, offset.y + 512));
        icons->Draw(slider, vec2i(offset.x + 490, offset.y + 487), vec2i(offset.x + 450, offset.y + 512));
    }

    float scale = 1.0;
    float spacing = 39;
    size_t maxIndex = std::min((size_t)(LEADERBOARD_SIZE - 1), _currentScoreboardAsLeaderBoard.size());
    for (unsigned int i = 0; i < maxIndex; i++) {
        char score[128];
        sprintf(score, "%d", _currentScoreboardAsLeaderBoard[i].score);
        float scoreWidth = fontWhite->GetWidth(score, scale);

        char place[10];
        sprintf(place, "%d.", i + 1);
        float placeWidth = fontWhite->GetWidth(place, scale);

        fontShadow->setColor(1.0, 1.0, 1.0, 1.0);
        fontShadow->DrawString(place, 80 + offset.x - placeWidth + 7, 420 + offset.y - (float)i * spacing - 7, scale,
                               scale);
        fontShadow->DrawString(_currentScoreboardAsLeaderBoard[i].name.c_str(), 90 + offset.x + 7.0f,
                               420 + offset.y - (float)i * spacing - 7, scale, scale);
        fontShadow->DrawString(score, 560 + offset.x - scoreWidth + 7, 420 + offset.y - (float)i * spacing - 7, scale,
                               scale);

        if (i == _infoIndex) {
            fontWhite->setColor(1.0, 0.1f, 0.1f, 1.0);
        } else if (_leaderBoardName == _currentScoreboard->name && i == _currentIndex) {
            fontWhite->setColor(1.0, 0.852f, 0.0, 1.0);
        } else if (_currentScoreboardAsLeaderBoard[i].online) {
            fontWhite->setColor(1.0, 1.0, 1.0, 1.0);
        } else {
            fontWhite->setColor(1.0, 0.95f, 0.7f, 1.0);
        }

        fontWhite->DrawString(place, 80 + offset.x - placeWidth, 420 + offset.y - (float)i * spacing, scale, scale);
        fontWhite->DrawString(_currentScoreboardAsLeaderBoard[i].name.c_str(), 90.0f + offset.x,
                              420 + offset.y - (float)i * spacing, scale, scale);
        fontWhite->DrawString(score, 560 + offset.x - scoreWidth, 420 + offset.y - (float)i * spacing, scale, scale);
    }
}

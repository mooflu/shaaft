#pragma once

#include <string>
#include <list>

class OnlineScore
{
public:
    static void SendScore( int score, const std::string &scoreMsg );
    static void RequestTopScores( const std::string &boardName, const std::string &scoreMsg);
    
    static bool GetNextScoreBoardData( std::string &boardName, std::string &scoreData);
    static void AddScoreBoardData( const std::string &boardName, const std::string &data);
};
#pragma once
// Description:
//   High level infrastructure for game.
//
// Copyright (C) 2003 Frank Becker
//
#include <fstream>
#include <sstream>

#include "Singleton.hpp"

#include "BlockModel.hpp"
#include "BlockController.hpp"
#include "BlockView.hpp"

class Game {
    friend class Singleton<Game>;

public:
    bool init(void);
    void run(void);
    void reset(void);
    void startNewGame(void);

    static void gameLoop(void);

private:
    ~Game();
    Game(void);
    Game(const Game&);
    Game& operator=(const Game&);

    void updateOtherLogic(void);
    void updateInGameLogic(void);

    enum SetupModelEnum {
        ModelCreate,
        ModelReset
    };

    void setupModel(SetupModelEnum setupType);

    BlockModel* _model;
    BlockController* _controller;
    BlockView* _view;

    std::ostream* _zo;
    std::ofstream* _oStream;

    std::istream* _zi;
    std::ifstream* _iStream;
};

typedef Singleton<Game> GameS;

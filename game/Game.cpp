// Description:
//   High level infrastructure for game.
//
// Copyright (C) 2003 Frank Becker
//
#include "Game.hpp"

#include <time.h>

#include "Trace.hpp"

#include "GameState.hpp"
#include "Constants.hpp"
#include "Config.hpp"
#include "gettimeofday.hpp"

#include "Audio.hpp"
#include "Input.hpp"
#include "BlockView.hpp"

#include "ScoreKeeper.hpp"
#include "MenuManager.hpp"

#include <ParticleGroup.hpp>
#include <ParticleGroupManager.hpp>
#include "VideoBase.hpp"

#include "zStream.hpp"
#include "ResourceManager.hpp"

#if defined(EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

using namespace std;

Game::Game(void) :
    _model(0),
    _controller(0),
    _view(0),
    _zo(0),
    _oStream(0),
    _zi(0),
    _iStream(0) {
    XTRACE();
}

Game::~Game() {
    XTRACE();

    LOG_INFO << "Shutting down..." << endl;

    delete _model;
    delete _controller;

    delete _zo;
    delete _oStream;
    delete _zi;
    delete _iStream;

    MenuManagerS::cleanup();
    ParticleGroupManagerS::cleanup();

    AudioS::cleanup();
    delete _view;  //calls SDL_Quit

    // save config stuff
    LOG_INFO << "Config cleanup..." << endl;
    ConfigS::instance()->saveToFile();

    // save leaderboard
    ScoreKeeperS::instance()->save();
    ScoreKeeperS::cleanup();
    InputS::cleanup();

    // Note: this shuts down PHYSFS
    LOG_INFO << "ResourceManager cleanup..." << endl;
    ResourceManagerS::cleanup();
}

bool Game::init(void) {
    XTRACE();
    bool result = true;

    ScoreKeeperS::instance()->load();

    AudioS::instance()->setDefaultSoundtrack("music/shaaft.ogg");
    if (!AudioS::instance()->init()) {
        return false;
    }
#if 0
    EventInjector *ei = 0;
    string play;
    if( ConfigS::instance()->getString( "play", play))
    {
        _iStream = new ifstream( play.c_str(), ios::in | ios::binary);
        _zi = new ziStream( *_iStream);

        string line;
        getline( *_zi, line);
        getline( *_zi, line);

        Tokenizer token(line);
        unsigned int version = strtoul(token.next().c_str(), 0, 10);
        unsigned int seed = strtoul(token.next().c_str(), 0, 10);

        LOG_INFO << "Line [" << line << "] version=" << version << " seed=" << seed << endl;

        GameState::r250.reset(seed);

        ei = new EventInjector(*_zi);
    }

    EventWatcher *ew = 0;
    string record;
    if( ConfigS::instance()->getString( "record", record))
    {
        _oStream = new ofstream( record.c_str(), ios::out | ios::binary);
        ostream *os = _oStream;

        bool compress = false;
        ConfigS::instance()->getBoolean( "compress", compress);

        if( compress)
        {
            _zo = new zoStream( *_oStream);
            os = _zo;
        }

        char rTime[128];
        time_t t;
        time(&t);
        strftime( rTime, 127, "%a %d-%b-%Y at %H:%M", localtime(&t));

        (*os) << "# Game recorded on " << rTime << endl;
        (*os) << "1 " << GameState::r250.getSeed() << endl;

        ew = new EventWatcher(*os);
    }
#else
    struct timeval tv;
    gettimeofday(&tv, 0);
    GameState::r250.reset(tv.tv_sec);
#endif

    setupModel(ModelCreate);

    if (!_model->init()) {
        return false;
    }

    _view = new BlockView(*_model);
    _model->registerView(_view);
    if (!_view->init()) {
        return false;
    }

    // input init also initializes Keys which requires that SDL video has ben initialized
    if (!InputS::instance()->init()) {
        return false;
    }

    _controller = new BlockController(*_model);
#if 0
    if( ew) _controller->setEventWatcher( ew);
    if( ei) _controller->setEventInjector( ei);
#endif

    if (!MenuManagerS::instance()->init()) {
        return false;
    }

    ParticleGroupManager* pgm = ParticleGroupManagerS::instance();
    if (!pgm->init()) {
        return false;
    }
    //there are 3 effect groups to give very simple control over the order
    //of drawing which is important for alpha blending.
    pgm->addGroup(EFFECTS_GROUP1, 1000);
    pgm->addGroup(EFFECTS_GROUP2, 1000);
    pgm->addGroup(EFFECTS_GROUP3, 1000);

    //reset our stopwatch
    GameState::stopwatch.reset();
    GameState::stopwatch.pause();

    GameState::mainTimer.reset();

    //make sure we start of in menu mode
    MenuManagerS::instance()->turnMenuOn();

    GameState::startOfStep = GameState::mainTimer.getTime();
    GameState::startOfGameStep = GameState::stopwatch.getTime();

    LOG_INFO << "Initialization complete OK." << endl;
    return result;
}

void Game::reset(void) {
    ScoreKeeperS::instance()->updateScoreBoardWithLeaderBoard();

    ParticleGroupManagerS::instance()->reset();

    //reset in order to start new game
    GameState::stopwatch.reset();
    GameState::startOfGameStep = GameState::stopwatch.getTime();
    GameState::gameTick = 0;
    GameState::isAlive = true;
    GameState::secondsPlayed = 0.0;

    setupModel(ModelReset);
}

void Game::setupModel(SetupModelEnum setupType) {
    int dimx = 5;
    int dimy = 5;
    int dimz = 12;
    int startLevel = 1;
    string blockset = "Shaaft";
    ConfigS::instance()->getInteger("shaftWidth", dimx);
    ConfigS::instance()->getInteger("shaftHeight", dimy);
    ConfigS::instance()->getInteger("shaftDepth", dimz);
    ConfigS::instance()->getInteger("startLevel", startLevel);
    ConfigS::instance()->getString("blockset", blockset);

    if (setupType == ModelCreate) {
        _model = new BlockModel(dimx, dimy, dimz, startLevel, blockset, GameState::r250);
    } else {
        _model->reset(dimx, dimy, dimz, startLevel, blockset);
    }

    bool practiceMode = false;
    ConfigS::instance()->getBoolean("practiceMode", practiceMode);
    _model->setPracticeMode(practiceMode);

    ostringstream ostr;
    ostr << dimx << "x" << dimy << "x" << dimz << ":" << blockset;

    LOG_INFO << "Setting active score board to [" << ostr.str() << "]" << endl;
    ScoreKeeperS::instance()->setLeaderBoard(ostr.str());
    ScoreKeeperS::instance()->resetCurrentScore();
    ScoreKeeperS::instance()->setPracticeMode(practiceMode);
}

void Game::startNewGame(void) {
    GameS::instance()->reset();
    GameState::context = Context::eInGame;
    InputS::instance()->disableInterceptor();
    GameState::stopwatch.start();
}

void Game::updateOtherLogic(void) {
    int stepCount = 0;
    double currentTime = GameState::mainTimer.getTime();
    while ((currentTime - GameState::startOfStep) > GAME_STEP_SIZE) {
        if (GameState::context == Context::eMenu) {
            MenuManagerS::instance()->update();
        }

        //advance to next start-of-game-step point in time
        GameState::startOfStep += GAME_STEP_SIZE;
        currentTime = GameState::mainTimer.getTime();

        stepCount++;
        if (stepCount > MAX_GAME_STEPS) {
            break;
        }
    }

    if (GameState::context == Context::eMenu || GameState::context == Context::ePaused) {
        // When in menu or paused, process input on every frame.
        // Especially in menu the mouse cursor needs to update every frame.
        InputS::instance()->update();
    }

    GameState::frameFractionOther = (float)((currentTime - GameState::startOfStep) / GAME_STEP_SIZE);

    if (stepCount > 1) {
        //LOG_WARNING << "Skipped " << stepCount << " frames." << endl;

        if (GameState::frameFractionOther > 1.0) {
            //Our logic is still way behind where it should be at this
            //point in time. If we get here we already ran through
            //MAX_GAME_STEPS logic runs trying to catch up.

            //We clamp the value to 1.0, higher values would try
            //to predict were we are visually. Maybe not a bad idead
            //to allow values up to let's say 1.3...
            GameState::frameFractionOther = 1.0;
        }
    }
}

void Game::updateInGameLogic(void) {
    int stepCount = 0;
    double currentGameTime = GameState::stopwatch.getTime();
    while ((currentGameTime - GameState::startOfGameStep) > GAME_STEP_SIZE) {
        GameState::prevShaftPitch = GameState::shaftPitch;
        GameState::prevShaftYaw = GameState::shaftYaw;

        ParticleGroupManagerS::instance()->update();
        InputS::instance()->update();

        if (GameState::isAlive) {
            GameState::secondsPlayed = GameState::stopwatch.getTime();
            if (!_model->update()) {
                GameState::isAlive = false;
            }
        }
        _view->update();

        //advance to next start-of-game-step point in time
        GameState::startOfGameStep += GAME_STEP_SIZE;
        GameState::gameTick++;
        currentGameTime = GameState::stopwatch.getTime();

        stepCount++;
        if (stepCount > MAX_GAME_STEPS) {
            break;
        }
    }

    GameState::frameFraction = (float)((currentGameTime - GameState::startOfGameStep) / GAME_STEP_SIZE);

    if (stepCount > 1) {
        if (GameState::frameFraction > 1.0) {
            //Our logic is still way behind where it should be at this
            //point in time. If we get here we already ran through
            //MAX_GAME_STEPS logic runs trying to catch up.

            //We clamp the value to 1.0, higher values would try
            //to predict were we are visually. Maybe not a bad idead
            //to allow values up to let's say 1.3...
            GameState::frameFraction = 1.0;
        }
    }
}

void Game::gameLoop() {
    Game& game = *GameS::instance();
    Audio& audio = *AudioS::instance();

    switch (GameState::context) {
        case Context::eInGame:
            //stuff that only needs updating when game is actually running
            game.updateInGameLogic();
            break;

        default:
            break;
    }

    //stuff that should run all the time
    game.updateOtherLogic();

    ScoreKeeperS::instance()->mergeOnlineScores();

    audio.update();

    game._view->draw();
    MenuManagerS::instance()->draw();
    VideoBaseS::instance()->swap();

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERROR << "GL ERROR: " << std::hex << err << "\n";
    }

#if defined(EMSCRIPTEN)
    if (GameState::requestExit) {
        GameS::cleanup();
        ConfigS::cleanup();

        EM_ASM(console.log('FS.syncfs...'); FS.syncfs(function(err) {
            if (err) {
                console.log('FS.syncfs error: ' + err)
            }
            console.log('syncfs done');
            Module.gamePostRun();
        }););

        emscripten_exit_pointerlock();
        emscripten_cancel_main_loop();
        //emscripten_exit_with_live_runtime();
    }
#endif
}

void Game::run(void) {
    XTRACE();

    // Here it is: the main loop.
    LOG_INFO << "Entering Main loop." << endl;
#if defined(IPHONE)
    Game::gameLoop();
#else
    while (!GameState::requestExit) {
        Game::gameLoop();
    }
#endif
}

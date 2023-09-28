#pragma once
// Description:
//   Video base.
//
// Copyright (C) 2011 Frank Becker
//
#include "SDL.h"
#include "Singleton.hpp"

#include <list>

class ResolutionChangeObserverI {
public:
    virtual void resolutionChanged(int w, int h) = 0;

    virtual ~ResolutionChangeObserverI() {}
};

class VideoBase {
    friend class Singleton<VideoBase>;

public:
    VideoBase(void);
    virtual ~VideoBase();

    bool init(void);
    void draw(bool oneToOne = true);
    void swap(void);

    int getWidth(void) { return _width; }

    int getHeight(void) { return _height; }

    bool isFullscreen(void) { return _isFullscreen; }

    void takeSnapshot(void);
    void setResolutionConfig(int w, int h, bool fs);

    //This should be called if fullscreen or resolution were changed
    //The actual values are retrieved from config (width, height, fullscreen)
    bool updateSettings(void);

    bool update(void);

    void registerResolutionObserver(ResolutionChangeObserverI* i);

private:
    VideoBase(const VideoBase&);
    VideoBase& operator=(const VideoBase&);

    void reload(void);
    bool setVideoMode(void);

    bool _isFullscreen;

    int _bpp;
    int _width;
    int _height;

    int _prevWidth;
    int _prevHeight;

    SDL_Window* _windowHandle;
    SDL_GLContext _glContext;

    int _pointer;

    std::list<ResolutionChangeObserverI*> _resolutionObservers;
};

typedef Singleton<VideoBase> VideoBaseS;

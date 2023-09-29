// Description:
//   Video base.
//
// Copyright (C) 2007 Frank Becker
//

#include "SDL.h"
#include <math.h>

#include "Trace.hpp"
#include "Config.hpp"
#include "Value.hpp"
#include "Timer.hpp"

#include "PNG.hpp"

#include "Constants.hpp"
#include "VideoBase.hpp"
#include "GameState.hpp"
//#include "BaseGameState.hpp"

#include <GL/glew.h>
#include "Camera.hpp"
#include "BitmapManager.hpp"
#include "FontManager.hpp"
#include "ModelManager.hpp"
#include "TextureManager.hpp"

#include "GLBitmapCollection.hpp"
#include "Input.hpp"

using namespace std;

#ifndef IPHONE
const int VIDEO_DEFAULT_WIDTH = 0;
const int VIDEO_DEFAULT_HEIGHT = 0;
#else
const int VIDEO_DEFAULT_WIDTH = 320;
const int VIDEO_DEFAULT_HEIGHT = 480;
#endif

VideoBase::VideoBase() :
    _isFullscreen(true),
    _bpp(0),
    _width(VIDEO_DEFAULT_WIDTH),
    _height(VIDEO_DEFAULT_HEIGHT),
    _prevWidth(VIDEO_DEFAULT_WIDTH),
    _prevHeight(VIDEO_DEFAULT_HEIGHT),
    _windowHandle(0),
    _glContext(0) {
#ifdef IPHONE
    _width = gGameState->width;
    _height = gGameState->height;
#endif
}

VideoBase::~VideoBase() {
    LOG_INFO << "VideoBase shutdown..." << endl;

    BitmapManagerS::cleanup();
    FontManagerS::cleanup();
    ModelManagerS::cleanup();

    TextureManagerS::cleanup();

    CameraS::cleanup();

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}

void VideoBase::reload(void) {
    BitmapManagerS::instance()->reset();
    FontManagerS::instance()->reset();
    ModelManagerS::instance()->reset();

    BitmapManagerS::instance()->reload();
    FontManagerS::instance()->reload();
    ModelManagerS::instance()->reload();

    std::list<ResolutionChangeObserverI*>::iterator i;
    for (i = _resolutionObservers.begin(); i != _resolutionObservers.end(); i++) {
        (*i)->resolutionChanged(_width, _height);
    }
}

void VideoBase::registerResolutionObserver(ResolutionChangeObserverI* i) {
    _resolutionObservers.push_back(i);
}

bool VideoBase::init(void) {
    LOG_INFO << "Initializing VideoBase..." << endl;

    if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
        LOG_ERROR << "Init VideoBase: failed # " << SDL_GetError() << endl;
        return false;
    }
    LOG_INFO << "VideoBase: OK" << endl;

    //const char *vidDriver = SDL_GetCurrentVideoDriver(); //E.g. "Windows"

    if (!setVideoMode()) {
        return false;
    }
#if 0
    GLBitmapCollection *icons =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuIcons");
    if( !icons)
    {
        LOG_ERROR << "Unable to load menuIcons." << endl;
        return false;
    }
    _pointer = icons->getIndex( "Pointer");
#endif
    return true;
}

bool VideoBase::setVideoMode(void) {
    ConfigS::instance()->getBoolean("fullscreen", _isFullscreen);
    ConfigS::instance()->getInteger("width", _width);
    ConfigS::instance()->getInteger("height", _height);
    _prevWidth = _width;
    _prevHeight = _height;

    if ((_width == 0) && (_height == 0)) {
        SDL_DisplayMode defaultMode;
        SDL_GetDesktopDisplayMode(0, &defaultMode);

        _width = defaultMode.w;
        _height = defaultMode.h;
    }

#if 0
    int numDisplays = SDL_GetNumVideoDisplays();
    for( int i=0; i<numDisplays; i++)
    {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(i, &mode);
        LOG_INFO << "Desktop Mode: "
            << mode.w << "x" << mode.h << "x" << SDL_BITSPERPIXEL(mode.format) << " "
            << mode.refresh_rate << "Hz\n";

        LOG_INFO << "Available Modes:\n";
        int numModes = SDL_GetNumDisplayModes(i);
        for( int m=0; m<numModes; m++)
        {
            SDL_GetDisplayMode(i, m, &mode);
            LOG_INFO << "  "
                << mode.w << "x" << mode.h << "x" << SDL_BITSPERPIXEL(mode.format) << " "
                << mode.refresh_rate << "Hz\n";
        }
    }
#endif

    Uint32 windowFlags = SDL_WINDOW_OPENGL;
    if (_isFullscreen) {
        LOG_INFO << "Fullscreen request." << endl;
        windowFlags |= SDL_WINDOW_FULLSCREEN;
    }

#if defined(EMSCRIPTEN)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
    if (_glContext) {
        SDL_GL_DeleteContext(_glContext);
        _glContext = 0;
    }

    if (_windowHandle) {
        SDL_DestroyWindow(_windowHandle);
        _windowHandle = 0;
    }

    _windowHandle = SDL_CreateWindow(GAMETITLE.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, _width,
                                     _height, windowFlags);
    if (!_windowHandle) {
        LOG_ERROR << "Video Mode: failed to create window: " << SDL_GetError() << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    _glContext = SDL_GL_CreateContext(_windowHandle);
    if (!_glContext) {
        LOG_ERROR << "Video Mode: failed to create GL context: " << SDL_GetError() << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

#if 0
    SDL_ShowWindow(_windowHandle);

    if( SDL_SetWindowDisplayMode( _windowHandle, windowFlags ) == -1 )
    {
        LOG_ERROR << "Video Mode: failed #" << SDL_GetError() << endl;
        SDL_QuitSubSystem( SDL_INIT_VIDEO);
        return false;
    }

    SDL_GetWindowSize(_windowHandle, &_width, &_height);
#endif

    glViewport(0, 0, _width, _height);

    //reset mouse position ang grab-state
    InputS::instance()->resetMousePosition();

#if 1
    SDL_SetRelativeMouseMode(SDL_TRUE);
#else
    // SDL_SetRelativeMouseMode used to only work on Mac
    SDL_ShowCursor(SDL_DISABLE);
    bool grabMouse = true;
    ConfigS::instance()->getBoolean("grabMouse", grabMouse);
    if (grabMouse || _isFullscreen) {
        SDL_SetWindowGrab(_windowHandle, SDL_TRUE);
    } else {
        SDL_SetWindowGrab(_windowHandle, SDL_FALSE);
    }
    // LOG_INFO << "MOUSEX " << InputS::instance()->mousePos().x() << "\n";
    // LOG_INFO << "MOUSEY " << InputS::instance()->mousePos().y() << "\n";
    SDL_WarpMouseInWindow(_windowHandle, InputS::instance()->mousePos().x(), InputS::instance()->mousePos().y());
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        //remove any queued up events due to warping, etc.
        ;
    }
#endif

    SDL_DisplayMode currentMode;
    SDL_GetCurrentDisplayMode(0, &currentMode);
    LOG_INFO << "Video Mode: OK (" << _width << "x" << _height << "x" << SDL_BITSPERPIXEL(currentMode.format) << ")"
             << endl;

    glewInit();

    int major = -1;
    int minor = -1;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

    LOG_INFO << "OpenGL info:" << endl;
    LOG_INFO << "  Vendor  : " << glGetString(GL_VENDOR) << endl;
    LOG_INFO << "  Renderer: " << glGetString(GL_RENDERER) << endl;
    LOG_INFO << "  Version : " << glGetString(GL_VERSION) << endl;
    LOG_INFO << "  Context : " << major << "." << minor << endl;
    LOG_INFO << "  GLEW : " << glewGetString(GLEW_VERSION) << endl;

#if 0
    GLint range[2];
    glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, range);
    LOG_INFO << "  line width range aa: " << range[0] << "-" << range[1] << endl;
    glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, range);
    LOG_INFO << "  line width range smooth: " << range[0] << "-" << range[1] << endl;
#endif
    return true;
}

bool VideoBase::updateSettings(void) {
    bool fullscreen = true;
    ConfigS::instance()->getBoolean("fullscreen", fullscreen);
    int width = 0;
    ConfigS::instance()->getInteger("width", width);
    int height = 0;
    ConfigS::instance()->getInteger("height", height);

    if ((fullscreen != _isFullscreen) || (width != _prevWidth) || (height != _prevHeight)) {
        LOG_INFO << "current:" << (_isFullscreen ? "fs " : "win") << " " << _prevWidth << "x" << _prevHeight << "\n";
        LOG_INFO << "request:" << (fullscreen ? "fs " : "win") << " " << width << "x" << height << "\n";
        LOG_INFO << "VideoBase::updateSettings change detected...\n";

#if 0
//not working on Linux vm
        if( (width == _prevWidth) && (height == _prevHeight))
        {
            //only changing fullscreen
            if( SDL_SetWindowFullscreen(_windowHandle, fullscreen ? SDL_TRUE : SDL_FALSE) == 0)
            {
                ConfigS::instance()->getBoolean( "fullscreen", _isFullscreen);
                return true;
            }
            LOG_WARNING << "Unable to set fullscren. Trying complete mode change...\n";
        }
#endif

        bool oldFullscreen = _isFullscreen;
        int oldWidth = _prevWidth;
        int oldHeight = _prevHeight;
        if (!setVideoMode()) {
            LOG_WARNING << "Unable to set video mode. Trying previous settings.\n";
            setResolutionConfig(oldWidth, oldHeight, oldFullscreen);

            if (!setVideoMode()) {
                LOG_WARNING << "Unable to set previous video mode. Trying default!\n";
                setResolutionConfig(0, 0, true);

                if (!setVideoMode()) {
                    LOG_ERROR << "Unable to set default video mode. Going down!\n";
                    //no luck, we are going down!
                    return false;
                }
            }
        }
        reload();
    }

    return true;
}

void VideoBase::setResolutionConfig(int width, int height, bool fullscreen) {
    //going back to desktop res fullscreen
    Value* w = new Value(width);
    ConfigS::instance()->updateKeyword("width", w);
    Value* h = new Value(height);
    ConfigS::instance()->updateKeyword("height", h);
    Value* fs = new Value(fullscreen);
    ConfigS::instance()->updateKeyword("fullscreen", fs);
}

bool VideoBase::update(void) {
    static double nextTime = Timer::getTime() + 0.5;
    double thisTime = Timer::getTime();
    if (thisTime > nextTime) {
        nextTime = thisTime + 0.5;
        return updateSettings();
    }
    return true;
}

void VideoBase::draw(bool oneToOne) {
#if 0
    //--- Ortho stuff from here on ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float orthoWidth;
    float orthoHeight;

    if( oneToOne)
    {
        orthoWidth = getWidth();
        orthoHeight = getHeight();
    }
    else
    {
        orthoWidth = (750.0*(float)getWidth()) / (float)getHeight();
        orthoHeight = 750.0;
    }
    glOrtho(-0.5,orthoWidth+0.5,-0.5,orthoHeight+0.5, -1000.0, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable( GL_DEPTH_TEST);
    glDisable( GL_LIGHTING);

    glEnable(GL_TEXTURE_2D);
    GLBitmapCollection *icons =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuIcons");
    icons->bind();
    glColor4f(1.0, 1.0, 1.0, 1.0);
    float gf = GameState::frameFractionOther; // gGameState->frameFractionOther;
    static float _prevMouseX = 0;
    static float _prevMouseY = 0;
    float interpMouseX = _prevMouseX + (InputS::instance()->mousePos().x()-_prevMouseX)*gf;
    float interpMouseY = _prevMouseY + (InputS::instance()->mousePos().y()-_prevMouseY)*gf;
    _prevMouseX = InputS::instance()->mousePos().x();
    _prevMouseY = InputS::instance()->mousePos().y();
    icons->Draw( _pointer, interpMouseX, interpMouseY, 0.3, 0.3);
    glDisable(GL_TEXTURE_2D);
#endif
}

void VideoBase::takeSnapshot(void) {
    static int count = 0;

    char filename[128];
    sprintf(filename, "snap%02d.png", count++);
    SDL_Surface* img;

    img = SDL_CreateRGBSurface(0, _width, _height, 24, 0x00FF0000, 0x0000FF00, 0x000000FF, 0);

    if (img) {
        glReadPixels(0, 0, _width, _height, GL_RGB, GL_UNSIGNED_BYTE, img->pixels);

        LOG_INFO << "Writing snapshot: " << filename << endl;
        if (!PNG::Snapshot(img, filename)) {
            LOG_ERROR << "Failed to save snapshot." << endl;
        }
        SDL_FreeSurface(img);
    } else {
        LOG_ERROR << "Failed to create surface for snapshot." << endl;
        LOG_ERROR << "SDL: " << SDL_GetError() << "\n";
    }
}

void VideoBase::swap(void) {
    SDL_GL_SwapWindow(_windowHandle);
}

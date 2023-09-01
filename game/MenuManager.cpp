// Description:
//   Menu Manager/Controller.
//
// Copyright (C) 2001 Frank Becker
//
#include "MenuManager.hpp"

#include "SDL.h" //key syms

#include "Trace.hpp"
#include "XMLHelper.hpp"
#include "GameState.hpp"
#include "Audio.hpp"
#include "FontManager.hpp"
#include "SelectableFactory.hpp"
#include "BitmapManager.hpp"
#include "Config.hpp"
#include "Value.hpp"
#include "Trigger.hpp"

#include <GL/glew.h>
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "gl3/ProgramManager.hpp"
#include "gl3/Program.hpp"

#include "Input.hpp"
#include "VideoBase.hpp"
#if 0
#include "Game.hpp"
#endif

using namespace std;

MenuManager::MenuManager():
    _menu(0),
    _topMenu(0),
    _currentMenu(0),
    _mouseX(200.0),
    _mouseY(200.0),
    _prevContext(Context::eUnknown),
    _delayedExit(false),
    _newLevelLoaded(false),
    _showCursorAnim(true),
    _cursorAnim( "CursorAnim", 1000)
{
    XTRACE();

    updateSettings();
}

MenuManager::~MenuManager()
{
    XTRACE();

    clearActiveSelectables();

    SelectableFactory::cleanup();

    delete _menu;
    _menu = 0;
}

bool MenuManager::init( void)
{
    XTRACE();
    _menu = XMLHelper::load( "system/Menu.xml");
    if( !_menu)
    {
        _menu = 0;
        return false;
    }

    _currentMenu = _menu->FirstChild("Menu");
    _topMenu = _currentMenu;

    loadMenuLevel();

    GLBitmapCollection *icons =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuIcons");
    if( !icons)
    {
        LOG_ERROR << "Unable to load menuIcons." << endl;
        return false;
    }
    _pointer = icons->getIndex( "Pointer");

    GLBitmapCollection *menuBoard =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuBoard");
    if( !menuBoard)
    {
        LOG_ERROR << "Unable to load menuBoard." << endl;
        return false;
    }
    _board = menuBoard->getIndex( "MenuBoard");

    _cursorAnim.init();

    return true;
}

void MenuManager::updateSettings( void)
{
    if( ! ConfigS::instance()->getBoolean( "showCursorAnimation", _showCursorAnim))
    {
        Value *v = new Value( _showCursorAnim);
        ConfigS::instance()->updateKeyword( "showCursorAnimation", v);
    }
}

void MenuManager::clearActiveSelectables( void)
{
    list<Selectable*>::iterator i;
    for( i=_activeSelectables.begin(); i!=_activeSelectables.end(); i++)
    {
        delete (*i);
    }
    _activeSelectables.clear();

    Selectable::reset();
}

void MenuManager::loadMenuLevel( void)
{
    _newLevelLoaded = true;
    clearActiveSelectables();

    TiXmlNode *node = _currentMenu->FirstChild();
    while( node)
    {
//        LOG_INFO << "MenuItem: [" << node->Value() << "]" << endl;

        SelectableFactory *selF = SelectableFactory::getFactory( node->Value());
        if( selF)
        {
            Selectable *sel = selF->createSelectable( node);
            if( sel)
            {
                _activeSelectables.insert( _activeSelectables.end(), sel);
            }
        }
        else
        {
            LOG_WARNING << "No Factory found for:" << node->Value() << endl;
        }
        node = node->NextSibling();
    }

    //add escape button
    if( _currentMenu != _topMenu)
    {
        BoundingBox r;
        r.min.x = 510;
        r.min.y = 510;
        r.max.x = r.min.x+48;
        r.max.y = r.min.y+48;

        Selectable *sel = new EscapeSelectable( true, r, 0.4);
        _activeSelectables.insert( _activeSelectables.end(), sel);
    }
    _currentSelectable = _activeSelectables.begin();
    activateSelectableUnderMouse();
}

void MenuManager::makeMenu( TiXmlNode *_node)
{
    _currentMenu = _node;
    loadMenuLevel();
}

bool MenuManager::update( void)
{
    static float nextTime = Timer::getTime()+0.5f;
    float thisTime = Timer::getTime();
    if( thisTime > nextTime)
    {
        updateSettings();
        nextTime = thisTime+0.5f;
    }

    if( _delayedExit)
    {
        if( !Exit())
        {
            turnMenuOff();
        }
        _delayedExit = false;
    }
    list<Selectable*>::iterator i;
    for( i=_activeSelectables.begin(); i!=_activeSelectables.end(); i++)
    {
        (*i)->update();
    }

    if( _showCursorAnim)
    {
#ifndef IPHONE
        GLBitmapCollection *icons =
            BitmapManagerS::instance()->getBitmap( "bitmaps/particles");
        float iw = icons->getWidth( _pointer);
        float ih = icons->getHeight( _pointer);
#endif
        for( int j=0; j<20; j++)
        {
            float interpMouseX = _prevMouseX + (_mouseX-_prevMouseX)*((19.0-(float)j)/19.0);
            float interpMouseY = _prevMouseY + (_mouseY-_prevMouseY)*((19.0-(float)j)/19.0);
#ifdef IPHONE
            float w = interpMouseX;// + iw*0.5;
            float h = interpMouseY;// - ih*0.5;
#else
            float w = interpMouseX + iw*1.4;
            float h = interpMouseY - ih*2.4;
#endif
            _cursorAnim.newParticle( "Spark", w, h, 0);
        }

        _cursorAnim.update();
    }

    _prevMouseX = _mouseX;
    _prevMouseY = _mouseY;

    return true;
}

bool MenuManager::draw( void)
{
    if( GameState::context != Context::eMenu) return true;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    VideoBase &video = *VideoBaseS::instance();
    glViewport(0, 0, video.getWidth(), video.getHeight());

    float orthoHeight = 750.0;
    float orthoWidth = (750.0*(float)video.getWidth()) / (float)video.getHeight();

    glm::mat4 projM = glm::ortho(-0.5f,orthoWidth+0.5f,-0.5f,orthoHeight+0.5f, -1000.0f, 1000.0f);
    glm::mat4 modelM(1.0f);

#if 0
    //--- Ortho stuff from here on ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float orthoHeight = 750.0;
    float orthoWidth = (750.0*(float)video.getWidth()) / (float)video.getHeight();

    glOrtho(-0.5,orthoWidth+0.5,-0.5,orthoHeight+0.5, -1000.0, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif

    glm::mat4 modelViewMatrix = projM * modelM;
    Program *prog = ProgramManagerS::instance()->getProgram("texture");
    prog->use(); //needed to set uniforms
    GLint modelViewMatrixLoc = glGetUniformLocation( prog->id(), "modelViewMatrix");
    glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(modelViewMatrix) );

    glDisable( GL_DEPTH_TEST);
    //glDisable( GL_LIGHTING);

    glActiveTexture(GL_TEXTURE0);

    GLBitmapCollection *menuBoard =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuBoard");
    menuBoard->bind();

    float boardScale = (float)min(orthoWidth, orthoHeight) * 0.8 / 512.0;
    _boardOffset.x = (int)((orthoWidth - (float)menuBoard->getWidth(_board)*boardScale) / 2.0);
    _boardOffset.y = (int)((orthoHeight - (float)menuBoard->getHeight(_board)*boardScale) / 2.0);

    menuBoard->setColor(vec4f(1.0, 1.0, 1.0, 0.9f));
    menuBoard->Draw( _board, _boardOffset.x, _boardOffset.y, boardScale, boardScale);

    TiXmlElement* elem = _currentMenu->ToElement();
    const char *val = elem->Attribute("Text");
    if( val)
    {
        float titleScale = 1.5;
        GLBitmapFont &fontShadow =
            *(FontManagerS::instance()->getFont( "bitmaps/menuShadow"));
        GLBitmapFont &fontWhite =
            *(FontManagerS::instance()->getFont( "bitmaps/menuWhite"));
        float w = fontWhite.GetWidth( val, titleScale);

        fontShadow.setColor(1.0, 1.0, 1.0, 1.0);
        fontShadow.DrawString( val,
                               _boardOffset.x+(menuBoard->getWidth(_board)*boardScale-w)/2.0+9,
                               _boardOffset.y+530-9, titleScale, titleScale);

        fontWhite.setColor(1.0, 1.0, 1.0, 1.0);
        fontWhite.DrawString( val,
                              _boardOffset.x+(menuBoard->getWidth(_board)*boardScale-w)/2.0,
                              _boardOffset.y+530, titleScale, titleScale);
    }

    list<Selectable*>::iterator i;
    for( i=_activeSelectables.begin(); i!=_activeSelectables.end(); i++)
    {
        (*i)->draw( _boardOffset);
    }

    if( _showCursorAnim)
    {
        _cursorAnim.draw();
    }

    GLBitmapCollection *icons =
        BitmapManagerS::instance()->getBitmap( "bitmaps/menuIcons");
    icons->bind();
    icons->setColor(1.0, 1.0, 1.0, 1.0);
    float gf = GameState::frameFractionOther;
    float interpMouseX = _prevMouseX + (_mouseX-_prevMouseX)*gf;
    float interpMouseY = _prevMouseY + (_mouseY-_prevMouseY)*gf;

    icons->Draw( _pointer, interpMouseX, interpMouseY, 0.5, 0.5);

    return true;
}

void MenuManager::reload( void)
{
}

void MenuManager::turnMenuOn( void)
{
    AudioS::instance()->playSample( "sounds/beep");
    _prevContext = GameState::context;
    GameState::context = Context::eMenu;

    //ask input system to forward all input to us
    InputS::instance()->enableInterceptor( this);
    GameState::stopwatch.pause();

#ifdef IPHONE
    _currentSelectable = _activeSelectables.end();
#else
    activateSelectableUnderMouse();
#endif
}

void MenuManager::turnMenuOff( void)
{
    if( _prevContext == Context::eUnknown) return;

    AudioS::instance()->playSample( "sounds/beep");
    GameState::context = _prevContext;

    //don't want anymore input
    InputS::instance()->disableInterceptor();

    if( GameState::context == Context::eInGame)
    {
        GameState::stopwatch.start();
    }
}

bool MenuManager::canReturnToGame( void)
{
    return (_prevContext == Context::eInGame) || (_prevContext == Context::ePaused);
}

void MenuManager::input( const Trigger &trigger, const bool &isDown)
{
    Trigger t = trigger;
    if( isDown)
    {
        switch( trigger.type)
        {
            case eKeyTrigger:
                switch( trigger.data1)
                {
                    case SDLK_RETURN:
                        Enter();
                        break;

                    case ESCAPE_KEY:
                        if( !Exit())
                        {
                            turnMenuOff();
                        }
                        break;

                    case SDLK_UP:
                        Up();
                        break;
                    case SDLK_DOWN:
                        Down();
                        break;

                    case SDLK_TAB:
                        if (trigger.data2 & KMOD_SHIFT) {
                            Up();
                        } else {
                            Down();
                        }
                        break;

                    case SDLK_F6:
                        VideoBaseS::instance()->takeSnapshot();
                        break;

                    default:
                        break;
                }
                break;

            case eButtonTrigger:
#ifdef IPHONE
                if( trigger.data1 == 1)
                {
                    _mouseX = trigger.fData1*1000/480;
                    _mouseY = 750-trigger.fData2*750/320;
                    Selectable::reset();
                    _currentSelectable = _activeSelectables.end();
                    activateSelectableUnderMouse();
                }
                else
                {
                    if( !Exit())
                    {
                        turnMenuOff();
                    }
                }
#endif
                break;

            case eMotionTrigger:
                {
                    //_prevMouseX = _mouseX;
                    //_prevMouseY = _mouseY;

#ifdef IPHONE
                    _mouseX = trigger.fData1*1000/480;
                    _mouseY = 750-trigger.fData2*750/320;
                    Clamp( _mouseX, 0.0f, 1000.0);
                    Clamp( _mouseY, 0.0f, 750.0);
#else
                    VideoBase &video = *VideoBaseS::instance();

                    _mouseX += (trigger.fData1*10.0f);
                    _mouseY += (trigger.fData2*10.0f);
                    Clamp( _mouseX, 0.0f, (750.0*(float)video.getWidth()) / (float)video.getHeight());
                    Clamp( _mouseY, 0.0f, 750.0);
#endif
                    activateSelectableUnderMouse();
                }
                break;

            default:
                break;
        }
    }

    //put the absolute mouse position in to trigger
    t.fData1 = _mouseX;
    t.fData2 = _mouseY;

    if( _currentSelectable != _activeSelectables.end())
    {
        (*_currentSelectable)->input( t, isDown, _boardOffset);
    }
}

void MenuManager::activateSelectableUnderMouse( void)
{
    list<Selectable*>::iterator i;
    bool foundSelectable = false;
    for( i=_activeSelectables.begin(); i!=_activeSelectables.end(); i++)
    {
        Selectable *sel = *i;
        if( !sel)
        {
            LOG_ERROR << "Selectable is 0 !!!" << endl;
            continue;
        }
        const BoundingBox &r = sel->getInputBox();
        if( (_mouseX >= (r.min.x+_boardOffset.x)) &&
           (_mouseX <= (r.max.x+_boardOffset.x)) &&
           (_mouseY >= (r.min.y+_boardOffset.y)) &&
           (_mouseY <= (r.max.y+_boardOffset.y)) )
        {
            sel->activate();
            foundSelectable = true;
            break;
        }
    }
    if( !foundSelectable)
    {
        _activeSelectables.front()->activate();
    }
}

void MenuManager::Down( void)
{
    XTRACE();
    if( _currentSelectable == _activeSelectables.end()) return;

    _currentSelectable++;
    if( _currentSelectable == _activeSelectables.end())
    {
        _currentSelectable = _activeSelectables.begin();
    }

    (*_currentSelectable)->activate();
}

void MenuManager::Up( void)
{
    XTRACE();
    if( _currentSelectable == _activeSelectables.end()) return;

    if( _currentSelectable == _activeSelectables.begin())
    {
        _currentSelectable = _activeSelectables.end();
    }
    _currentSelectable--;

    (*_currentSelectable)->activate();
}

void MenuManager::Goto( Selectable *s)
{
    list<Selectable*>::iterator i;
    for( i=_activeSelectables.begin(); i!=_activeSelectables.end(); i++)
    {
        if( (*i) == s)
        {
//            LOG_INFO << "Goto found match" << endl;
            break;
        }
    }
    _currentSelectable = i;
}

void MenuManager::Enter( void)
{
    XTRACE();
    if( _currentSelectable == _activeSelectables.end()) return;

    (*_currentSelectable)->select();
}

bool MenuManager::Exit( bool delayed)
{
    XTRACE();
    if( delayed)
    {
        //while iterating over the selectables we dont want to loadMenuLevel
        _delayedExit = true;
        return true;
    }
    if( _currentMenu != _topMenu)
    {
        _currentMenu = _currentMenu->Parent();
        loadMenuLevel();
        AudioS::instance()->playSample( "sounds/beep");
        return true;
    }

    //at the top level menu
    return false;
}

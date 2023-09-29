// Description:
//   Block View
//
// Copyright (C) 2007 Frank Becker
//
#include "BlockView.hpp"

#include "SDL.h"

#include "Trace.hpp"
#include "FPS.hpp"
#include "Config.hpp"
#include "ResourceManager.hpp"
#include "zrwops.hpp"

#include <GL/glew.h>
#include "GLVertexBufferObject.hpp"

#include "FontManager.hpp"
#include "ModelManager.hpp"
#include "BitmapManager.hpp"

#include "BlockModel.hpp"
#include "Constants.hpp"
#include "GameState.hpp"
#include "ScoreKeeper.hpp"

#include "ParticleGroupManager.hpp"

#include "gl3/Program.hpp"
#include "gl3/Shader.hpp"
#include "gl3/Buffer.hpp"
#include "gl3/VertexArray.hpp"
#include "gl3/ProgramManager.hpp"
#include "gl3/MatrixStack.hpp"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

const float BLOCKROTSPEED = 2.0f;
const float DEFAULT_ROTATION_SPEED = 7.0f * GAME_STEP_SCALE;
const int DEFAULT_MOVE_STEPS = 10;

BlockView::BlockView(BlockModel& model) :
    _model(model),
    _showNextBlock(false),
    _showBlockIndicator(false),
    _blockAngle(0.0),
    _blockFace(0),
    _rotationSpeed(DEFAULT_ROTATION_SPEED),
    _moveSteps(DEFAULT_MOVE_STEPS),
    _numSteps(0),
    _shaftVerts(0),
    _shaftNormals(0),
    _shaftVindices(0),
    _shaftVao(0) {
    XTRACE();
    resetRotations();
}

BlockView::~BlockView() {
    XTRACE();
    delete _blockFace;
    delete _shaftVerts;
    delete _shaftNormals;
    delete _shaftVindices;
    delete _shaftVao;
    VideoBaseS::cleanup();
}

bool BlockView::init(void) {
    if (!VideoBaseS::instance()->init()) {
        return false;
    }

    initGL3Test();

    //set title and icon name
    //SDL_WM_SetCaption( "Shaaft OpenGL", "Shaaft GL" ); -- SDL1

    _font = FontManagerS::instance()->getFont("bitmaps/menuWhite");
    if (!_font) {
        LOG_ERROR << "Unable to get font... (menuWhite)" << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    _fineFont = FontManagerS::instance()->getFont("bitmaps/arial-small");
    if (!_fineFont) {
        LOG_ERROR << "Unable to get font... (arial-small)" << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    GLBitmapCollection* blackBox = BitmapManagerS::instance()->getBitmap("bitmaps/blackBox");
    if (!blackBox) {
        LOG_ERROR << "Unable to load blackBox." << endl;
        return false;
    }
    _blackBox = blackBox->getIndex("BlackBox");
    _pausedBox = blackBox->getIndex("Paused");

    GLBitmapCollection* scoreBoard = BitmapManagerS::instance()->getBitmap("bitmaps/scoreBoard");
    if (!blackBox) {
        LOG_ERROR << "Unable to load scoreBoard." << endl;
        return false;
    }
    _scoreBoard = scoreBoard->getIndex("ScoreBoard");
    _moo = scoreBoard->getIndex("Moo");
    _mooChoo = scoreBoard->getIndex("MooChoo");
    _target = scoreBoard->getIndex("Target");

    _cube = ModelManagerS::instance()->getModel("models/Cube");
    if (!_cube) {
        LOG_ERROR << "Cube.model not found." << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }

    _indicator = ModelManagerS::instance()->getModel("models/Indicator");
    if (!_indicator) {
        LOG_ERROR << "Indicator.model not found." << endl;
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        return false;
    }
#if 0
    std::unique_ptr<ziStream> bminfile1P(ResourceManagerS::instance()->getInputStream("bitmaps/biohazard.png"));
    if( ! bminfile1P)
    {
        LOG_ERROR << "biohazard.png not found." << endl;
        SDL_QuitSubSystem( SDL_INIT_VIDEO);
        return false;
    }

    ziStream &bminfile1 = *bminfile1P;
    SDL_RWops *src = RWops_from_ziStream( bminfile1);
    SDL_Surface *img1 = IMG_LoadPNG_RW( src);
    _blockFace = new GLTexture( GL_TEXTURE_2D, img1, false);
    SDL_FreeRW(src);
#endif
    VideoBaseS::instance()->registerResolutionObserver(this);

    return true;
}

void BlockView::resolutionChanged(int /*w*/, int /*h*/) {
#if 0
    _blockFace->reload();
#endif
    ProgramManagerS::instance()->reset();
    initGL3Test();
}

void BlockView::update(void) {
    if (GameState::isAlive) {
        _prevOffset = _currentOffset;
        _prev2Angle = _currentAngle;

        if ((_currentAngle + _rotationSpeed) < _targetAngle) {
            _currentAngle += _rotationSpeed;
        } else {
            _currentAngle = _targetAngle;
        }

        if (_numSteps) {
            _currentOffset = _currentOffset + _delta;
            _numSteps--;
        }

        Point3Di& offset = _model.getBlockOffset();
        if (_oldOffset != offset) {
            _targetOffset = offset;  // Point3D = Point3Di
            _oldOffset = offset;

            _delta = _targetOffset - _currentOffset;
            _delta = _delta / (float)_moveSteps;
            _numSteps = _moveSteps;
        }
    }
    _blockAngle += BLOCKROTSPEED;
}

void BlockView::updateSettings(void) {
    float rotSpeed = 7.0;
    ConfigS::instance()->getFloat("rotationSpeed", rotSpeed);
    _rotationSpeed = 5.0f + rotSpeed * 1.5f;

    int moveSpeed = 5;
    ConfigS::instance()->getInteger("moveSpeed", moveSpeed);
    _moveSteps = 20 - 2 * moveSpeed;

    ConfigS::instance()->getBoolean("showNextBlock", _showNextBlock);
    ConfigS::instance()->getBoolean("showBlockIndicator", _showBlockIndicator);

    VideoBaseS::instance()->updateSettings();
}

//--------------------------------------------------------------------------------------
//Smoothing
void BlockView::resetRotations(void) {
    _prevAxis = Point3D(1, 0, 0);
    _prevAngle = 0;
    _prevQ.set(_prevAngle, _prevAxis);

    _currentAxis = Point3D(1, 0, 0);
    _currentAngle = 0;
    _targetAngle = 0;

    _prev2Angle = _currentAngle;
}

void BlockView::notifyNewBlock(void) {
    //LOG_INFO << "New Block. Resetting rotations\n";
    resetRotations();

    _oldOffset = _model.getBlockOffset();
    _currentOffset = _oldOffset;  // Point3D = Point3Di
}

#if 0

A sequence of rotations:
  Q(rotate origin to current orientation) * Q(rotate current to final orientation)
  t: partial rotation (angle*t) 0<t<1

1. Rotate request A->B
  Q(1) * Q(AB)t
2. at 'a' rotate request B->C
  Q(Aa) * Q(aBC)t = Q(Aa) * Q(aC)t
3. at 'b' rotate request C->D
  Q(Aab) * Q(bCD)t = Q(Ab) * Q(bD)t


A----a    B
      \
     __b
 __--
D         C

#endif
//TODO: check if slerping is cheaper...

void BlockView::notifyNewRotation(const Quaternion& q) {
    Quaternion tmpQ;

    //convert the ongoing rotation into a quaternion: tmpQ = Q(ab)
    tmpQ.set(_currentAngle, _currentAxis);
    //combine rotations -> Q(Aab) = Q(Ab)
    //prevQ  = Q(Aa)
    _prevQ = tmpQ * _prevQ;
    //get the angle and axis for the new static rotation
    _prevQ.get(_prevAngle, _prevAxis);

    //convert the not yet executed portion of the rotation into a quaternion
    //tmpQ = Q(bC)
    tmpQ.set(_targetAngle - _currentAngle, _currentAxis);
    //combine rotations Q(bC) * Q(CD) -> Q(bCD) = Q(bD)
    _targetQ = q * tmpQ;
    //get the angle and axis for the new ongoing rotation
    _targetQ.get(_targetAngle, _currentAxis);

    //reset the current angle in the ongoing rotation
    _currentAngle = 0;
}

Point3D& BlockView::getInterpolatedOffset(void) {
    float gf = GameState::frameFraction;
    _interpOffset = _prevOffset + (_currentOffset - _prevOffset) * gf;
    return _interpOffset;
}

float BlockView::getInterpolatedAngle(void) {
    float gf = GameState::frameFraction;
    return _prev2Angle + (_currentAngle - _prev2Angle) * gf;
}

void BlockView::initGL3Test() {
    while (!MatrixStack::model.empty()) {
        MatrixStack::model.pop();
    }
    while (!MatrixStack::projection.empty()) {
        MatrixStack::projection.pop();
    }

    MatrixStack::model.push(glm::mat4(1.0f));
    MatrixStack::projection.push(glm::mat4(1.0f));

    Program* progLight = ProgramManagerS::instance()->createProgram("lighting");
    progLight->use();

    _shaftVao = new VertexArray();
    _shaftVao->bind();

    _shaftVerts = new Buffer();
    GLuint shaftVertLoc = 0;
    glEnableVertexAttribArray(shaftVertLoc);
    _shaftVerts->bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(shaftVertLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    _shaftNormals = new Buffer();
    GLuint shaftNormLoc = 1;
    glEnableVertexAttribArray(shaftNormLoc);
    _shaftNormals->bind(GL_ARRAY_BUFFER);
    glVertexAttribPointer(shaftNormLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    _shaftVindices = new Buffer();
    _shaftVindices->bind(GL_ELEMENT_ARRAY_BUFFER);

    _shaftVao->unbind();
    progLight->release();

    Program* progTexture = ProgramManagerS::instance()->createProgram("texture");
    progTexture->use();
    progTexture->release();

    LOG_INFO << "initGL3Test DONE\n";
}

//--------------------------------------------------------------------------------------
//Drawing
void BlockView::draw(void) {
    static double nextTime = Timer::getTime() + 0.5;
    double thisTime = Timer::getTime();
    if (thisTime > nextTime) {
        updateSettings();
        nextTime = thisTime + 0.5;
    }
    VideoBase& video = *VideoBaseS::instance();
    float gf = GameState::frameFraction;

    int blockView = video.getHeight();
    int shaftOffset = (video.getWidth() - blockView * 4 / 3) / 2;
    glViewport(shaftOffset, 0, blockView, blockView);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4& projection = MatrixStack::projection.top();
    projection = glm::mat4(1.0);

    const float fov = 53.13f;
    projection = glm::perspective(glm::radians(fov), 1.0f, 2.0f, 2000.0f);
#if 0
    projection = glm::frustum<float>(
        (3.0/3.0)*(-2.0*tan(fov * M_PI / 360.0)),   //xmin
        (3.0/3.0)*( 2.0*tan(fov * M_PI / 360.0)),   //xmax
        -2.0*tan(fov * M_PI / 360.0),               //ymin
        2.0*tan(fov * M_PI / 360.0),                //ymax
        2.0,                                        //znear
        2000.0                                      //zfar
    );
    LOG_INFO << glm::to_string(projection) << endl;
#endif

    glm::mat4& modelview = MatrixStack::model.top();
    modelview = glm::mat4(1.0);

#if 0
    glEnable( GL_LIGHT0);
    GLfloat light_position[] = { -200.0, 200.0, 400.0, 1.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    static GLfloat ambientL0[]  = {0.0, 0.0, 0.0, 1.0};
    static GLfloat diffuseL0[]  = {0.5, 0.5, 0.5, 1.0};
    static GLfloat specularL0[] = {2.0, 2.0, 2.0, 1.0};
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientL0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseL0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularL0);

    static GLfloat ambientL1[]  = {0.2, 0.2, 0.2, 1.0};
    static GLfloat diffuseL1[]  = {0.8, 0.8, 0.8, 1.0};
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambientL1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseL1);
    glDisable( GL_LIGHT1);

#if 0
    static GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
#endif
    static GLfloat lmodel_twoside[] = {GL_TRUE};
    glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

    static GLfloat front_shininess[] = {10.0};
    static GLfloat front_specular[]  = {0.5, 0.5, 0.5, 1.0};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, front_shininess);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, front_specular);
#endif

    Program* prog = ProgramManagerS::instance()->getProgram("lighting");
    prog->use();  //needed to set uniforms
    GLint projectionLoc = glGetUniformLocation(prog->id(), "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //glm::mat4 viewer = glm::lookAt(
    //    glm::vec3(0,0,2), // Camera location
    //    glm::vec3(0,0,0), // and looks at the origin
    //    glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
    //);

    GLint viewLoc = glGetUniformLocation(prog->id(), "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0)));
    //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewer) );

    bool allowShaftTilting = false;
    ConfigS::instance()->getBoolean("allowShaftTilting", allowShaftTilting);

    if (allowShaftTilting) {
        float interpPitch = GameState::prevShaftPitch + (GameState::shaftPitch - GameState::prevShaftPitch) * gf;
        float interpYaw = GameState::prevShaftYaw + (GameState::shaftYaw - GameState::prevShaftYaw) * gf;

        modelview = glm::translate(modelview, glm::vec3(0, 0, -300));
        modelview = glm::rotate(modelview, glm::radians(interpPitch), glm::vec3(0, 1, 0));
        modelview = glm::rotate(modelview, glm::radians(interpYaw), glm::vec3(1, 0, 0));
        modelview = glm::translate(modelview, glm::vec3(0, 0, 300));
    }

    modelview = glm::translate(modelview, glm::vec3(0, 0, -100));

    int w = _model.getWidth();
    int h = _model.getHeight();
    int d = _model.getDepth();

    _squaresize = 200.0f / (float)(max(w, h));
    _bottom = -120.0f - ((float)d * _squaresize);

    // -- Draw the shaft
    bool drawSolidShaftTiles = true;
    ConfigS::instance()->getBoolean("drawSolidShaftTiles", drawSolidShaftTiles);
    if (drawSolidShaftTiles) {
        drawShaft(false);
    }

    bool drawShaftTileFrame = false;
    ConfigS::instance()->getBoolean("drawShaftTileFrame", drawShaftTileFrame);
    if (drawShaftTileFrame) {
        drawShaft(true);
    }

    // -- Draw all the locked elements
    if ((GameState::isDeveloper) || (GameState::context == Context::eInGame)) {
        drawLockedElements();

        if (_showBlockIndicator) {
            MatrixStack::model.push(MatrixStack::model.top());
            glm::mat4& modelview = MatrixStack::model.top();
            modelview = glm::translate(modelview, glm::vec3(
                        0-_squaresize*(w-1)/2.0f,
                        0-_squaresize*(h-1)/2.0f,
                        0+_bottom + _squaresize/2.0f));

            ElementList& hintElementList = _model.getElementListHint();
            ElementList::iterator i;
            for (i = hintElementList.begin(); i != hintElementList.end(); i++) {
                Point3Di* p = *i;
                drawElement(p, Hint);
            }
            MatrixStack::model.pop();
        }

        if (GameState::isAlive) {
            // -- Draw the block
            MatrixStack::model.push(MatrixStack::model.top());
            glm::mat4& modelview = MatrixStack::model.top();

            Point3D& interpOffset = getInterpolatedOffset();
            modelview = glm::translate(modelview, glm::vec3(
                            interpOffset.x*_squaresize-_squaresize*(w-1)/2.0f,
                            interpOffset.y*_squaresize-_squaresize*(h-1)/2.0f,
                            interpOffset.z*_squaresize+_bottom + _squaresize/2.0f));
            Point3D& currentAxis = getCurrentAxis();
            Point3D& prevAxis = getPrevAxis();

            modelview = glm::rotate(modelview, glm::radians(getInterpolatedAngle()),
                                    glm::vec3(currentAxis.x, currentAxis.y, currentAxis.z));
            modelview =
                glm::rotate(modelview, glm::radians(getPrevAngle()), glm::vec3(prevAxis.x, prevAxis.y, prevAxis.z));

            ElementList& elementList = _model.getElementListNorm();
            ElementList::iterator i;
            for (i = elementList.begin(); i != elementList.end(); i++) {
                Point3Di* p = *i;
                drawElement(p, Normal);
            }

            MatrixStack::model.pop();
        }
    }

    //--- draw indicator and next block on score board

    int statsOffset = video.getWidth() - blockView / 3;
    glViewport(statsOffset, 0, blockView / 3, video.getHeight());

    projection = glm::frustum<double>(
        (1.0/4.0)*(-2.0*tan(fov * M_PI / 360.0)),   //xmin
        (1.0/4.0)*( 2.0*tan(fov * M_PI / 360.0)),   //xmax
        -2.0*tan(fov * M_PI / 360.0),               //ymin
        2.0*tan(fov * M_PI / 360.0),                //ymax
        2.0,                                        //znear
        2000.0                                      //zfar
    );
    {
        Program* prog = ProgramManagerS::instance()->getProgram("lighting");
        prog->use();  //needed to set uniforms
        GLint projectionLoc = glGetUniformLocation(prog->id(), "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint lightPosLoc = glGetUniformLocation(prog->id(), "lightPos");
        vec3f lightPos(30.0, 30.0, 200.0);
        glUniform3fv(lightPosLoc, 1, lightPos.array);
    }

    modelview = glm::mat4(1.0);

    drawIndicator();
    drawNextBlock();

    //---
    float orthoHeight = 750.0;
    float orthoWidth = 250.0;  //(750.0*(float)video.getWidth()) / (float)video.getHeight();

    projection = glm::ortho(-0.5f, orthoWidth + 0.5f, -0.5f, orthoHeight + 0.5f, -1000.0f, 1000.0f);
    modelview = glm::mat4(1.0);

    GLBitmapCollection* scoreBoard = BitmapManagerS::instance()->getBitmap("bitmaps/scoreBoard");
    scoreBoard->bind();

    MatrixStack::model.push(MatrixStack::model.top());
    modelview = MatrixStack::model.top();

    modelview = glm::translate(modelview, glm::vec3(0, 0, -950));
    {
        Program* prog = ProgramManagerS::instance()->getProgram("texture");
        prog->use();  //needed to set uniforms
        GLint modelViewMatrixLoc = glGetUniformLocation(prog->id(), "modelViewMatrix");
        glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(projection * modelview));
    }

    scoreBoard->setColor(1.0, 1.0, 1.0, 1.0);
    scoreBoard->Draw(_scoreBoard, 0, 0, 0.73f, 0.733f);
    modelview = glm::translate(modelview, glm::vec3(0, 0, 1));
    {
        Program* prog = ProgramManagerS::instance()->getProgram("texture");
        prog->use();  //needed to set uniforms
        GLint modelViewMatrixLoc = glGetUniformLocation(prog->id(), "modelViewMatrix");
        glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(projection * modelview));
    }

    int mooImage = _model.HachooInProgress() ? _mooChoo : _moo;
    scoreBoard->Draw(mooImage, 98, 625, 0.73f, 0.733f);

    glDisable(GL_DEPTH_TEST);

    float barLength = (float)(_model.HachooSecsLeft() * (130.0 / _model.HachooDuration()));
    if (barLength > 1e-3) {
        _font->DrawString("2x", 215, 625, 0.5f, 0.5f);
    }

    vec4f v[4] = {
        vec4f(          104.0f, 619, 0, 1),
        vec4f(          104.0f, 624, 0, 1),
        vec4f(barLength+104.0f, 624, 0, 1),
        vec4f(barLength+104.0f, 619, 0, 1),
    };
    GLVBO vbo;
    vbo.setColor(1.0f, 1.0f, 1.0f, 0.4f);
    vbo.DrawQuad(v);

    MatrixStack::model.pop();
    modelview = MatrixStack::model.top();

    float textWidth;
    FPS::Update();
    bool showFPS = false;
    ConfigS::instance()->getBoolean("showFPS", showFPS);
    if (showFPS) {
        textWidth = _fineFont->GetWidth(FPS::GetFPSString(), 0.5);
        _fineFont->setColor(1.0, 1.0, 1.0, 1.0);
        _fineFont->DrawString(FPS::GetFPSString(), orthoWidth - textWidth - 5, orthoHeight - 30, 0.5, 0.5);
    }

    char num[30];

    _font->setColor(1.0, 1.0, 1.0, 1.0);
    sprintf(num, "%d", _model.getLevel());
    _font->DrawString(num, 30, 640, 1.7f, 1.7f);
    sprintf(num, "LEVEL");
    _font->DrawString(num, 25, 693, 0.5f, 0.5f);

    float deltaY1 = 52;
    float deltaY2 = 25;
    float xOffset = 110;
    float yOffset = 387;
    sprintf(num, "%dx%dx%d", _model.getWidth(), _model.getHeight(), _model.getDepth());
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);
    yOffset -= deltaY2;
    _font->DrawString(_model.getBlockset().c_str(), xOffset, yOffset, 0.6f, 0.6f);

    yOffset -= deltaY1;
    sprintf(num, "HiScore:");
    _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);
    yOffset -= deltaY2;
    if (_model.isPracticeMode()) {
        sprintf(num, "Practice");
    } else {
        sprintf(num, "%d", ScoreKeeperS::instance()->getHighScore());
    }
    _font->setColor(1.0, 1.0, 1.0, 1.0);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);

    yOffset -= deltaY1;
    sprintf(num, "Score:");
    _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);
    yOffset -= deltaY2;
    if (_model.isPracticeMode()) {
        sprintf(num, "Practice");
    } else {
        sprintf(num, "%d", ScoreKeeperS::instance()->getCurrentScore());
    }
    _font->setColor(1.0, 1.0, 1.0, 1.0);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);

    yOffset -= deltaY1;
    sprintf(num, "Cubes:");
    _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);
    yOffset -= deltaY2;
    sprintf(num, "%d", _model.getElementCount());
    _font->setColor(1.0, 1.0, 1.0, 1.0);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);

    yOffset -= deltaY1;
    sprintf(num, "Time:");
    _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);
    yOffset -= deltaY2;
    double fsecs = GameState::secondsPlayed;
    int minutes = (int)(fsecs / 60.0);
    int secs = (int)(fsecs) % 60;
    sprintf(num, "%02d:%02d", minutes, secs);
    _font->setColor(1.0, 1.0, 1.0, 1.0);
    _font->DrawString(num, xOffset, yOffset, 0.6f, 0.6f);

    string cr = "Copyright ~ 2023 Frank Becker";
    textWidth = _font->GetWidth(cr.c_str(), 0.4f);
    _font->setColor(1.0, 1.0, 1.0, 0.5);
    _font->DrawString(cr.c_str(), orthoWidth - textWidth - 5, 22.0, 0.4f, 0.5f);

    string gVersion = " " + GAMEVERSION;
    textWidth = _font->GetWidth(gVersion.c_str(), 0.4f);
    _font->DrawString(gVersion.c_str(), orthoWidth - textWidth - 5, 8, 0.4f, 0.4f);

    //restore viewport
    glViewport(0, 0, video.getWidth(), video.getHeight());

    if (GameState::context != Context::eMenu) {
        float orthoWidth2 = (750.0f * (float)video.getWidth()) / (float)video.getHeight();

        projection = glm::ortho(-0.5f, orthoWidth2 + 0.5f, -0.5f, orthoHeight + 0.5f, -1000.0f, 1000.0f);
        modelview = glm::mat4(1.0);

        {
            Program* prog = ProgramManagerS::instance()->getProgram("texture");
            prog->use();  //needed to set uniforms
            GLint modelViewMatrixLoc = glGetUniformLocation(prog->id(), "modelViewMatrix");
            glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(projection * modelview));
        }

        GLBitmapCollection* blackBox = BitmapManagerS::instance()->getBitmap("bitmaps/blackBox");
        blackBox->bind();

        if (GameState::context == Context::ePaused) {
            float boardScale = (float)min(orthoWidth2, orthoHeight) * 0.5f / 512.0f;
            Point2Di boardOffset;
            boardOffset.x = (int)((orthoWidth2 - (float)blackBox->getWidth(_blackBox) * boardScale) / 2.0f);
            boardOffset.y = (int)((orthoHeight - (float)blackBox->getHeight(_blackBox) * boardScale) / 2.0f);

            blackBox->setColor(1.0, 1.0, 1.0, 1.0);
            blackBox->Draw(_pausedBox, (float)boardOffset.x, (float)boardOffset.y, boardScale, boardScale);
        }

        if (!GameState::isAlive) {
            float boardScale = (float)min(orthoWidth2, orthoHeight) * 1.0f / 512.0f;
            Point2Di boardOffset;
            boardOffset.x = (int)((orthoWidth2 - (float)blackBox->getWidth(_blackBox) * boardScale) / 2.0f);
            boardOffset.y = (int)((orthoHeight - (float)blackBox->getHeight(_blackBox) * boardScale) / 2.0f);

            if (ScoreKeeperS::instance()->currentIsTopTen()) {
                boardOffset.y += 180;
            }

            blackBox->setColor(1.0, 1.0, 1.0, 0.7f);
            blackBox->Draw(_blackBox, (float)boardOffset.x, (float)boardOffset.y, boardScale, boardScale);

            if (ScoreKeeperS::instance()->currentIsTopTen()) {
                if (!_textInput.isOn()) {
                    _textInput.turnOn();
                }
                string currentText = _textInput.getText();

                string topTenFinish = "Top Ten Finish!";
                _font->setColor(1.0f, 1.0f, 1.0f, 1.0f);
                _font->DrawString(topTenFinish.c_str(), boardOffset.x + 60.0f, boardOffset.y + 130.0f, 1.2f, 1.2f);

                string pname = "Enter name: ";
                pname += _textInput.getText() + "_";
                _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
                _font->DrawString(pname.c_str(), boardOffset.x + 60.0f, boardOffset.y + 50.0f, 1.2f, 1.2f);

                ScoreKeeperS::instance()->setNameForCurrent(currentText);
            } else {
                string gameOver = "Game Over!";
                if (_model.TimeLimitReached()) {
                    gameOver += " (time limit)";
                }

                _font->setColor(1.0, 1.0, 1.0, 1.0);
                _font->DrawString(gameOver.c_str(), boardOffset.x + 90.0f, boardOffset.y + 130.0f, 1.2f, 1.2f);

                string toMenu = "Press ESC to return to menu";
                textWidth = _font->GetWidth(toMenu.c_str(), 1.0);
                _font->setColor(1.0f, 0.852f, 0.0f, 1.0f);
                _font->DrawString(toMenu.c_str(), boardOffset.x + 90.0f, boardOffset.y + 50.0f, 1.2f, 1.2f);
            }
        } else {
            ParticleGroupManagerS::instance()->draw();
        }
    }
}

void BlockView::drawIndicator(void) {
    MatrixStack::model.push(MatrixStack::model.top());
    glm::mat4& modelview = MatrixStack::model.top();

    float gf = GameState::frameFraction;
    float interpAngle = _blockAngle + (BLOCKROTSPEED * gf);

    modelview = glm::translate(modelview, glm::vec3(-3.65, -19, -50));
    modelview = glm::rotate(modelview, glm::radians(interpAngle), glm::vec3(0, 1, 0));

    for (int i = 0; i < _model.getDepth(); i++) {
        int count = _model.numBlocksInPlane(i);

        if (!count) {
            _indicator->setColor(0.2f, 0.2f, 0.2f, 1.0f);
        } else {
            _indicator->setColor(getColor(i));
        }

        float yOffset = (float)i * 2.5f;

        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        modelview = glm::translate(modelview, glm::vec3(0, yOffset, 0));
        _indicator->draw();
        MatrixStack::model.pop();
    }
    MatrixStack::model.pop();
}

void BlockView::drawNextBlock(void) {
    if (_showNextBlock && (GameState::context == Context::eInGame)) {
        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        ElementList& nextElementList = _model.getElementListNext();
        const Point3D& center = _model.getNextElementCenter();

        float gf = GameState::frameFraction;
        float interpAngle = _blockAngle + (BLOCKROTSPEED * gf);

        modelview = glm::translate(modelview, glm::vec3(1.35, 5.2, -30));
        modelview = glm::rotate(modelview, glm::radians(interpAngle), glm::vec3(0, 1, 0));
        modelview = glm::translate(modelview, glm::vec3(-center.x, -center.y, -center.z));
        modelview = glm::scale(modelview, glm::vec3(0.8, 0.8, 0.8));

        for (ElementList::iterator i = nextElementList.begin(); i != nextElementList.end(); i++) {
            Point3Di* p = *i;
            drawElement(p, Lookahead);
        }
        MatrixStack::model.pop();
    }
}

void BlockView::drawShaft(bool drawLines) {
    int w = _model.getWidth();
    int h = _model.getHeight();
    int d = _model.getDepth();

    float tilesize = _squaresize * 0.80f;
    float halfgapsize = (_squaresize - tilesize) / 2.0f;

    glDisable(GL_DEPTH_TEST);

    MatrixStack::model.push(MatrixStack::model.top());
    glm::mat4& modelview = MatrixStack::model.top();

    modelview = glm::translate(modelview, glm::vec3(0, 0, _bottom));

    Program* prog = ProgramManagerS::instance()->getProgram("lighting");
    prog->use();  //needed to set uniforms
    GLint modelLoc = glGetUniformLocation(prog->id(), "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelview));

    int drawType;
    if (drawLines) {
        drawType = GL_LINE_LOOP;
        //glLineWidth(3.0);
    } else {
        drawType = GL_TRIANGLE_FAN;
    }

    GLint lightPosLoc = glGetUniformLocation(prog->id(), "lightPos");
    vec3f lightPos(-600.0, 600.0, 400.0);
    glUniform3fv(lightPosLoc, 1, lightPos.array);

    GLint viewPosLoc = glGetUniformLocation(prog->id(), "viewPos");
    vec3f viewPos(0.0, 0.0, 0.0);
    glUniform3fv(viewPosLoc, 1, viewPos.array);

    GLint lightColorLoc = glGetUniformLocation(prog->id(), "lightColor");
    vec3f lightColor(1.0, 1.0, 1.0);
    glUniform3fv(lightColorLoc, 1, lightColor.array);

    GLint objectColorLoc = glGetUniformLocation(prog->id(), "objectColor");
    vec4f tileColor(0.0, 1.0, 0.0, 0.5);
    glUniform4fv(objectColorLoc, 1, tileColor.array);

    _shaftVao->bind();

    for (int x = 0; x < w; x++) {
        float xp = x * _squaresize - _squaresize * w / 2.0f + halfgapsize;
        for (int y = 0; y < h; y++) {
            float yp = y * _squaresize - _squaresize * h / 2.0f + halfgapsize;
            vec3f v[4] = {
                vec3f(  xp         , yp         , 0),
                vec3f(  xp+tilesize, yp         , 0),
                vec3f(  xp+tilesize, yp+tilesize, 0),
                vec3f(  xp         , yp+tilesize, 0),
            };
            vec3f n[4] = {
                vec3f(0, 0, 1),
                vec3f(0, 0, 1),
                vec3f(0, 0, 1),
                vec3f(0, 0, 1),
            };

            _shaftVerts->bind(GL_ARRAY_BUFFER);
            _shaftVerts->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), v, GL_STATIC_DRAW);

            _shaftNormals->bind(GL_ARRAY_BUFFER);
            _shaftNormals->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), n, GL_STATIC_DRAW);

            glDrawArrays(drawType, 0, 4);
        }
    }

    for (int x = 0; x < w; x++) {
        float xp = x * _squaresize - _squaresize * w / 2.0f + halfgapsize;
        float yp = _squaresize * h / 2.0f;
        for (int z = 0; z < d; z++) {
            float zp = z * _squaresize;

            vec3f v[4] = {
                vec3f(  xp+tilesize,-yp, zp),
                vec3f(  xp         ,-yp, zp),
                vec3f(  xp         ,-yp, zp+tilesize),
                vec3f(  xp+tilesize,-yp, zp+tilesize),
            };
            vec3f n[4] = {
                vec3f(0, 1, 0),
                vec3f(0, 1, 0),
                vec3f(0, 1, 0),
                vec3f(0, 1, 0),
            };
            _shaftVerts->bind(GL_ARRAY_BUFFER);
            _shaftVerts->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), v, GL_STATIC_DRAW);

            _shaftNormals->bind(GL_ARRAY_BUFFER);
            _shaftNormals->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), n, GL_STATIC_DRAW);

            glDrawArrays(drawType, 0, 4);

            vec3f v2[4] = {
                vec3f(  xp+tilesize, yp, zp+tilesize),
                vec3f(  xp         , yp, zp+tilesize),
                vec3f(  xp         , yp, zp),
                vec3f(  xp+tilesize, yp, zp),
            };
            vec3f n2[4] = {
                vec3f(0, -1, 0),
                vec3f(0, -1, 0),
                vec3f(0, -1, 0),
                vec3f(0, -1, 0),
            };

            _shaftVerts->bind(GL_ARRAY_BUFFER);
            _shaftVerts->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), v2, GL_STATIC_DRAW);

            _shaftNormals->bind(GL_ARRAY_BUFFER);
            _shaftNormals->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), n2, GL_STATIC_DRAW);

            glDrawArrays(drawType, 0, 4);
        }
    }

    for (int y = 0; y < h; y++) {
        float xp = _squaresize * w / 2.0f;
        float yp = y * _squaresize - _squaresize * h / 2.0f + halfgapsize;
        for (int z = 0; z < d; z++) {
            float zp = z * _squaresize;

            vec3f v[4] = {
                vec3f(-xp, yp         , zp),
                vec3f(-xp, yp+tilesize, zp),
                vec3f(-xp, yp+tilesize, zp+tilesize),
                vec3f(-xp, yp         , zp+tilesize),
            };
            vec3f n[4] = {
                vec3f(1, 0, 0),
                vec3f(1, 0, 0),
                vec3f(1, 0, 0),
                vec3f(1, 0, 0),
            };

            _shaftVerts->bind(GL_ARRAY_BUFFER);
            _shaftVerts->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), v, GL_STATIC_DRAW);

            _shaftNormals->bind(GL_ARRAY_BUFFER);
            _shaftNormals->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), n, GL_STATIC_DRAW);

            glDrawArrays(drawType, 0, 4);

            vec3f v2[4] = {
                vec3f( xp, yp         , zp+tilesize),
                vec3f( xp, yp+tilesize, zp+tilesize),
                vec3f( xp, yp+tilesize, zp),
                vec3f( xp, yp         , zp),
            };
            vec3f n2[4] = {
                vec3f(-1, 0, 0),
                vec3f(-1, 0, 0),
                vec3f(-1, 0, 0),
                vec3f(-1, 0, 0),
            };

            _shaftVerts->bind(GL_ARRAY_BUFFER);
            _shaftVerts->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), v2, GL_STATIC_DRAW);

            _shaftNormals->bind(GL_ARRAY_BUFFER);
            _shaftNormals->setData(GL_ARRAY_BUFFER, 4 * sizeof(vec3f), n2, GL_STATIC_DRAW);

            glDrawArrays(drawType, 0, 4);
        }
    }
    _shaftVao->unbind();

    MatrixStack::model.pop();

    glEnable(GL_DEPTH_TEST);
}

vec4f BlockView::getColor(int p) {
    switch (p % 7) {
        case 0:
            return vec4f(0.0, 0.0, 1.0, 1.0);
            break;
        case 1:
            return vec4f(0.0, 1.0, 0.0, 1.0);
            break;
        case 2:
            return vec4f(0.0, 1.0, 1.0, 1.0);
            break;
        case 3:
            return vec4f(1.0, 0.0, 0.0, 1.0);
            break;
        case 4:
            return vec4f(1.0, 0.0, 1.0, 1.0);
            break;
        case 5:
            return vec4f(1.0, 0.7f, 0.0, 1.0);
            break;
        case 6:
            return vec4f(0.5, 0.5, 0.5, 1.0);
            break;
    }
    return vec4f(0.0, 0.0, 0.0, 1.0);
}

void BlockView::drawLockedElements(void) {
    MatrixStack::model.push(MatrixStack::model.top());
    glm::mat4& modelview = MatrixStack::model.top();

    int w = _model.getWidth();
    int h = _model.getHeight();

    modelview = glm::translate(modelview, glm::vec3(
                    0-_squaresize*(w-1)/2.0f,
                    0-_squaresize*(h-1)/2.0f,
                    0+_bottom + _squaresize/2.0f));
    ElementList& lockedElementList = _model.getLockedElementList();
    ElementList::iterator i;
    for (i = lockedElementList.begin(); i != lockedElementList.end(); i++) {
        Point3Di* p = *i;
        drawElement(p, Locked);
    }

    MatrixStack::model.pop();
}

void BlockView::drawElement(Point3Di* p, BlockType blockType) {
    float xp = (p->x) * _squaresize;
    float yp = (p->y) * _squaresize;
    float zp = (p->z) * _squaresize;

    float halftilesize = (_squaresize * 0.80f) / 2.0f;

    if (blockType == Lookahead) {
        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        modelview = glm::translate(modelview, glm::vec3(p->x, p->y, p->z));
        modelview = glm::scale(modelview, glm::vec3(0.5, 0.5, 0.5));

        _indicator->setColor(1.0f, 0.852f, 0.0f, 1.0f);
        _indicator->draw();

        MatrixStack::model.pop();
    } else if (blockType == Locked) {
        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        modelview = glm::translate(modelview, glm::vec3(xp, yp, zp));
        modelview = glm::scale(modelview, glm::vec3(_squaresize * 0.5f, _squaresize * 0.5f, _squaresize * 0.5f));

        _indicator->setColor(getColor(p->z));
        _indicator->draw();

        MatrixStack::model.pop();
    }
#if 0
    else if( blockType == 99)
    {
        float halfmtilesize = (_squaresize*0.80f)/2.0f;

        float xpmm = xp-halfmtilesize;
        float xpmp = xp+halfmtilesize;
        float ypmm = yp-halfmtilesize;
        float ypmp = yp+halfmtilesize;
        float zpmm = zp-halfmtilesize;
        float zpmp = zp+halfmtilesize;

        glNormal3f( 0,0,-1);
        GLVBO::DrawQuad(GL_TRIANGLE_FAN,
                        vec3f( xpmm, ypmp, zpmp),
                        vec3f( xpmp, ypmp, zpmp),
                        vec3f( xpmp, ypmm, zpmp),
                        vec3f( xpmm, ypmm, zpmp));

        glNormal3f( 0, 1,0);
        GLVBO::DrawQuad(GL_TRIANGLE_FAN,
                        vec3f( xpmp, ypmm, zpmm),
                        vec3f( xpmm, ypmm, zpmm),
                        vec3f( xpmm, ypmm, zpmp),
                        vec3f( xpmp, ypmm, zpmp));

        glNormal3f( 0,-1,0);
        GLVBO::DrawQuad(GL_TRIANGLE_FAN,
                        vec3f( xpmp, ypmp, zpmp),
                        vec3f( xpmm, ypmp, zpmp),
                        vec3f( xpmm, ypmp, zpmm),
                        vec3f( xpmp, ypmp, zpmm));

        glNormal3f( 1,0,0);
        GLVBO::DrawQuad(GL_TRIANGLE_FAN,
                        vec3f( xpmm, ypmm, zpmm),
                        vec3f( xpmm, ypmp, zpmm),
                        vec3f( xpmm, ypmp, zpmp),
                        vec3f( xpmm, ypmm, zpmp));

        glNormal3f(-1,0,0);
        GLVBO::DrawQuad(GL_TRIANGLE_FAN,
                        vec3f( xpmp, ypmm, zpmp),
                        vec3f( xpmp, ypmp, zpmp),
                        vec3f( xpmp, ypmp, zpmm),
                        vec3f( xpmp, ypmm, zpmm));
    }
#endif
    else if (blockType == Hint) {
        GLBitmapCollection* scoreBoard = BitmapManagerS::instance()->getBitmap("bitmaps/scoreBoard");
        scoreBoard->bind();

        float hintSize = 0.6f;
        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        modelview = glm::translate(modelview, glm::vec3(xp - halftilesize * hintSize, yp - halftilesize * hintSize,
                                                        zp - halftilesize * hintSize));

        Program* prog = ProgramManagerS::instance()->getProgram("texture");
        prog->use();  //needed to set uniforms
        GLint modelViewMatrixLoc = glGetUniformLocation(prog->id(), "modelViewMatrix");
        glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(MatrixStack::projection.top() * modelview));

        scoreBoard->setColor(1.0, 1.0, 1.0, 1.0f);
        scoreBoard->Draw(_target, 0, 0, 0.18f * hintSize, 0.18f * hintSize);

        MatrixStack::model.pop();
    } else {
        MatrixStack::model.push(MatrixStack::model.top());
        glm::mat4& modelview = MatrixStack::model.top();

        modelview = glm::translate(modelview, glm::vec3(xp, yp, zp));
        modelview = glm::scale(modelview, glm::vec3(_squaresize * 0.6f, _squaresize * 0.6f, _squaresize * 0.6f));

        _cube->draw();

        MatrixStack::model.pop();
    }
}

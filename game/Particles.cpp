// Description:
//   All kinds of particles.
//
// Copyright (C) 2008 Frank Becker
//
#include "Particles.hpp"

#include <Trace.hpp>
#include <Constants.hpp>
#include <GameState.hpp>
#include <RandomKnuth.hpp>
#include <ParticleGroup.hpp>
#include <ParticleGroupManager.hpp>
#include <Audio.hpp>
#include <FontManager.hpp>
#include <BitmapManager.hpp>
#include <ScoreKeeper.hpp>

#include "gl3/Program.hpp"
#include "gl3/ProgramManager.hpp"
#include "gl3/MatrixStack.hpp"
#include "glm/ext.hpp"

using namespace std;

static RandomKnuth _random;

void Particles::Initialize( void)
{
    static bool initialized = false;
    if( !initialized)
    {
        new Spark();
        new ScoreHighlight();
        new StatusMessage();

        initialized = true;
    }
}

//------------------------------------------------------------------------------

GLBitmapCollection *BitmapParticleType::_bitmaps = 0;

BitmapParticleType::BitmapParticleType( const string &particleName):
    ParticleType( particleName)
{
    XTRACE();
    LoadBitmaps();
}

BitmapParticleType::~BitmapParticleType()
{
    XTRACE();
}

void BitmapParticleType::LoadBitmaps( void)
{
    XTRACE();
    static bool bitmapsLoaded = false;
    if( !bitmapsLoaded)
    {
        _bitmaps = BitmapManagerS::instance()->getBitmap( "bitmaps/particles");
        if( !_bitmaps)
        {
            LOG_ERROR << "Unable to load particle bitmap" << endl;
        }
        bitmapsLoaded = true;
    }
}

//------------------------------------------------------------------------------

SingleBitmapParticle::SingleBitmapParticle(
    const string &particleName, const char *bitmapName)
    :BitmapParticleType(particleName)
{
    XTRACE();
    string bmName( bitmapName);
    _bmIndex = _bitmaps->getIndex( bmName);

    _bmHalfWidth = (float)(_bitmaps->getWidth( _bmIndex))/2.0f;
    _bmHalfHeight= (float)(_bitmaps->getHeight( _bmIndex))/2.0f;

//if most of the movement is vertical, width represents the radius
//most realisticly (for non square objs).
//        _radius = sqrt(_bmHalfWidth*_bmHalfWidth + _bmHalfHeight*_bmHalfHeight);
    _radius = _bmHalfWidth;// + _bmHalfHeight)/2.0;
}

//------------------------------------------------------------------------------

Spark::Spark( void):SingleBitmapParticle( "Spark", "Spark1")
{
    XTRACE();
}

Spark::~Spark()
{
    XTRACE();
}

void Spark::init( ParticleInfo *p)
{
    XTRACE();
    p->velocity.x =  ((float)(_random.random()&0xff)-127) * 0.01f * GAME_STEP_SCALE;
    p->velocity.y =  ((float)(_random.random()&0xff)-127) * 0.01f * GAME_STEP_SCALE;

    p->extra.x = 0.01f * GAME_STEP_SCALE;
    p->extra.z = 0.4;

    //init previous values for interpolation
    updatePrevs(p);
}

bool Spark::update( ParticleInfo *p)
{
    //    XTRACE();
    //update previous values for interpolation
    updatePrevs(p);

    //p->velocity.y -= 0.8f * GAME_STEP_SCALE;

    p->extra.z -= p->extra.x;
    //if alpha reaches 0, we can die
    if( p->extra.z < 0) return false;

    p->position.x += p->velocity.x;
    p->position.y += p->velocity.y;

    //if( p->position.y < 0) return false;

    return true;
}

void Spark::draw( ParticleInfo *p)
{
    //    XTRACE();
    ParticleInfo pi;
    interpolateOther( p, pi);

    _bitmaps->setColor(1.0, 0.8 - pi.extra.z*2.0,0.0, pi.extra.z);

    bindTexture();
    _bitmaps->DrawC( _bmIndex, pi.position.x, pi.position.y, 1.0, 1.0);
}

//------------------------------------------------------------------------------

StatusMessage::StatusMessage( void):
    ParticleType( "StatusMessage")
{
    XTRACE();
    _smallFont = FontManagerS::instance()->getFont( "bitmaps/arial-small");
    if( !_smallFont)
    {
        LOG_ERROR << "Unable to get font... (arial-small)" << endl;
    }
}

StatusMessage::~StatusMessage()
{
    XTRACE();
}

void StatusMessage::init( ParticleInfo *p)
{
//    XTRACE();
    p->velocity.x = -1.0f* GAME_STEP_SCALE;

    p->extra.x = _smallFont->GetWidth( p->text.c_str(), 0.1f);
    p->position.x = 70.0f;

    LOG_INFO << "StatusMsg = [" << p->text << "] " /*<< p->position.y*/ << endl;

    p->tod = -1;

    //init previous values for interpolation
    updatePrevs(p);
}

bool StatusMessage::update( ParticleInfo *p)
{
//    XTRACE();
    //update previous values for interpolation
    updatePrevs(p);

    p->position.x += p->velocity.x;

    if( p->position.x < -(70.0f+p->extra.x))
    {
        return false;
    }

    return true;
}

void StatusMessage::draw( ParticleInfo *p)
{
//    XTRACE();
    ParticleInfo pi;
    interpolate( p, pi);

    glDisable(GL_DEPTH_TEST);

    MatrixStack::model.push(MatrixStack::model.top());
    glm::mat4 &modelview= MatrixStack::model.top();

    modelview = glm::translate(modelview, glm::vec3(pi.position.x, pi.position.y, pi.position.z));

    _smallFont->setColor( p->color.x, p->color.y, p->color.z, 0.8f);
    _smallFont->DrawString(
            p->text.c_str(), 0 ,0 , p->extra.y, p->extra.z);

    MatrixStack::model.pop();

    glEnable(GL_DEPTH_TEST);
}

//------------------------------------------------------------------------------

ScoreHighlight::ScoreHighlight( void):
    ParticleType( "ScoreHighlight")
{
    _font = FontManagerS::instance()->getFont("bitmaps/arial-small");
    if( !_font)
    {
        LOG_ERROR << "Unable to get font... (arial-small)" << endl;
    }
}

ScoreHighlight::~ScoreHighlight()
{
}

void ScoreHighlight::init( ParticleInfo *p)
{
    int r1 = (int)(_random.random() & 0xff);
    int r2 = (int)(_random.random() & 0x7f);

    p->velocity.x = (float)(r1-128)*0.02f* GAME_STEP_SCALE;
    p->velocity.y = (float)(r2+128)*0.01f* GAME_STEP_SCALE;
    p->velocity.z = 0;//(float)((_random.random()&0xff)-128)*0.02f* GAME_STEP_SCALE;


    if( p->color.z > 0.5) p->velocity.y = -p->velocity.y;

    p->extra.y = 0.05f;
    p->extra.z = 1.0f;

    p->tod = -1;

    //init previous values for interpolation
    updatePrevs(p);
}

bool ScoreHighlight::update( ParticleInfo *p)
{
//    XTRACE();
    //update previous values for interpolation
    updatePrevs(p);

    p->extra.z -= 0.02f * GAME_STEP_SCALE;
    //if alpha reaches 0, we can die
    if( p->extra.z < 0) return false;

    p->extra.y += 0.1f * GAME_STEP_SCALE;

    p->position.x += p->velocity.x;
    p->position.y += p->velocity.y;
    p->position.z += p->velocity.z;

    return true;
}

void ScoreHighlight::draw( ParticleInfo *p)
{
//    XTRACE();
    ParticleInfo pi;
    interpolate( p, pi);

    glDisable(GL_DEPTH_TEST);

    MatrixStack::model.push(MatrixStack::model.top());
    glm::mat4 &modelview= MatrixStack::model.top();
    glm::mat4 &projection= MatrixStack::projection.top();

    pi.position.x -= _font->GetWidth( p->text.c_str(), pi.extra.y) / 2.0;
    pi.position.y -= _font->GetHeight( p->extra.y) / 2.0;

    modelview = glm::translate(modelview, glm::vec3(pi.position.x, pi.position.y, pi.position.z));
    {
        Program *prog = ProgramManagerS::instance()->getProgram("texture");
        prog->use(); //needed to set uniforms
        GLint modelViewMatrixLoc = glGetUniformLocation( prog->id(), "modelViewMatrix");
        glUniformMatrix4fv(modelViewMatrixLoc, 1, GL_FALSE, glm::value_ptr(projection * modelview) );
    }

    _font->setColor( p->color.x, p->color.y, p->color.z, pi.extra.z);
    _font->DrawString( p->text.c_str(), 0, 0, pi.extra.y, pi.extra.y);

    MatrixStack::model.pop();

    glEnable(GL_DEPTH_TEST);
}

//------------------------------------------------------------------------------

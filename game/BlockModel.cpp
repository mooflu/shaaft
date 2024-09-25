// Description:
//   Block Model
//
// Copyright (C) 2003 Frank Becker
//
#include "BlockModel.hpp"

#include <string>
using namespace std;

#include "Trace.hpp"
#include "Point.hpp"
#include "GameState.hpp"
#include "Tokenizer.hpp"
#include "ResourceManager.hpp"
#include "Audio.hpp"
#include "Quaternion.hpp"
#include "ScoreKeeper.hpp"

#include "BlockView.hpp"  //TODO: use a observer callback instead of _view

#ifdef min
#undef min
#endif

#include <algorithm>
#include <memory>
using namespace std;

/*

Terminology used:
Element/Point: The smallest piece.
Block: Made up of a number of elements.
Plane: A Z-slice of elements
Shaft: Like an elevator shaft; the playing area.

TODO:
Bonus blocks:
- point multiplier
- eraser
erases blocks
- filler
fills planes
- crater
block explodes and removes elements

 */

BlockModel::BlockModel(int w, int h, int d, int level, const std::string& blockset, R250& r) :
    _width(w),
    _height(h),
    _depth(d),
    _level(level),
    _blockset(blockset),
    _r250(r),
    _orientation(0, 0, 1),
    _offset(0, 0, d - 1),
    _freefall(false),
    _freefallZ(0),
    _multiplier(0),
    _practiceMode(false),
    _timeLimitReached(false),
    _elementCount(0),
    _hachooInProgress(false),
    _view(0) {}

BlockModel::~BlockModel() {
    cleanup();
}

int BlockModel::getWidth(void) {
    return _width;
}

int BlockModel::getHeight(void) {
    return _height;
}

int BlockModel::getDepth(void) {
    return _depth;
}

int BlockModel::getLevel(void) {
    return _level;
}

int BlockModel::numBlocksInPlane(unsigned int plane) {
    if (plane >= (unsigned int)_depth) {
        return 0;
    }
    return _planesLockCount[plane];
}

void BlockModel::updateNextHachoo(void) {
    _nextHachoo = GameState::stopwatch.getTime() + 60.0 * (float)((_r250.random() % 5) + 1);
}

double BlockModel::HachooSecsLeft(void) {
    if (_nextHachooBonusEnd > GameState::stopwatch.getTime()) {
        return _nextHachooBonusEnd - GameState::stopwatch.getTime();
    }
    return 0.0;
}

double BlockModel::HachooDuration(void) {
    return (_width * _height * 20.0) / 12.0;
}

void BlockModel::clearAndDeleteElementList(ElementList*& elemList, DeleteType deleteType) {
    ElementList::iterator i;

    for (i = elemList->begin(); i != elemList->end(); i++) {
        delete (*i);
    }
    elemList->clear();

    if (deleteType == DeleteElementsAndList) {
        delete elemList;
        elemList = 0;
    }
}

void BlockModel::cleanup(void) {
    delete[] _planes;
    delete[] _planesLockCount;
    delete[] _planesmem;

    clearAndDeleteElementList(_elementList);
    clearAndDeleteElementList(_elementListNorm);
    clearAndDeleteElementList(_elementListHint);
    clearAndDeleteElementList(_tmpElementList);
    clearAndDeleteElementList(_lockedElementList);

    for (unsigned int c = 0; c < _blockList.size(); c++) {
        ElementList* el = _blockList[c].elements;
        clearAndDeleteElementList(el);
    }
    _blockList.clear();
}

bool BlockModel::init(void) {
    LOG_INFO << "BlockModel:"
        << " w=" << _width
        << " h=" << _height
        << " d=" << _depth << endl;

    _planes = new unsigned char*[_depth];
    _planesLockCount = new unsigned char[_depth];
    memset(_planesLockCount, 0, _depth);
    _planesmem = new unsigned char[_depth * _width * _height];
    memset(_planesmem, 0, _depth * _width * _height);

    for (int i = 0; i < _depth; i++) {
        _planes[i] = &(_planesmem[i * _width * _height]);
    }

    _elementList = new ElementList;
    _elementListNorm = new ElementList;
    _elementListHint = new ElementList;
    _tmpElementList = new ElementList;
    _lockedElementList = new ElementList;

    updateDropDelay();
    if (!loadBlocks()) {
        return false;
    }

    _nextBlock = _r250.random() % _blockList.size();

    addBlock();
    verifyAndAdjust();
    updateNextDrop(_dropDelay);

    _nextHachoo = GameState::stopwatch.getTime();  //hachoo at start
    _nextHachooBonusEnd = 0.0;
    _hachooInProgress = false;

    return true;
}

bool BlockModel::reset(int w, int h, int d, int level, const std::string& blockset) {
    cleanup();

    _width = w;
    _height = h;
    _depth = d;
    _level = level;

    _blockset = blockset;

    _orientation = Point3Di(0, 0, 1);
    _offset = Point3Di(0, 0, d - 1);
    _freefall = false;
    _freefallZ = 0;
    _multiplier = 0;
    _elementCount = 0;
    _nextBlock = -1;
    _timeLimitReached = false;

    return init();
}

void BlockModel::addBlock(void) {
    if (!_blockList.size()) {
        return;
    }

    clearAndDeleteElementList(_elementList, OnlyDeleteElements);
    clearAndDeleteElementList(_elementListNorm, OnlyDeleteElements);
    clearAndDeleteElementList(_elementListHint, OnlyDeleteElements);
    clearAndDeleteElementList(_tmpElementList, OnlyDeleteElements);

    ElementList* el = _blockList[_nextBlock].elements;
    _multiplier = _blockList[_nextBlock].multiplier;

    ElementList::iterator i;
    for (i = el->begin(); i != el->end(); i++) {
        Point3Di* p = *i;
        _elementList->insert(_elementList->begin(), new Point3Di(p->x, p->y, p->z));
        _elementListNorm->insert(_elementListNorm->begin(), new Point3Di(p->x, p->y, p->z));
        _elementListHint->insert(_elementListHint->begin(), new Point3Di(p->x, p->y, p->z));
        _tmpElementList->insert(_tmpElementList->begin(), new Point3Di(p->x, p->y, p->z));
    }

    _offset = Point3Di(0, 0, _depth - 1);
    _orientation = Point3Di(0, 0, 1);

    _freefall = false;
    _freefallZ = 0;

    _nextBlock = _r250.random() % _blockList.size();

    if (_view) {
        _view->notifyNewBlock();
    }
}

bool BlockModel::loadBlocks(void) {
    string filename = "blocksets/" + _blockset + ".txt";

    std::unique_ptr<ziStream> infileP(ResourceManagerS::instance()->getInputStream(filename));
    if (!infileP) {
        LOG_ERROR << "Unable to open: [" << filename << "]" << endl;
        return false;
    }
    ziStream& infile = *infileP;

    LOG_INFO << "Loading block info from [" << filename << "]" << endl;

    string line;
    int linecount = 0;
    while (!getline(infile, line).eof()) {
        if (line.length() == 0) {
            continue;
        }

        linecount++;
        //        LOG_INFO << "[" <<  line << "]" << endl;
        if (line[0] == '#') {
            continue;
        }

        Tokenizer t(line);
        string token = t.next();
        if (token == "Elements") {
            int numElems = atoi(t.next().c_str());
            if (!readBlock(infile, numElems, linecount)) {
                LOG_ERROR << "Error reading blocks. Line: " << linecount << endl;
                return false;
            }
        }
    }
    return true;
}

bool BlockModel::readBlock(ziStream& infile, int numElems, int& linecount) {
    ElementList* el = new ElementList;
    bool blockOK = true;

    Point3Di minPos;
    Point3Di maxPos;

    string line;
    for (int i = 0; i < numElems; i++) {
        if (getline(infile, line).eof()) {
            blockOK = false;
            break;
        }
        linecount++;

        Tokenizer t(line);

        Point3Di p;
        p.x = atoi(t.next().c_str());
        p.y = atoi(t.next().c_str());
        p.z = atoi(t.next().c_str());

        if (t.tokensReturned() != 3) {
            blockOK = false;
            break;
        }

        if (i == 0) {
            minPos = p;
            maxPos = p;
        } else {
            if (p.x < minPos.x) {
                minPos.x = p.x;
            }
            if (p.x > maxPos.x) {
                maxPos.x = p.x;
            }
            if (p.y < minPos.y) {
                minPos.y = p.y;
            }
            if (p.y > maxPos.y) {
                maxPos.y = p.y;
            }
            if (p.z < minPos.z) {
                minPos.z = p.z;
            }
            if (p.z > maxPos.z) {
                maxPos.z = p.z;
            }
        }

        Point3Di* np = new Point3Di(p.x, p.y, p.z);
        el->insert(el->begin(), np);
        //                LOG_INFO << "[" << p.x << "," << p.y << "," << p.z << "]" << endl;
    }

    //check if the block doesn't fit in the shaft
    int maxSize = std::min(std::min(_width, _height), _depth);
    int dx = (maxPos.x - minPos.x + 1);
    int dy = (maxPos.y - minPos.y + 1);
    int dz = (maxPos.z - minPos.z + 1);
    if ((dx > maxSize) || (dy > maxSize) || (dz > maxSize)) {
        LOG_INFO << "Excluding big block (line:" << linecount << ")" << endl;
        blockOK = false;
    }

    if (blockOK) {
        BlockInfo bi;
        bi.elements = el;
        bi.multiplier = dx * dy * dz;
        bi.center = Point3D((maxPos.x + minPos.x) / 2.0f, (maxPos.y + minPos.y) / 2.0f, (maxPos.z + minPos.z) / 2.0f);
        _blockList.push_back(bi);
    } else {
        clearAndDeleteElementList(el);
    }

    return true;
}

void BlockModel::updateDropDelay(void) {
#ifdef IPHONE
    float dropDelay[] = {
        9.000f,  // lvl 0 (dummy)
        5.000f,  // lvl 1
        3.700f,  // lvl 2
        2.600f,  // lvl 3
        1.900f,  // lvl 4
        1.400f,  // lvl 5
        1.000f,  // lvl 6
        0.750f,  // lvl 7
        0.600f,  // lvl 8
        0.500f,  // lvl 9
    };
#else
    float dropDelay[] = {
        9.000f,  // lvl 0 (dummy)
        6.000f,  // lvl 1
        4.000f,  // lvl 2
        2.667f,  // lvl 3
        1.778f,  // lvl 4
        1.185f,  // lvl 5
        0.790f,  // lvl 6
        0.527f,  // lvl 7
        0.351f,  // lvl 8
        0.234f,  // lvl 9
    };
#endif
    _dropDelay = dropDelay[_level];
}

void BlockModel::moveBlock(Direction dir) {
    switch (dir) {
        case eLeft:
            _offset.x--;
            break;
        case eRight:
            _offset.x++;
            break;
        case eUp:
            _offset.y++;
            break;
        case eDown:
            _offset.y--;
            break;
        case eOut:
            _offset.z++;
            break;
        case eIn:
            if (!_freefall) {
                //LOG_INFO << "Falling at " << _offset.z << "\n";
                _freefall = true;
                _freefallZ = _offset.z;
            }
            break;
        default:
            break;
    }

    if (!verifyAndAdjust()) {
        //failed verify, undo move
        switch (dir) {
            case eLeft:
                _offset.x++;
                break;
            case eRight:
                _offset.x--;
                break;
            case eUp:
                _offset.y--;
                break;
            case eDown:
                _offset.y++;
                break;
            case eOut:
                _offset.z--;
                break;
            case eIn:
            default:
                break;
        }
    }
}

void BlockModel::rotateBlock(const Point3Di& r1, const Point3Di& r2, const Point3Di& r3, const Point3Di& axis) {
    ElementList::iterator i;
    ElementList::iterator j;
    for (i = _elementList->begin(), j = _tmpElementList->begin(); i != _elementList->end(); i++, j++) {
        Point3Di p1 = **i;   //current position
        Point3Di& p2 = **j;  //new position

        p2.x = p1.x * r1.x + p1.y * r1.y + p1.z * r1.z;
        p2.y = p1.x * r2.x + p1.y * r2.y + p1.z * r2.z;
        p2.z = p1.x * r3.x + p1.y * r3.y + p1.z * r3.z;
    }

    ElementList* tmp = _elementList;
    _elementList = _tmpElementList;

    if (verifyAndAdjust()) {
        //accept new element list and complete ptr swap
        _tmpElementList = tmp;

        Quaternion q;
        q.set(90.0f, Point3D((float)axis.x, (float)axis.y, (float)axis.z));
        _view->notifyNewRotation(q);
    } else {
        //reject and restore original ptr
        _elementList = tmp;
    }
}

bool BlockModel::verifyAndAdjust(void) {
    int minx = 0;
    int maxx = 0;
    int miny = 0;
    int maxy = 0;

    ElementList::iterator i;
    for (i = _elementList->begin(); i != _elementList->end(); i++) {
        Point3Di* p = *i;
        Point3Di a = *p + _offset;

        if (-a.x > minx) {
            minx = -a.x;
        }
        if (a.x >= (_width + maxx)) {
            maxx = (a.x - _width + 1);
        }
        if (-a.y > miny) {
            miny = -a.y;
        }
        if (a.y >= (_height + maxy)) {
            maxy = (a.y - _height + 1);
        }

        if (a.z < 0) {
            return false;
        }
    }

    Point3Di origOffset = _offset;
    if ((minx + miny + maxx + maxy) > 0) {
        _offset.x += minx;
        _offset.x -= maxx;
        _offset.y += miny;
        _offset.y -= maxy;
    }

    for (i = _elementList->begin(); i != _elementList->end(); i++) {
        Point3Di* p = *i;
        Point3Di a = *p + _offset;

        if (elementLocked(a)) {
            _offset = origOffset;
            return false;
        }
    }

    return true;
}

bool BlockModel::canDrop(void) {
    ElementList::iterator i;
    for (i = _elementList->begin(); i != _elementList->end(); i++) {
        Point3Di* p = *i;
        Point3Di a = *p + _offset;
        a.z--;
        if (a.z < 0) {
            return false;
        }

        if (elementLocked(a)) {
            return false;
        }
    }
    return true;
}

void BlockModel::updateHintList(void) {
    ElementList::iterator i = _elementListHint->begin();
    ElementList::iterator j = _elementList->begin();
    int highest = 0;
    for (; i != _elementListHint->end(); i++, j++) {
        Point3Di checkPos = *(*j) + _offset;
        Point3Di& hint = *(*i);

        while ((checkPos.z >= 0) && (!elementLocked(checkPos))) {
            checkPos.z--;
        }

        checkPos.z++;
        if (checkPos.z > highest) {
            highest = checkPos.z;
        }

        hint = checkPos;
    }
    for (i = _elementListHint->begin(); i != _elementListHint->end(); i++) {
        Point3Di& hint = *(*i);
        hint.z = highest;
    }
}

float BlockModel::getScoreMultiplier(void) {
    double now = GameState::stopwatch.getTime();
    if (_nextHachooBonusEnd > now) {
        return 2.0;
    }
    return 1.0;
}

bool BlockModel::update(void) {
    if (_nextHachoo < GameState::stopwatch.getTime()) {
        if (!_hachooInProgress) {
            _nextHachooBonusEnd = _nextHachoo + HachooDuration();
            AudioS::instance()->playSample("sounds/achoo");
        }
        _hachooInProgress = true;
        if ((_nextHachoo + 0.7) < GameState::stopwatch.getTime()) {
            updateNextHachoo();
            _hachooInProgress = false;
        }
    }

    if (_freefall) {
        if (canDrop()) {
            updateNextDrop(0.5f, true);
            _offset.z--;
        } else {
            _freefall = false;
            if (_practiceMode) {
                _nextDrop = GameState::stopwatch.getTime() + 0.5;
            }
        }
    } else {
        double now = GameState::stopwatch.getTime();
        if (now >= _nextDrop) {
            if (canDrop()) {
                _offset.z--;
            } else {
                //lock block
                ElementList::iterator i;
                int eCount = 0;
                for (i = _elementList->begin(); i != _elementList->end(); i++) {
                    Point3Di* p = *i;
                    Point3Di a = *p + _offset;
                    if (!lockElement(a)) {
                        //failed to lock block, game over!

                        //update time played
                        ScoreKeeperS::instance()->addToCurrentScore(0, 0, (int)GameState::stopwatch.getTime());
                        //push leader board scores to score board list
                        ScoreKeeperS::instance()->updateScoreBoardWithLeaderBoard();
                        return false;
                    }
                    eCount++;
                    if ((_level < 9) && (_elementCount >= (_level * 150))) {
                        _level++;
                        AudioS::instance()->playSample("sounds/blblib");
                        updateDropDelay();
                        LOG_INFO << "New level is " << _level << endl;
                    }
                }
                _elementCount += eCount;

                float scoreToAdd = (eCount + _multiplier + _freefallZ / 2.0f) * _level;
                scoreToAdd *= getScoreMultiplier();
                ScoreKeeperS::instance()->addToCurrentScore((int)scoreToAdd, eCount, (int)GameState::stopwatch.getTime());

                AudioS::instance()->playSample("sounds/katoung");
                checkPlanes();

                addBlock();

                if (!verifyAndAdjust()) {
                    LOG_INFO << "Can't fit new block!" << endl;
                    //update time played
                    ScoreKeeperS::instance()->addToCurrentScore(0, 0, (int)GameState::stopwatch.getTime());
                    //push leader board scores to score board list
                    ScoreKeeperS::instance()->updateScoreBoardWithLeaderBoard();
                    return false;
                }
            }

            updateNextDrop(_dropDelay);
        }

        updateHintList();
    }
    return true;
}

void BlockModel::checkPlanes(void) {
    int planeElems = _width * _height;
    int planeCount = 0;

    for (int d = 0; d < _depth; d++) {
        if (_planesLockCount[d] < planeElems) {
            continue;
        }
        if (_planesLockCount[d] == 0) {
            break;
        }

        planeCount++;

        // collapse
        unsigned char* tmpplane = _planes[d];
        for (int i = d; i < (_depth - 1); i++) {
            _planesLockCount[i] = _planesLockCount[i + 1];
            _planes[i] = _planes[i + 1];
        }
        _planesLockCount[_depth - 1] = 0;
        _planes[_depth - 1] = tmpplane;

        memset(tmpplane, 0, planeElems);

        ElementList::iterator i;
        for (i = _lockedElementList->begin(); i != _lockedElementList->end();) {
            Point3Di* e = *i;
            if (e->z == d) {
                i = _lockedElementList->erase(i);
                delete e;
            } else if (e->z > d) {
                e->z--;
                i++;
            } else {
                i++;
            }
        }

        //we moved everything down, so do this depth again
        d--;
    }

    switch (planeCount) {
        case 0:
            break;

        case 1:
            AudioS::instance()->playSample("sounds/chirp2");
            break;
        case 2:
            AudioS::instance()->playSample("sounds/xdoubleplay");
            break;
        case 3:
            AudioS::instance()->playSample("sounds/xtripleplay");
            break;
        case 4:
            AudioS::instance()->playSample("sounds/xmonsterplay");
            break;

        default:
            AudioS::instance()->playSample("sounds/xrediculous");
            break;
    }

    float scoreToAdd = planeCount * planeCount * _level * 50.f;
    scoreToAdd *= getScoreMultiplier();
    ScoreKeeperS::instance()->addToCurrentScore((int)scoreToAdd);
}

bool BlockModel::lockElement(Point3Di& a) {
    if (a.z >= _depth) {
        LOG_ERROR << "Out of bounds!" << endl;
        return false;
    }

    _planes[a.z][a.y * _width + a.x]++;

    unsigned char val = _planes[a.z][a.y * _width + a.x];
    if (val > 1) {
        LOG_ERROR << "Bad locked element! (" << a.x << "," << a.y << "," << a.z << " = " << (int)val << ")" << endl;
        return false;
    }

    _planesLockCount[a.z]++;
    _lockedElementList->insert(_lockedElementList->begin(), new Point3Di(a.x, a.y, a.z));

    return true;
}

bool BlockModel::elementLocked(Point3Di& a) {
    //elements outside the shaft are never locked
    if (a.z >= _depth) {
        return false;
    }

    unsigned char val = _planes[a.z][a.y * _width + a.x];
    return (val > 0);
}

void BlockModel::updateNextDrop(float delta, bool freeFall) {
    if (_practiceMode && !freeFall) {
        _nextDrop = GameState::stopwatch.getTime() + 60 * 60 * 24 * 100;
    } else {
        _nextDrop = GameState::stopwatch.getTime() + delta;
    }
}

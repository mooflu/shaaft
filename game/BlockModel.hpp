#pragma once
// Description:
//   Block Model
//
// Copyright (C) 2007 Frank Becker
//

#include <string>
#include <list>
#include <vector>

#include "Point.hpp"
#include "R250.hpp"

typedef std::list<Point3Di*> ElementList;

class BlockView;
class ziStream;

class BlockModel {
public:
    enum Direction {
        eLeft,   // -x
        eRight,  // +x

        eDown,  // -y
        eUp,    // +y

        eIn,  // -z
        eOut  // +z
    };

    BlockModel(int w, int h, int d, int level, const std::string& blockset, R250& r);
    ~BlockModel();

    bool init(void);
    bool reset(int w, int h, int d, int level, const std::string& blockset);

    void setPracticeMode(bool isPractice) { _practiceMode = isPractice; }

    bool isPracticeMode(void) { return _practiceMode; }

    bool TimeLimitReached(void) { return _timeLimitReached; }

    int getWidth(void);
    int getHeight(void);
    int getDepth(void);
    int getLevel(void);

    std::string getBlockset(void) { return _blockset; }

    unsigned int getElementCount(void) { return _elementCount; }

    ElementList& getElementListNorm(void) { return *_elementListNorm; }

    ElementList& getElementList(void) { return *_elementList; }

    ElementList& getLockedElementList(void) { return *_lockedElementList; }

    ElementList& getElementListHint(void) { return *_elementListHint; }

    ElementList& getElementListNext(void) { return *_blockList[_nextBlock].elements; }

    const Point3D& getNextElementCenter(void) { return _blockList[_nextBlock].center; }

    Point3Di& getBlockOffset(void) { return _offset; }

    Point3Di& getOrientation(void) { return _orientation; }

    void moveBlock(Direction dir);
    void rotateBlock(const Point3Di& r1, const Point3Di& r2, const Point3Di& r3, const Point3Di& axis);

    void registerView(BlockView* view) { _view = view; }

    int numBlocksInPlane(unsigned int plane);

    bool HachooInProgress(void) { return _hachooInProgress; }

    double HachooSecsLeft(void);
    double HachooDuration(void);

    bool update(void);

protected:
    bool verifyAndAdjust(void);
    bool canDrop(void);
    void checkPlanes(void);

    void updateNextDrop(float delta, bool freeFall = false);
    void updateDropDelay(void);
    void updateHintList(void);

    bool lockElement(Point3Di& a);
    bool elementLocked(Point3Di& a);

private:
    BlockModel(const BlockModel&);
    BlockModel& operator=(const BlockModel&);

    enum DeleteType {
        OnlyDeleteElements,
        DeleteElementsAndList
    };

    void clearAndDeleteElementList(ElementList*& elemList, DeleteType deleteList = DeleteElementsAndList);
    void cleanup(void);
    void updateNextHachoo(void);
    float getScoreMultiplier(void);

    void addBlock(void);
    bool loadBlocks(void);
    bool readBlock(ziStream& infile, int numElems, int& linecount);

    int _width;
    int _height;
    int _depth;
    int _level;

    std::string _blockset;
    R250& _r250;

    double _nextDrop;
    float _dropDelay;

    unsigned char* _planesmem;
    unsigned char** _planes;
    unsigned char* _planesLockCount;

    struct BlockInfo {
        ElementList* elements;
        int multiplier;
        Point3D center;
    };

    typedef std::vector<BlockInfo> BlockInfoVector;
    BlockInfoVector _blockList;

    //index to the next Block
    int _nextBlock;

    //The next three lists contain the block the user adjusts so they all
    //have the same length.
    //element list that remains unmodified and is used for drawing
    ElementList* _elementListNorm;
    //element list that reflects the actual positions
    ElementList* _elementList;
    //element list that contains the proposed new position
    ElementList* _tmpElementList;
    //element list that shows where the elements would land
    ElementList* _elementListHint;

    //elements that have been dropped and now sit at the bottom
    ElementList* _lockedElementList;

    Point3Di _orientation;
    Point3Di _offset;
    bool _freefall;
    int _freefallZ;
    int _multiplier;
    bool _practiceMode;
    bool _timeLimitReached;

    int _elementCount;

    BlockView* _view;

    bool _hachooInProgress;
    double _nextHachoo;
    double _nextHachooBonusEnd;
};

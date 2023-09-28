#pragma once
// Description:
//   Different kinds of selectable factories.
//
// Copyright (C) 2001 Frank Becker
//

#include <string>

#include "hashMap.hpp"
#include "HashString.hpp"
#include "FindHash.hpp"

#include "Trace.hpp"
#include "Point.hpp"
#include "Tokenizer.hpp"
#include "Selectable.hpp"

#include "tinyxml.h"

class SelectableFactory {
public:
    static SelectableFactory* getFactory(const std::string& name);
    static void cleanup(void);

    virtual Selectable* createSelectable(TiXmlNode*) = 0;

protected:
    SelectableFactory(void) {}

    virtual ~SelectableFactory() {}

    static hash_map<const std::string, SelectableFactory*, hash<const std::string>, std::equal_to<const std::string>>
        _sfMap;

    void posToPoint2D(const std::string& pos, Point2D& point);
    std::string getAttribute(const TiXmlElement* elem, std::string attr);
    void getBasics(TiXmlElement* elem, Point2D& pos, std::string& text, std::string& info, bool& enabled);

    void getBasics(TiXmlElement* elem, Point2D& pos, std::string& text, std::string& info, bool& enabled,
                   bool& centered);

private:
    static bool _initialized;
};

class ActionItemFactory : public SelectableFactory {
public:
    ActionItemFactory(void);
    virtual ~ActionItemFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class MenuItemFactory : public SelectableFactory {
public:
    MenuItemFactory(void);
    virtual ~MenuItemFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class TextItemFactory : public SelectableFactory {
public:
    TextItemFactory(void);
    virtual ~TextItemFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class ImageItemFactory : public SelectableFactory {
public:
    ImageItemFactory(void);
    virtual ~ImageItemFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class BoolFactory : public SelectableFactory {
public:
    BoolFactory(void);
    virtual ~BoolFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class EnumFactory : public SelectableFactory {
public:
    EnumFactory(void);
    virtual ~EnumFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class FloatFactory : public SelectableFactory {
public:
    FloatFactory(void);
    virtual ~FloatFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class LeaderBoardFactory : public SelectableFactory {
public:
    LeaderBoardFactory(void);
    virtual ~LeaderBoardFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class ResolutionFactory : public SelectableFactory {
public:
    ResolutionFactory(void);
    virtual ~ResolutionFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

class BindKeyFactory : public SelectableFactory {
public:
    BindKeyFactory(void);
    virtual ~BindKeyFactory();
    virtual Selectable* createSelectable(TiXmlNode* node);
};

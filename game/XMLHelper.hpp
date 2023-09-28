#pragma once
// Description:
//   XML helper to help load xml docs.
//
// Copyright (C) 2001 Frank Becker
//
#include <string>
#include "tinyxml.h"

class XMLHelper {
public:
    static TiXmlDocument* load(const std::string& filename);
};

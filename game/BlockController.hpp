#pragma once
// Description:
//   Controller
//
// Copyright (C) 2003 Frank Becker
//

#include "EventWatcher.hpp"
#include "EventInjector.hpp"

#include "BlockModel.hpp"
#include "BlockView.hpp"

class BlockController {
public:
    BlockController(BlockModel& model);
    ~BlockController();

private:
    BlockController(const BlockController&);
    BlockController& operator=(const BlockController&);

    BlockModel& _model;
};

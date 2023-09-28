#pragma once
// Description:
//   Base for callbacks.
//
// Copyright (C) 2007 Frank Becker
//

#include <string>
#include "Input.hpp"

class Callback {
public:
    Callback(const std::string& actionName, const std::string& defaultTrigger);
    virtual ~Callback();

    virtual void performAction(Trigger& trigger, bool isDown) = 0;

    std::string& getActionName(void) { return _actionName; };

    std::string& getDefaultTriggerName(void) { return _defaultTrigger; };

protected:
    std::string _actionName;
    std::string _defaultTrigger;

private:
    Callback(const Callback&);
    Callback& operator=(const Callback&);
};

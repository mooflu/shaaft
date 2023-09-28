#pragma once
// Description:
//   Interface for interceptors.
//
// Copyright (C) 2007 Frank Becker
//

#include "Trigger.hpp"

class InterceptorI {
public:
    virtual void input(const Trigger& trigger, const bool& isDown) = 0;

    virtual ~InterceptorI() {}
};

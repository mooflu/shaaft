#pragma once
// Description:
//   Structure for input triggers.
//
// Copyright (C) 2007 Frank Becker
//
#include <string>

enum TriggerTypeEnum {
    eUnknownTrigger,
    eKeyTrigger,
    eButtonTrigger,
    eMotionTrigger,
    eTextInputTrigger,
};

struct Trigger {
    TriggerTypeEnum type;
    int data1;
    int data2;
    int data3;

    //just for mouse smoothing for now...
    float fData1;
    float fData2;

    // text input
    std::string text;

    bool operator==(const Trigger& t) const {
        if ((type == eMotionTrigger) && (type == t.type)) {
            return true;
        }
        //ignore modifiers (data2) and keycode (data1)
        return ((type == t.type) && (data3 == t.data3));  //data3 is scancode
    }
};

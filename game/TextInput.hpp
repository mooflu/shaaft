#pragma once
// Description:
//   Get a line of text.
//
// Copyright (C) 2007 Frank Becker
//

#include <string>
#include <InterceptorI.hpp>

class TextInput: public InterceptorI
{
public:
    TextInput( unsigned int maxLength=15);
    virtual ~TextInput();

    virtual void input( const Trigger &trigger, const bool &isDown);
    void turnOn( void);
    void turnOff( void);

    bool isOn( void)
    {
	return _isOn;
    }

    const std::string &getText( void);

private:
    TextInput( const TextInput&);
    TextInput &operator=(const TextInput&);

    unsigned int _maxLen;
    std::string _line;
    bool _isOn;
};

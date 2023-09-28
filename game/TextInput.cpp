// Description:
//   Get a line of text.
//
// Copyright (C) 2007 Frank Becker
//
#include "TextInput.hpp"

#include "Trace.hpp"
#include "TextInput.hpp"
#include "Input.hpp"
#include "Config.hpp"

#include <ctype.h>

#include <sstream>
#include "ScoreKeeper.hpp"
#include "GameState.hpp"
#include "OnlineScore.hpp"
#include "RandomKnuth.hpp"
#include "StringUtils.hpp"

#include "MenuManager.hpp"

static RandomKnuth _random;

#ifdef IPHONE
#import "UIKit/UIKit.h"
static UITextField* playerName = 0;
static bool editEnded = false;

@interface MyDelegate : NSObject <UITextFieldDelegate> {
}
@end

@implementation MyDelegate

- (id)init {
    //    NSLog(@"MyDelegate::init");
    return self;
}

- (void)textFieldDidBeginEditing:(UITextField*)textField {
    //    NSLog(@"textFieldDidBeginEditing");
}

- (BOOL)textFieldShouldEndEditing:(UITextField*)textField {
    //    NSLog(@"textFieldShouldEndEditing");
    return YES;
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField {
    //    NSLog(@"textFieldShouldReturn");
    editEnded = true;
    return YES;
}

@end
#endif

TextInput::TextInput(unsigned int maxLength) :
    _maxLen(maxLength),
    _line(""),
    _isOn(false) {
    XTRACE();
}

TextInput::~TextInput() {
    XTRACE();
}

void TextInput::input(const Trigger& trigger, const bool& isDown) {
#ifndef IPHONE
    XTRACE();
    switch (trigger.type) {
        case eKeyTrigger:
            if (!isDown) {
                return;
            }

            switch (trigger.data1) {
                case SDLK_ESCAPE:
                case SDLK_RETURN:
                    turnOff();
                    break;

                case SDLK_DELETE:
                case SDLK_BACKSPACE:
                    if (_line.length() > 0) {
                        _line.erase(_line.length() - 1, 1);
                    }
                    break;

                default:
                    break;
            }
            break;

        case eTextInputTrigger:
            if (_line.length() <= _maxLen) {
                //LOG_INFO << "TextInput::input TEXT " << trigger.text << "\n";
                _line += trigger.text;
            }
            break;

        default:
            break;
    }
#endif
}

void TextInput::turnOn(void) {
    XTRACE();
    LOG_INFO << "TextInput::turnOn\n";
#ifdef IPHONE
    if (!playerName) {
        playerName = [[UITextField alloc] initWithFrame:CGRectMake(0.0, 0.0, 5.0, 5.0)];
        [playerName setEnablesReturnKeyAutomatically:YES];
        [playerName setAutocorrectionType:UITextAutocorrectionTypeNo];
        [playerName setDelegate:[[MyDelegate alloc] init]];
        [playerName setReturnKeyType:UIReturnKeyDone];
        [playerName setKeyboardType:UIKeyboardTypeASCIICapable];
        UIView* oglView = [[[UIApplication sharedApplication] delegate] oglView];
        [oglView addSubview:playerName];

        ConfigS::instance()->getString("lastPlayerName", _line);
        [playerName setText:[NSString stringWithCString:_line.c_str() length:_line.length()]];
    }
    [playerName setHidden:NO];
    [playerName becomeFirstResponder];
    editEnded = false;
#else
    InputS::instance()->enableInterceptor(this);
    SDL_StartTextInput();
    SDL_SetTextInputRect(0);
#endif
    _isOn = true;
}

const std::string& TextInput::getText(void) {
#ifdef IPHONE
    if (_isOn) {
        NSString* text = [playerName text];
        if (text != nil) {
            const char* constText = [text cStringUsingEncoding:NSASCIIStringEncoding];
            if (constText) {
                _line = constText;
            } else {
                _line += "?";
                [playerName setText:[NSString stringWithCString:_line.c_str() length:_line.length()]];
            }
            if (_line.length() > _maxLen) {
                _line = _line.substr(0, _maxLen);
                [playerName setText:[NSString stringWithCString:_line.c_str() length:_line.length()]];
            }
            ConfigS::instance()->updateKeyword("lastPlayerName", _line);
        }
        if (editEnded) {
            LOG_INFO << "Text edit mode turned off\n";
            turnOff();
        }
    }
#endif
    return _line;
}

void TextInput::turnOff(void) {
    LOG_INFO << "TextInput::turnOff\n";
#ifdef IPHONE
    [playerName setHidden:YES];
    [playerName resignFirstResponder];
#endif

    ScoreData& scoreData = ScoreKeeperS::instance()->getCurrentScoreData();

    bool onlineScores = false;
    ConfigS::instance()->getBoolean("onlineScores", onlineScores);

    if (!scoreData.sent && onlineScores) {
        string escapedName = '"' + StringUtils(_line).replaceAll("\"", "\"\"").str() + '"';

        stringstream scoreMsg;
        scoreMsg << _random.random() << ",";
        scoreMsg << "Shaaft"
                 << ",";
        scoreMsg << ScoreKeeperS::instance()->getCurrentScoreBoardName() << ",";
        scoreMsg << escapedName << ",";
        scoreMsg << scoreData.score << ",";
        scoreMsg << scoreData.cubes << ",";
        scoreMsg << scoreData.secondsPlayed << ",";
        scoreMsg << scoreData.time << ",";
        scoreMsg << GameState::deviceId << ",";
        scoreMsg << (GameState::isPirate ? "P" : "F") << ",";

        OnlineScore::SendScore(scoreData.score, scoreMsg.str());

        scoreData.sent = true;
    }
#ifdef IPHONE
#else

    InputS::instance()->disableInterceptor();
    SDL_StopTextInput();

    MenuManagerS::instance()->turnMenuOn();
#endif
    _isOn = false;
}

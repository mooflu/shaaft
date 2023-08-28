#pragma once
// Description:
//   Context enum.
//
// Copyright (C) 2007 Frank Becker
//

#ifdef HAS_NAMESPACE
namespace 
#else
struct
#endif
Context
{
    enum ContextEnum
    {
        eUnknown,
        eMenu,
        eInGame,
        ePaused,
        eCameraFlyby,
        eLAST
    };
};

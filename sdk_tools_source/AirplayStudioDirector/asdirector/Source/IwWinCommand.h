/*
 * Copyright (C) 2001-2012 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This document is protected by copyright, and contains information
 * proprietary to Ideaworks Labs.
 * This file consists of source code released by Ideaworks Labs under
 * the terms of the accompanying End User License Agreement (EULA).
 * Please do not use this program/source code before you have read the
 * EULA and have agreed to be bound by its terms.
 */
//-----------------------------------------------------------------------------
/*!
    \file IwWinCommand.h
    \brief Commands passed between
 */
//--------------------------------------------------------------------------------

#if !defined(IwWinCOMMAND_H)
#define IwWinCOMMAND_H

enum EIwWinCommandType
{
    GUICMD_UNKNOWN,

    //from ui
    GUICMD_SETDATA,
    GUICMD_GETDATA,
    GUICMD_COMMAND,
    GUICMD_QUEUEMSG,

    //from viewer
    GUICMD_START,
    GUICMD_GETDATA_RETURN,
    GUICMD_MSG,
};

class CIwWinCommand
{
public:
    EIwWinCommandType Type;
    int Num;
    char Line[1024];

    CIwWinCommand(EIwWinCommandType _Type) : Type(_Type) {}
    CIwWinCommand(EIwWinCommandType _Type,const char *Text) : Type(_Type) { strcpy(Line,Text); }
    CIwWinCommand(EIwWinCommandType _Type,unsigned int _Num,const char *Text) : Type(_Type),Num(_Num) { strcpy(Line,Text); }
    CIwWinCommand(EIwWinCommandType _Type,unsigned int _Num) : Type(_Type),Num(_Num) {}

    ~CIwWinCommand() {}
};

typedef void (*IwWinSend)(CIwWinCommand *Cmd);
typedef IwWinSend (*IwWinInitFn)(IwWinSend IwWinSendFn);

#endif // !defined(IwWinCOMMAND_H)

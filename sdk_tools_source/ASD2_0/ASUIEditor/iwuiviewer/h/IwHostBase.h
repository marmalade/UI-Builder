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
#ifndef IW_HOST_BASE_H
#define IW_HOST_BASE_H

// logging typedefs
typedef int (*MessageFn)(unsigned int msg, const char* line);
typedef void (*HostLogFn)(const char* message);
typedef bool (*HostLogErrorFn)(const char* message);

//-----------------------------------------------------------------------------
class CIwHostBase
{
public:
    // Originally from CIwViewerUIBase in IwViewerUI.h
    enum UIMessages
    {
        PARSE_UPDATE        = 0x2e, // String to parse
        MOUSE_POS           = 0x30, // mode,pos.x,pos.y,keyDown
        UPDATED_ELEMENTS    = 0x35, // Compound. Element and update file
        SET_STYLESHEET      = 0x37, // Name of stylesheet resource
        SHUTDOWN            = 0x39, // Exit s86
        ADD_SELECTION       = 0x3a, // Multi-selection
        REMOVE_SELECTION    = 0x3b, // Not used??
        OUTLINE_FLAGS       = 0x41, // Combination of OutlineFlags
        LOCALISATION_FILE   = 0x42, // Set iwlocalise .dat file
    };

    // Source compatibility for unused messages
    enum
    {
        UPDATE_SELECTION    = 0x1,
        CREATE_SELECTION    = 0x2,
        IS_DEAD             = 0x3,

        CURRENT_ELEMENT     = 0x31,
        SELECT_ELEMENT      = 0x32,
        ADDED_ELEMENT       = 0x33,
        MOVED_ELEMENT       = 0x34,
        UPDATE_POS          = 0x36
    };

    CIwHostBase();
    virtual ~CIwHostBase();

    virtual int  GetData(int dataID, char* pData) = 0;
    virtual void SetData(int dataID, const char* pData = 0) = 0;
    virtual void Command(unsigned int cmd) = 0;

    // Use Queue Message to force correct thread safety
    virtual void QueueMessage(unsigned int msg, char* pPendingString = "") = 0;
    virtual void Update() = 0;

    char _align1[0x8];
    MessageFn m_UIMessage;              // Sending windows commands to s86
    MessageFn m_ViewerMessage;          // Receiving commands from s86

    char _align2[0x1124];
    HostLogFn m_LogFn;
    HostLogErrorFn m_LogErrorFn;

protected:
    void* m_BucketMem;
};

#endif

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
#ifndef IW_HOST_LINK_H
#define IW_HOST_LINK_H

// Project includes
#include "BucketAllocator.h"
#include "IwHostBase.h"
#include "IwLock.h"

// Library includes
#include "s3eMemory.h"
#include <deque>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------

class CIwHostLink : public CIwHostBase
{
public:
    CIwHostLink();
    virtual ~CIwHostLink();

    enum ViewerMessages
    {
        UPDATE_SELECTION    = 0x01,
        CREATE_SELECTION    = 0x02,
        IS_DEAD             = 0x03,
        // Try to separate meaning
        _UPDATED_ELEMENTS   = 0x35, // Send _fromviewer.ui
        _UPDATE_POS         = 0x36, // Send MOUSE_POS
        _ADD_SELECTION      = 0x3a, // Add element to host selection
    };

    // Public interface
    bool LogError(const char* pError);
    int ViewerMessage(ViewerMessages msg, const char* pLine = "");

    // Public interface
    virtual void Update();

private:
    // Private enums
    enum OutlineFlags
    {
        OUTLINE_ELEMENTS    = 1 << 0,
        OUTLINE_LAYOUTS     = 1 << 1,
        OUTLINE_SIBLINGS    = 1 << 2,
        OUTLINE_HIERARCHY   = 1 << 3,
        OUTLINE_MARGINS     = 1 << 4
    };

    enum MouseFlags
    {
        LEFT_DOWN           = 1 << 0,
        MIDDLE_DOWN         = 1 << 1,
        RIGHT_DOWN          = 1 << 2,
        SHIFT_DOWN          = 1 << 3,
        CONTROL_DOWN        = 1 << 4,
        META_DOWN           = 1 << 5
    };

    // IwHostBase virtuals
    virtual int GetData(int dataID, char* pData) { return 0; }
    virtual void SetData(int dataID, const char* pData);
    virtual void Command(unsigned int cmd) {}
    virtual void QueueMessage(unsigned int msg, char* pPendingString = "") {}

    // Private structs
    struct Message
    {
        typedef std::deque<Message*, CBucketAllocator<Message*> > Queue;
        typedef std::basic_string<char, std::char_traits<char>, CBucketAllocator<char> > String;

        void* operator new (size_t size) { return g_BucketAllocator->Realloc(0, size); }
        void operator delete(void* p) { g_BucketAllocator->Free(p); }

        UIMessages m_Msg;
        String     m_Data;
    };

    // Private utils
    void _ProcessMessageImmediately(UIMessages msg, bool canCalls3e);
    void _ProcessMessage(UIMessages msg, const std::string& data);
    void _SetOutlineFlags(int flags);
    void _SetInput(const std::vector<std::string>& params);
    void _UpdateElements(const std::vector<std::string>& params);
    void _LinkToViewer();

    // Member data
    CIwCriticalSection m_QueueMutex;
    Message::Queue m_Queue;
};

#endif

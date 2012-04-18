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
// Standard includes
#include "IwHostLink.h"

// Project includes
#include "Util.h"

// Library includes
#include "IwDebug.h"
#include "IwMemory.h"
#include "IwMemBucketHelpers.h"
#include "IwUIViewer.h"
#include "IwUtilInitTerm.h"
#include "memory.h"
#include "s3eLibrary.h"
#include <sstream>

extern "C"
{
typedef void (*LinkFn)(CIwHostBase* host);
}

// Globals
CIwMemBucket* g_BucketAllocator = NULL;

//-----------------------------------------------------------------------------

static CIwHostBase* s_HostBase = NULL;

CIwHostBase::CIwHostBase() :
    m_UIMessage(NULL),
    m_ViewerMessage(NULL),
    m_LogFn(NULL),
    m_LogErrorFn(NULL),
    m_BucketMem(NULL)
{
    s_HostBase = this;

    IwUtilInit();

    // IwMemBucketDebugSetBreakpoint(5);

    // to match CIwViewerUI of iwviewer:
    memset(_align1, 0, sizeof(_align1));
    memset(_align2, 0, sizeof(_align2));

    // Allocations are not thread safe in Marmalade
    // Use separate bucket & allocator to make queue thread safe (hopefully)
    const int bucketSize = 1024 * 1024;
    m_BucketMem = new char[bucketSize];
    g_BucketAllocator =
        new CIwMemBucketDLBuffer(m_BucketMem, bucketSize, "_link_bucket_");
}

CIwHostBase::~CIwHostBase()
{
    delete g_BucketAllocator;
    delete[] m_BucketMem;
    g_BucketAllocator = NULL;

    IwUtilTerminate();

    s_HostBase = NULL;
}

//-----------------------------------------------------------------------------

CIwHostLink::CIwHostLink()
{
    _LinkToViewer();
}

CIwHostLink::~CIwHostLink()
{
    ViewerMessage(IS_DEAD);

    // Destroy any remaining messages
    CIwLock queueLock(m_QueueMutex);
    if (queueLock.HaveLock())
    {
        while (m_Queue.size())
        {
            delete m_Queue.back();
            m_Queue.pop_back();
        }
    }
}

void CIwHostLink::Update()
{
    // TODO: The problem with polling input is that we sometimes
    // completely miss mouse clicks
    ViewerMessage(CIwHostLink::_UPDATE_POS);

    while (true)
    {
        bool haveMessage = false;
        UIMessages msg;
        std::string data;

        // Only keep lock for popping message, not processing.
        // Avoids deadlock if we call back into wx side.
        {
            CIwLock queueLock(m_QueueMutex, true);
            if (queueLock.HaveLock())
            {
                if (m_Queue.size() > 0)
                {
                    Message* pMsg = m_Queue.front();
                    m_Queue.pop_front();

                    haveMessage = true;
                    msg = pMsg->m_Msg;
                    data = pMsg->m_Data.c_str();

                    delete pMsg;
                }
            }
        }

        if (haveMessage)
        {
            // Can't be as immediate as we'd like!
            _ProcessMessageImmediately(msg, true);
            _ProcessMessage(msg, data);
            haveMessage = false;
        }
        else
            // Wait till next time
            break;
    }
}

void CIwHostLink::SetData(int dataID, const char* pData)
{
    CIwLock queueLock(m_QueueMutex);

    if (queueLock.HaveLock())
    {
        Message* pMsg = new Message;
        pMsg->m_Msg = (UIMessages) dataID;
        pMsg->m_Data = pData ? pData : "";

        m_Queue.push_back(pMsg);
    }

    // Currently can't call any s3e functions from a foreign thread
    _ProcessMessageImmediately((UIMessages) dataID, false);
}

bool CIwHostLink::LogError(const char* pError)
{
    IwDebugTraceLinePrintf("%s", pError);

    if (m_LogErrorFn)
        return m_LogErrorFn(pError);

    return false;
}

int CIwHostLink::ViewerMessage(ViewerMessages msg, const char* pLine)
{
    if (m_ViewerMessage)
    {
        // Copy string as host can alter it!
        char buffer[512];
        strncpy(buffer, pLine, sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';

        // This doesn't seem very thread safe
        return m_ViewerMessage(msg, pLine);
    }

    return 0;
}

void CIwHostLink::_ProcessMessageImmediately(UIMessages msg, bool canCalls3e)
{
    // Allow for quit during loading
    if (msg == SHUTDOWN)
    {
        if (canCalls3e)
            s3eDeviceRequestQuit();

        // Clear these now - host is exiting
        m_UIMessage     = NULL;
        m_ViewerMessage = NULL;
        m_LogFn         = NULL;
        m_LogErrorFn    = NULL;
    }

    // Get things moving
    if (canCalls3e)
        s3eDeviceUnYield();
}

void CIwHostLink::_ProcessMessage(UIMessages msg, const std::string& data)
{
    try
    {
        // Throw exception if we fail to decode data
        std::istringstream iss(data);
        iss.exceptions(std::ios_base::failbit);

        switch (msg)
        {
        case PARSE_UPDATE:
        {
            IwGetUIViewer()->ParseUpdate(data.c_str());
        }
        break;

        case MOUSE_POS:
        {
            std::vector<std::string> params;
            Util::SplitSeparatedStrings(data, params, '|', true);
            _SetInput(params);
        }
        break;

        case UPDATED_ELEMENTS:
        {
            std::vector<std::string> subStrings;
            Util::SplitQuotedStrings(data, subStrings);
            _UpdateElements(subStrings);
        }
        break;

        case SET_STYLESHEET:
        {
            // Take empty string to indicate no stylesheet
            const char* pResName = !data.empty() ? data.c_str() : NULL;
            IwGetUIViewer()->SetStyleSheet(pResName);
        }
        break;

        case SHUTDOWN:
        {
            IwGetUIViewer()->Shutdown();
        }
        break;

        case ADD_SELECTION:
        {
            IwGetUIViewer()->AddSelection(data.c_str());
        }
        break;

        case REMOVE_SELECTION:
        {
            IwGetUIViewer()->RemoveSelection(data.c_str());
        }
        break;

        case OUTLINE_FLAGS:
        {
            int flags = -1;
            iss >> flags;
            _SetOutlineFlags(flags);
        }
        break;

        case LOCALISATION_FILE:
        {
            const char* pLocaliseDat = !data.empty() ? data.c_str() : NULL;
            IwGetUIViewer()->SetLocalisation(pLocaliseDat);
        }
        break;

        default:
            IwAssertMsg(VIEWER, false,
                        ("Unrecognised msg: %d, %s", (int)msg, data.c_str()));
        }
    }
    catch (std::ios_base::failure error)
    {
        IwAssertMsg(VIEWER, false,
                    ("Failed processing msg(%d): %s", msg, data.c_str()));
    }
}

void CIwHostLink::_SetOutlineFlags(int flags)
{
    bool elements   = (flags & OUTLINE_ELEMENTS) != 0;
    bool layouts    = (flags & OUTLINE_LAYOUTS) != 0;
    bool siblings   = (flags & OUTLINE_SIBLINGS) != 0;
    bool hierarchy  = (flags & OUTLINE_HIERARCHY) != 0;
    bool margins    = (flags & OUTLINE_MARGINS) != 0;

    IwGetUIViewer()->SetOutlineFlags(elements, layouts, siblings, hierarchy, margins);
}

void CIwHostLink::_SetInput(const std::vector<std::string>& params)
{
    if (params.size() == 4)
    {
        int vals[3];
        for (int i=0; i<3; ++i)
        {
            std::istringstream iss(params[i]);
            iss.exceptions (std::ios_base::failbit);
            iss >> vals[i];
        }

        int mode = vals[0], posX = vals[1], posY = vals[2];
        const char* pKeyOrText = params[3].c_str();

        if (mode & 128)
        {
            bool mouseDown = (mode & LEFT_DOWN) != 0;
            bool controlDown = (mode & CONTROL_DOWN) != 0;
            bool shiftDown = (mode & SHIFT_DOWN) != 0;
            int keyDown = atoi(pKeyOrText);

            IwGetUIViewer()->UpdateKeyboard(keyDown);

            IwGetUIViewer()->UpdatePointer(posX, posY, mouseDown,
                                           controlDown, shiftDown);
        }
        else
        {
            CIwUIViewer::DragMode dragMode = (CIwUIViewer::DragMode) mode;

            IwGetUIViewer()->UpdatePaletteDrag(dragMode, posX, posY, pKeyOrText);
        }
    }
    else
        IwAssertMsg(VIEWER, false,
                    ("Unexpected number of update input parameters"));
}

void CIwHostLink::_UpdateElements(const std::vector<std::string>& params)
{
    int numParams = params.size();

    if (4 <= numParams && numParams <= 5)
    {
        const char* root = params[0].c_str();
        const char* file = params[1].c_str();
        const char* curr = params[2].c_str();

        std::istringstream iss(params[3]);
        iss.exceptions (std::ios_base::failbit);
        int mode = -1;
        iss >> mode;

        const char* path = numParams >= 5 ? params[4].c_str() : NULL;

        IwGetUIViewer()->UpdateElements(root, file, curr,
                                        (CIwUIViewer::UpdateMode)mode, path);
    }
    else
        IwAssertMsg(VIEWER, false,
                    ("Unexpected number of update element parameters"));
}

static LinkFn GetLinkFn()
{
    LinkFn linkFunc = NULL;

    if (s3eDLLHandle* dllHandle = s3eLibraryOpen("UIBuilder.exe"))
    {
        linkFunc = (LinkFn)
                   s3eExtLibraryGetSymbol(dllHandle, "LinkFromLauncherUI");

        s3eExtLibraryClose(dllHandle);
    }

    return linkFunc;
}

void CIwHostLink::_LinkToViewer()
{
    if (LinkFn linkFn = GetLinkFn())
        linkFn(this);
}

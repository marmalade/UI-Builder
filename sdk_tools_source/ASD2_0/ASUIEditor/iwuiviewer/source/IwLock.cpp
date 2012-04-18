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
#include "IwLock.h"
#include <wchar.h>

// System includes
// Can't use s3e pthreads because it hasn't created all threads involved
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include "Windows.h"


//-----------------------------------------------------------------------------

CIwCriticalSection::CIwCriticalSection() : m_CriticalSection(NULL)
{
    m_CriticalSection = new CRITICAL_SECTION;

    InitializeCriticalSection(m_CriticalSection);
}

CIwCriticalSection::~CIwCriticalSection()
{
    DeleteCriticalSection(m_CriticalSection);

    delete m_CriticalSection;
}

//-----------------------------------------------------------------------------

CIwLock::CIwLock(CIwCriticalSection& criticalSection, bool tryLock) :
    m_CriticalSection(&criticalSection),
    m_HaveLock(false)
{
    if (tryLock)
        m_HaveLock =
            TryEnterCriticalSection(m_CriticalSection->m_CriticalSection) != 0;
    else
    {
        EnterCriticalSection(m_CriticalSection->m_CriticalSection);
        m_HaveLock = true;
    }
}

CIwLock::~CIwLock()
{
    if (m_HaveLock)
        LeaveCriticalSection(m_CriticalSection->m_CriticalSection);
}

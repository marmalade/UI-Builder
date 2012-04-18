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
#ifndef IW_LOCK_H
#define IW_LOCK_H

//-----------------------------------------------------------------------------

class CIwCriticalSection
{
public:
    CIwCriticalSection();
    ~CIwCriticalSection();

private:
    struct _RTL_CRITICAL_SECTION * m_CriticalSection;

    friend class CIwLock;
};

//-----------------------------------------------------------------------------

class CIwLock
{
public:
    CIwLock(CIwCriticalSection& criticalSection, bool tryLock = false);
    ~CIwLock();

    // Public interface
    bool HaveLock() const { return m_HaveLock; }

private:
    // Member data
    CIwCriticalSection* m_CriticalSection;
    bool m_HaveLock;
};

#endif

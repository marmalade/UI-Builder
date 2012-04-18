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
#ifndef IW_UI_APP_H
#define IW_UI_APP_H

// Library includes
#include "s3eTypes.h"

// Forward declarations
class CIwResGroup;
class CIwUIApp;

//-----------------------------------------------------------------------------

class IIwUIActivity
{
public:
    virtual void Update(int32 deltaMS) = 0;
    virtual void Render() = 0;
};

//-----------------------------------------------------------------------------

class CIwUIApp
{
public:
    CIwUIApp();
    ~CIwUIApp();

    // Public interface
    void Run(IIwUIActivity* pActivity = NULL);

    void Restart();
    void Quit();

private:
    // Private utils
    void Update(int32 deltaMS);
    void Render();
    int32 Yield();

    // Member data
    bool m_Restart;
    uint64 m_Timer;
    CIwResGroup* m_ResGroup;
};

#endif

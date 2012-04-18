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
#include "IwUIApp.h"

// Library includes
#include "IwUI.h"

//-----------------------------------------------------------------------------

CIwUIApp::CIwUIApp() :
    m_Restart(false),
    m_Timer(0)
{
    IwUIInit();

    new CIwUIView;
    new CIwUIController;
    new CIwUITextInput;

    // Black background
    IwGxSetColClear(0x0, 0x0, 0x0, 0xff);
}

CIwUIApp::~CIwUIApp()
{
    delete IwGetUITextInput();
    delete IwGetUIController();
    delete IwGetUIView();

    IwUITerminate();
}

void CIwUIApp::Run(IIwUIActivity* pActivity)
{
    m_Timer = s3eTimerGetMs();

    int32 updateTime = 0;
    while (!s3eDeviceCheckQuitRequest() && !m_Restart)
    {
        IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);

        Update(updateTime);
        if (pActivity)
            pActivity->Update(updateTime);

        Render();
        if (pActivity)
            pActivity->Render();

        IwGxFlush();
        IwGxSwapBuffers();
        IwGxTickUpdate(); // Required for cursor animation

        updateTime = Yield();
    }
}

void CIwUIApp::Restart()
{
    m_Restart = true;
}

void CIwUIApp::Quit()
{
    s3eDeviceRequestQuit();
}

void CIwUIApp::Update(int32 deltaMS)
{
    IwGetUIController()->Update();
    IwGetUIView()->Update(deltaMS);
}

void CIwUIApp::Render()
{
    IwGetUIView()->Render();
}

int32 CIwUIApp::Yield()
{
    uint64 now = s3eTimerGetMs();

    // Attempt frame rate
    // (low in an attempt to keep UI Builder responsive / less CPU intensive)
    const int32 frameTime = 1000 / 20; // 20fps
    int32 yield = now <= m_Timer + frameTime ?
                  (int32)(m_Timer + frameTime - now) : 0;
    s3eDeviceYield(yield);

    // Standard device interaction
    s3eKeyboardUpdate();
    s3ePointerUpdate();

    // Determine time since last call
    now = s3eTimerGetMs();
    int32 updateTime = (int32)(now - m_Timer);
    updateTime = MAX(0, MIN(updateTime, 1000));
    m_Timer = now;

    return updateTime;
}

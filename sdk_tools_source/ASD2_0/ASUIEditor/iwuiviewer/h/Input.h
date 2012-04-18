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
#ifndef INPUT_H
#define INPUT_H

// Library includes
#include "IwGeomVec2.h"

//-----------------------------------------------------------------------------

class IInputHandler
{
public:
    virtual void HandleKey(int key) = 0;
    virtual void HandleClick(const CIwVec2& pos) = 0;

    virtual void HandleDragPress(const CIwVec2& start) = 0;
    virtual void HandleDragMove(const CIwVec2& move) = 0;
    virtual void HandleDragRelease(const CIwVec2& end) = 0;
};

class CInput
{
public:
    CInput(IInputHandler& inputHandler);

    // Public interface
    void SetPointerPos(const CIwVec2& pos);
    void SetPointerButton(bool down);
    void SetControl(bool down);
    void SetShift(bool down);
    void SetKey(int key);

    bool GetControl() const { return m_Control; }
    bool GetShift() const { return m_Shift; }
    const CIwVec2& GetPointerPos() const { return m_PointerPos; }

private:
    // Member data
    IInputHandler& m_InputHandler;

    CIwVec2 m_PointerPos;
    CIwVec2 m_PointerDownPos;
    bool m_PointerButton;
    bool m_PointerDrag;

    bool m_Control;
    bool m_Shift;

    int m_Key;
    int m_LastKey;
};

#endif

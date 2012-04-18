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
#include "Input.h"

//-----------------------------------------------------------------------------

CInput::CInput(IInputHandler& inputHandler) :
    m_InputHandler(inputHandler),
    m_PointerPos(0, 0),
    m_PointerDownPos(0, 0),
    m_PointerButton(false),
    m_PointerDrag(false),
    m_Control(false),
    m_Shift(false),
    m_Key(0),
    m_LastKey(0)
{

}

void CInput::SetPointerPos(const CIwVec2& pos)
{
    bool changed = m_PointerPos != pos;

    m_PointerPos = pos;

    // Moving with mouse down - drag
    if (m_PointerButton)
    {
        if (!m_PointerDrag && m_PointerDownPos != pos)
        {
            m_PointerDrag = true;
            m_InputHandler.HandleDragPress(m_PointerDownPos);
        }

        if (m_PointerDrag && changed)
            m_InputHandler.HandleDragMove(pos);
    }
}

void CInput::SetPointerButton(bool down)
{
    bool changed = down != m_PointerButton;

    if (changed && down)
        // Store position of mouse down
        m_PointerDownPos = m_PointerPos;

    m_PointerButton = down;

    if (changed && !down)
    {
        if (m_PointerDrag)
        {
            m_PointerDrag = false;
            m_InputHandler.HandleDragRelease(m_PointerPos);
        }
        // No mouse movement - click
        // Second because we might drag back to the same position
        else if (m_PointerDownPos == m_PointerPos)
            m_InputHandler.HandleClick(m_PointerPos);
    }
}

void CInput::SetControl(bool down)
{
    m_Control = down;
}

void CInput::SetShift(bool down)
{
    m_Shift = down;
}

void CInput::SetKey(int key)
{
    // Keys seem to get repeated, so wait for zero to be sent in between.
    if (key && key != m_LastKey)
        m_InputHandler.HandleKey(key);

    m_LastKey = key;
}

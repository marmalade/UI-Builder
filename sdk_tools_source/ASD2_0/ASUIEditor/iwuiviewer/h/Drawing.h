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
#ifndef DRAWING_H
#define DRAWING_H

// Library includes
#include "IwGeomVec2.h"
#include "IwUIRect.h"

// Forward declarations
class CIwUIElement;

//-----------------------------------------------------------------------------

namespace Drawing
{

void DrawLoadingScreen(int count);

void DrawHandle(CIwVec2 posa,int16 sizea,bool first);

void DrawBox(CIwVec2 pos,CIwVec2 size);

void DrawBox(CIwVec2 pos,CIwVec2 size,int16 offset);

void DrawLine(CIwVec2 from,CIwVec2 to);

void DrawLine(CIwVec2 from,CIwVec2 to,int16 offset);

void DrawGrid(CIwVec2 size,CIwVec2 spacing);

void DrawSelectionBox(CIwVec2 from,int16 size);

void DrawSelectionBox(CIwVec2 from,CIwVec2 to);

void DrawHilight(CIwVec2 pos,CIwVec2 size,int sel);

void DrawOutline(CIwVec2 pos,CIwVec2 size,int16 offset,bool first,int selHandle);

void DrawOutline(CIwUIElement* elem,int selHandle, int offset);

enum
{
    HANDLES_NONE            = -1,
    HANDLES_ALL             = 0,
    HANDLES_LEFT_TOP        = 1,
    HANDLES_CENTRE_TOP      = 2,
    HANDLES_RIGHT_TOP       = 3,
    HANDLES_RIGHT_MIDDLE    = 4,
    HANDLES_RIGHT_BOTTOM    = 5,
    HANDLES_CENTRE_BOTTOM   = 6,
    HANDLES_LEFT_BOTTOM     = 7,
    HANDLES_LEFT_MIDDLE     = 8
};

CIwUIRect GetHandleRect(const CIwUIRect& rect, int handle);

int IntersectHandles(const CIwUIRect& rect, const CIwVec2& pos);

CIwUIRect SizeRectWithHandle(const CIwUIRect& rect, int handle, const CIwVec2& pos);

}

#endif

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
#include "Drawing.h"

// Library includes
#include "IwGx.h"
#include "IwUIElement.h"

//-----------------------------------------------------------------------------

namespace Drawing
{

void DrawLoadingScreen(int count)
{
    if (count < 0) count = -count;

    IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);

    int x = (IwGxGetScreenWidth() - 6) / 2;
    int y = (IwGxGetScreenHeight() - 8) / 2;

    IwGxPrintSetColour(0x80, 0x80, 0x80);

    const char* pStrings[] = { "\\", "|", "/", "-" };
    int numStrings = sizeof(pStrings) / sizeof(pStrings[0]);

    IwGxPrintString(x, y, pStrings[count % numStrings]);

    IwGxFlush();
    IwGxSwapBuffers();
}

void DrawHandle(CIwVec2 posa,int16 sizea,bool first)
{
    CIwVec2 pos(posa.x-sizea,posa.y-sizea);
    CIwVec2 size(sizea*2,sizea*2);

    if (!first)
    {
        pos.x++;
        pos.y++;
        size.x-=2;
        size.y-=2;
    }

    DrawLine(CIwVec2(pos.x,pos.y),CIwVec2(pos.x,pos.y+size.y));
    DrawLine(CIwVec2(pos.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y+size.y));
    DrawLine(CIwVec2(pos.x+size.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y));
    DrawLine(CIwVec2(pos.x+size.x,pos.y),CIwVec2(pos.x,pos.y));
}

void DrawBox(CIwVec2 pos,CIwVec2 size)
{
    DrawLine(CIwVec2(pos.x,pos.y),CIwVec2(pos.x,pos.y+size.y));
    DrawLine(CIwVec2(pos.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y+size.y));
    DrawLine(CIwVec2(pos.x+size.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y));
    DrawLine(CIwVec2(pos.x+size.x,pos.y),CIwVec2(pos.x,pos.y));
}

void DrawBox(CIwVec2 pos,CIwVec2 size,int16 offset)
{
    DrawLine(CIwVec2(pos.x,pos.y),CIwVec2(pos.x,pos.y+size.y),offset);
    DrawLine(CIwVec2(pos.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y+size.y),offset);
    DrawLine(CIwVec2(pos.x+size.x,pos.y+size.y),CIwVec2(pos.x+size.x,pos.y),offset);
    DrawLine(CIwVec2(pos.x+size.x,pos.y),CIwVec2(pos.x,pos.y),offset);
}

void DrawLine(CIwVec2 from,CIwVec2 to)
{
    CIwSVec2* verts = IW_GX_ALLOC(CIwSVec2, 2);

    verts[0]=from;
    verts[1]=to;

    IwGxSetVertStreamScreenSpace(verts, 2);
    IwGxSetNormStream(NULL);
    IwGxSetColStream(NULL);

    IwGxDrawPrims(IW_GX_LINE_STRIP, NULL, 2);
}

void DrawLine(CIwVec2 from,CIwVec2 to,int16 offset)
{
    CIwVec2 lenv=to-from;
    int32 len=lenv.GetLength();
    float x=lenv.x/(float)len;
    float y=lenv.y/(float)len;

    int num =  2 * ((3+len-offset) / 4);
    CIwSVec2* verts = IW_GX_ALLOC(CIwSVec2, num);

    for (int j=0, i=offset; j<num; j+=2, i+=4)
    {
        verts[j]=from+CIwVec2((int16)(x*i),(int16)(y*i));
        verts[j+1]=from+CIwVec2((int16)(x*(i+2)),(int16)(y*(i+2)));
    }

    IwGxSetVertStreamScreenSpace(verts, num);
    IwGxSetNormStream(NULL);
    IwGxSetColStream(NULL);
    IwGxDrawPrims(IW_GX_LINE_LIST, NULL, num);
}

void DrawGrid(CIwVec2 size,CIwVec2 spacing)
{
    int i;
    CIwColour col;
    col.Set(0xFF3f3f3f);

    CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
    pMat->Init();
    pMat->SetCullMode(CIwMaterial::CULL_NONE);
    pMat->SetColEmissive(0);
    pMat->SetColAmbient(col);
    pMat->SetColDiffuse(0);
    IwGxSetMaterial(pMat);
    IwGxSetViewSpaceOrg(&CIwSVec3::g_Zero);

    int32 zdf = IwGxGetZDepthFixed();
    IwGxSetZDepthFixed( IwGxGetFarZ());

    int32 screenSlot = IwGxGetScreenSpaceSlot();
    IwGxSetScreenSpaceSlot(-1);

    for (i=spacing.x; i<size.x; i+=spacing.x)
    {
        DrawLine(CIwVec2(i,0),CIwVec2(i,size.y));
    }

    for (i=spacing.y; i<size.y; i+=spacing.y)
    {
        DrawLine(CIwVec2(0,i),CIwVec2(size.x,i));
    }

    IwGxSetScreenSpaceSlot(screenSlot);
    IwGxFlush();
    IwGxClear(IW_GX_DEPTH_BUFFER_F);
    IwGxFlush();
}

void DrawSelectionBox(CIwVec2 from,int16 size)
{
    DrawSelectionBox(CIwVec2(from.x-size,from.y-size),CIwVec2(from.x+size,from.y+size));
}

void DrawSelectionBox(CIwVec2 from,CIwVec2 to)
{
    CIwColour col;
    col.Set(0xFF0000FF);

    CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
    pMat->Init();
    pMat->SetCullMode(CIwMaterial::CULL_NONE);
    pMat->SetColEmissive(0);
    pMat->SetColAmbient(col);
    pMat->SetColDiffuse(0);
    IwGxSetMaterial(pMat);
    IwGxSetViewSpaceOrg(&CIwSVec3::g_Zero);

    int32 zdf = IwGxGetZDepthFixed();
    IwGxSetZDepthFixed( IwGxGetNearZ() );

    DrawBox(from,to-from-CIwVec2(1,1));

    IwGxSetZDepthFixed( zdf );
}

void DrawHilight(CIwVec2 pos,CIwVec2 size,int sel)
{
    CIwColour col;
    col.Set(0xFF0000FF);

    size.x--;
    size.y--;

    CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
    pMat->Init();
    pMat->SetCullMode(CIwMaterial::CULL_NONE);
    pMat->SetColEmissive(0);
    pMat->SetColAmbient(col);
    pMat->SetColDiffuse(0);
    IwGxSetMaterial(pMat);
    IwGxSetViewSpaceOrg(&CIwSVec3::g_Zero);

    int32 zdf = IwGxGetZDepthFixed();
    IwGxSetZDepthFixed( IwGxGetNearZ() );

    if (sel==1)
        DrawHandle(CIwVec2(pos.x,pos.y),2,true);

    if (sel==2)
        DrawHandle(CIwVec2(pos.x+size.x/2,pos.y),2,true);

    if (sel==3)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y),2,true);

    if (sel==4)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y+size.y/2),2,true);

    if (sel==5)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y+size.y),2,true);

    if (sel==6)
        DrawHandle(CIwVec2(pos.x+size.x/2,pos.y+size.y),2,true);

    if (sel==7)
        DrawHandle(CIwVec2(pos.x,pos.y+size.y),2,true);

    if (sel==8)
        DrawHandle(CIwVec2(pos.x,pos.y+size.y/2),2,true);

    IwGxSetZDepthFixed( zdf );
}

void DrawOutline(CIwVec2 pos,CIwVec2 size,int16 offset,bool first,int selHandle)
{
    offset=offset%4;

    DrawBox(pos,size,offset);
    if (selHandle==-1) return;

    if (selHandle==0 || selHandle==1)
        DrawHandle(CIwVec2(pos.x,pos.y),2,first);

    if (selHandle==0 || selHandle==2)
        DrawHandle(CIwVec2(pos.x+size.x/2,pos.y),2,first);

    if (selHandle==0 || selHandle==3)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y),2,first);

    if (selHandle==0 || selHandle==4)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y+size.y/2),2,first);

    if (selHandle==0 || selHandle==5)
        DrawHandle(CIwVec2(pos.x+size.x,pos.y+size.y),2,first);

    if (selHandle==0 || selHandle==6)
        DrawHandle(CIwVec2(pos.x+size.x/2,pos.y+size.y),2,first);

    if (selHandle==0 || selHandle==7)
        DrawHandle(CIwVec2(pos.x,pos.y+size.y),2,first);

    if (selHandle==0 || selHandle==8)
        DrawHandle(CIwVec2(pos.x,pos.y+size.y/2),2,first);
}

void DrawOutline(CIwUIElement* elem,int selHandle, int offset)
{
    CIwVec2 pos=elem->GetPosAbsolute();
    CIwVec2 size=elem->GetSize();
    size.x--;
    size.y--;
    CIwColour col;
    col.Set(0xFFFFFFFF);

    CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
    pMat->Init();
    pMat->SetCullMode(CIwMaterial::CULL_NONE);
    pMat->SetColEmissive(0);
    pMat->SetColAmbient(col);
    pMat->SetColDiffuse(0);

    CIwMaterial* pMat2 = IW_GX_ALLOC_MATERIAL();
    pMat2->Init();
    pMat2->SetCullMode(CIwMaterial::CULL_NONE);
    pMat2->SetColEmissive(0);
    pMat2->SetColDiffuse(0);

    IwGxSetMaterial(pMat);
    IwGxSetViewSpaceOrg(&CIwSVec3::g_Zero);

    int32 zdf = IwGxGetZDepthFixed();
    IwGxSetZDepthFixed( IwGxGetNearZ() );

    DrawOutline(pos,size,offset,true,selHandle);

    col.Set(0xFF000000);
    pMat2->SetColAmbient(col);
    IwGxSetMaterial(pMat2);

    DrawOutline(pos,size,offset+2,false,selHandle);

    IwGxSetZDepthFixed(zdf);
}

CIwUIRect GetHandleRect(const CIwUIRect& rect, int handle)
{
    if (handle <= 0 || handle > 8)
        return rect;

    CIwVec2 centre(0,0);
    switch (handle)
    {
    case HANDLES_LEFT_TOP:
        centre = rect.GetTopLeft();
        break;
    case HANDLES_CENTRE_TOP:
        centre = (rect.GetTopLeft() + rect.GetTopRight()) / IW_FIXED(2);
        break;
    case HANDLES_RIGHT_TOP:
        centre = rect.GetTopRight();
        break;
    case HANDLES_RIGHT_MIDDLE:
        centre = (rect.GetTopRight() + rect.GetBottomRight()) / IW_FIXED(2);
        break;
    case HANDLES_RIGHT_BOTTOM:
        centre = rect.GetBottomRight();
        break;
    case HANDLES_CENTRE_BOTTOM:
        centre = (rect.GetBottomRight() + rect.GetBottomLeft()) / IW_FIXED(2);
        break;
    case HANDLES_LEFT_BOTTOM:
        centre = rect.GetBottomLeft();
        break;
    case HANDLES_LEFT_MIDDLE:
        centre = (rect.GetBottomLeft() + rect.GetTopLeft()) / IW_FIXED(2);
        break;
    }

    int handleSize = 2;

    CIwVec2 pos(centre.x - handleSize, centre.y - handleSize);
    CIwVec2 size(2 * handleSize, 2 * handleSize);

    return CIwUIRect(pos, size);
}

int IntersectHandles(const CIwUIRect& rect, const CIwVec2& pos)
{
    for (int i=1; i<=8; ++i)
    {
        CIwUIRect handle = GetHandleRect(rect, i);
        if (handle.Intersects(pos))
            return i;
    }

    return HANDLES_NONE;
}

CIwUIRect SizeRectWithHandle(const CIwUIRect& rect, int handle, const CIwVec2& pos)
{
    if (handle <= 0 || handle > 8)
        return rect;

    CIwVec2 topLeft = rect.GetTopLeft();
    CIwVec2 bottomRight = rect.GetBottomRight();

    switch (handle)
    {
    case HANDLES_LEFT_TOP:
        topLeft = pos;
        break;
    case HANDLES_CENTRE_TOP:
        topLeft.y = pos.y;
        break;
    case HANDLES_RIGHT_TOP:
        topLeft.y = pos.y;
        bottomRight.x = pos.x;
        break;
    case HANDLES_RIGHT_MIDDLE:
        bottomRight.x = pos.x;
        break;
    case HANDLES_RIGHT_BOTTOM:
        bottomRight = pos;
        break;
    case HANDLES_CENTRE_BOTTOM:
        bottomRight.y = pos.y;
        break;
    case HANDLES_LEFT_BOTTOM:
        topLeft.x = pos.x;
        bottomRight.y = pos.y;
        break;
    case HANDLES_LEFT_MIDDLE:
        topLeft.x = pos.x;
        break;
    }

    // Top-left shouldn't move beyond original bottom-right
    topLeft.x = MIN(topLeft.x, rect.x + rect.w);
    topLeft.y = MIN(topLeft.y, rect.y + rect.h);

    // Bottom-right shouldn't move before original top-left
    bottomRight.x = MAX(bottomRight.x, rect.x);
    bottomRight.y = MAX(bottomRight.y, rect.y);

    return CIwUIRect(topLeft, bottomRight - topLeft);
}

}

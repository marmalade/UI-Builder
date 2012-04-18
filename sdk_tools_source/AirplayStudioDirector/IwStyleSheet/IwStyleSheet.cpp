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
#include "wxInclude.h"
#include "IwStyleSheet.h"

static wxString s_SDKIdent;
//--------------------------------------------------------------------------------
//get SDK identity
wxString IwGetSDKIdent()
{
    if (!s_SDKIdent.empty())
        return s_SDKIdent;

    s_SDKIdent=L"Marmalade";
    char key[]="branding";
    char buffer[256];
    int len=0;

    FILE* fp=fopen((IwGetSharedResourceDir()+L"/sdkident.ast").mb_str(),"rb");
    if (!fp) return s_SDKIdent;

    char* s=fgets(buffer,256,fp);
    len=ftell(fp);
    if (!s) return s_SDKIdent;

    fclose(fp);

    for (int i=0; i<len; i++)
    {
        buffer[i]=buffer[i] ^ key[i % strlen(key)];
    }
    while (buffer[strlen(buffer)-1]=='\n')
        buffer[strlen(buffer)-1]=0;

    if (strlen(buffer)>0)
        s_SDKIdent=wxString(buffer,wxConvUTF8);

    return s_SDKIdent;
}

static wxString s_SDKSDKCopyright;
//--------------------------------------------------------------------------------
//get SDK identity
wxString IwGetSDKCopyright()
{
    if (!s_SDKSDKCopyright.empty())
        return s_SDKSDKCopyright;

    s_SDKSDKCopyright=L"Copyright © 2001-2012 Ideaworks3D Ltd.";
    char key[]="branding";
    char buffer[256];
    int len=0;

    FILE* fp=fopen((IwGetSharedResourceDir()+L"/sdkcopyright.ast").mb_str(),"rb");
    if (!fp) return s_SDKSDKCopyright;

    char* s=fgets(buffer,256,fp);
    len=ftell(fp);
    if (!s) return s_SDKSDKCopyright;

    fclose(fp);

    for (int i=0; i<len; i++)
    {
        buffer[i]=buffer[i] ^ key[i % strlen(key)];
    }
    while (buffer[strlen(buffer)-1]=='\n')
        buffer[strlen(buffer)-1]=0;

    if (strlen(buffer)>0)
        s_SDKSDKCopyright=wxString(buffer,wxConvUTF8);

    return s_SDKSDKCopyright;
}

bool CIwStyleCtrlGroup::s_UseODCtrls = false;

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleHeader, wxPanel)
    EVT_PAINT(CIwStyleHeader::OnPaint)
    EVT_SIZE(CIwStyleHeader::OnSize)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleNoteBook, wxNotebook)
    EVT_MOUSE_EVENTS(CIwStyleNoteBook::OnMouse)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleList, wxListCtrl)
    EVT_MOUSE_EVENTS(CIwStyleList::OnMouse)
END_EVENT_TABLE()

#ifndef I3D_OS_WINDOWS
BEGIN_EVENT_TABLE(CIwStyleLabelOD, wxStaticText)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleButtonOD, wxButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleCheckOD, wxCheckBox)
END_EVENT_TABLE()
#else
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleButtonOD, wxButton)
    EVT_PAINT(CIwStyleButtonOD::OnPaint)
    EVT_MOUSE_EVENTS(CIwStyleButtonOD::OnMouse)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleLabelOD, wxStaticText)
    EVT_PAINT(CIwStyleLabelOD::OnPaint)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwStyleCheckOD, wxCheckBox)
    EVT_PAINT(CIwStyleCheckOD::OnPaint)
    EVT_MOUSE_EVENTS(CIwStyleCheckOD::OnMouse)
END_EVENT_TABLE()

//'alf-inched from wxWidgets code
static void DrawButtonText(HDC hdc,RECT *pRect,const wxString& text,COLORREF col)
{
    COLORREF colOld = SetTextColor(hdc, col);
    int modeOld = SetBkMode(hdc, TRANSPARENT);

    wxFont font=CIwStyleHeader::GetFont();
    SelectObject(hdc,font.GetHFONT());

    if ( text.find(_T('\n')) != wxString::npos )
    {
        // draw multiline label

        // first we need to compute its bounding rect
        RECT rc;
        ::CopyRect(&rc, pRect);
        ::DrawText(hdc, text, text.length(), &rc, DT_CENTER | DT_CALCRECT);

        // now center this rect inside the entire button area
        const LONG w = rc.right - rc.left;
        const LONG h = rc.bottom - rc.top;
        rc.left = (pRect->right - pRect->left)/2 - w/2;
        rc.right = rc.left+w;
        rc.top = (pRect->bottom - pRect->top)/2 - h/2;
        rc.bottom = rc.top+h;

        ::DrawText(hdc, text, text.length(), &rc, DT_CENTER);
    }
    else // single line label
    {
        // Note: we must have DT_SINGLELINE for DT_VCENTER to work.
        ::DrawText(hdc, text, text.length(), pRect,
                   DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    }

    SetBkMode(hdc, modeOld);
    SetTextColor(hdc, colOld);
}

static void DrawRect(HDC hdc, const RECT& r)
{
    wxDrawLine(hdc, r.left, r.top, r.right, r.top);
    wxDrawLine(hdc, r.right, r.top, r.right, r.bottom);
    wxDrawLine(hdc, r.right, r.bottom, r.left, r.bottom);
    wxDrawLine(hdc, r.left, r.bottom, r.left, r.top);
}

static void DrawButtonFrame(HDC hdc, const RECT& rectBtn,bool selected, bool pushed,bool On)
{
    RECT r;
    CopyRect(&r, &rectBtn);

    HPEN hpenBlack   = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW)),
         hpenGrey    = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW)),
         hpenLightGr = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT)),
         hpenWhite   = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));

    HPEN hpenOld = (HPEN)SelectObject(hdc, hpenBlack);

    r.right--;
    r.bottom--;

    if ( pushed )
    {
        DrawRect(hdc, r);

        (void)SelectObject(hdc, hpenGrey);
        ::InflateRect(&r, -1, -1);

        DrawRect(hdc, r);
    }
    else // !pushed
    {
        if ( selected )
        {
            DrawRect(hdc, r);

            ::InflateRect(&r, -1, -1);
        }

        if (On)
        {
            ::InflateRect(&r, -1, -1);

            (void)SelectObject(hdc, hpenWhite);
            wxDrawLine(hdc, r.left, r.bottom, r.right, r.bottom);
            wxDrawLine(hdc, r.right, r.bottom, r.right, r.top - 1);

            (void)SelectObject(hdc, hpenBlack);
            wxDrawLine(hdc, r.left, r.bottom - 1, r.left, r.top);
            wxDrawLine(hdc, r.left, r.top, r.right, r.top);

            (void)SelectObject(hdc, hpenGrey);
            wxDrawLine(hdc, r.left + 1, r.bottom - 2, r.left + 1, r.top + 1);
            wxDrawLine(hdc, r.left + 1, r.top + 1, r.right - 1, r.top + 1);

            (void)SelectObject(hdc, hpenLightGr);
            wxDrawLine(hdc, r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1);
            wxDrawLine(hdc, r.right - 1, r.bottom - 1, r.right - 1, r.top);
        }
        else
        {
            wxDrawLine(hdc, r.left, r.bottom, r.right, r.bottom);
            wxDrawLine(hdc, r.right, r.bottom, r.right, r.top - 1);

            (void)SelectObject(hdc, hpenWhite);
            wxDrawLine(hdc, r.left, r.bottom - 1, r.left, r.top);
            wxDrawLine(hdc, r.left, r.top, r.right, r.top);

            (void)SelectObject(hdc, hpenLightGr);
            wxDrawLine(hdc, r.left + 1, r.bottom - 2, r.left + 1, r.top + 1);
            wxDrawLine(hdc, r.left + 1, r.top + 1, r.right - 1, r.top + 1);

            (void)SelectObject(hdc, hpenGrey);
            wxDrawLine(hdc, r.left + 1, r.bottom - 1, r.right - 1, r.bottom - 1);
            wxDrawLine(hdc, r.right - 1, r.bottom - 1, r.right - 1, r.top);
        }
    }

    (void)SelectObject(hdc, hpenOld);
    DeleteObject(hpenWhite);
    DeleteObject(hpenLightGr);
    DeleteObject(hpenGrey);
    DeleteObject(hpenBlack);
}


//--------------------------------------------------------------------------------
void CIwStyleButtonOD::OnPaint(wxPaintEvent& event)
{
    wxPaintDC wxdc(this);
    HDC hdc=GetHdcOf(wxdc);
    wxSize Size=GetClientSize();
    RECT rectBtn={0,0,Size.x,Size.y};

    COLORREF colBg = wxColourToRGB(GetBackgroundColour()),
             colFg = wxColourToRGB(GetForegroundColour());

    HBRUSH hbrushBackground = ::CreateSolidBrush(m_On ? GetSysColor(COLOR_SCROLLBAR) : colBg);

    FillRect(hdc, &rectBtn, hbrushBackground);

    bool selected = false;
    bool pushed = (SendMessage(GetHwnd(), BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;

    DrawButtonFrame(hdc, rectBtn, selected, pushed, m_On);

    if ( pushed || m_On )
        // the label is shifted by 1 pixel to create "pushed" effect
        OffsetRect(&rectBtn, 1, 1);

    DrawButtonText(hdc, &rectBtn, GetLabel(),!IsEnabled() ? GetSysColor(COLOR_GRAYTEXT) : colFg);

    ::DeleteObject(hbrushBackground);
}


//--------------------------------------------------------------------------------
void CIwStyleLabelOD::OnPaint(wxPaintEvent& event)
{
    wxPaintDC wxdc(this);
    HDC hdc=GetHdcOf(wxdc);
    wxSize Size=GetClientSize();
    RECT rectBtn={0,0,Size.x,Size.y};

    COLORREF colBg = wxColourToRGB(GetBackgroundColour()),
             colFg = wxColourToRGB(GetForegroundColour());

    HBRUSH hbrushBackground = ::CreateSolidBrush(colBg);

    FillRect(hdc, &rectBtn, hbrushBackground);

    COLORREF colOld = SetTextColor(hdc,!IsEnabled() ? GetSysColor(COLOR_GRAYTEXT) : colFg);
    int modeOld = SetBkMode(hdc, TRANSPARENT);

    wxFont font=CIwStyleHeader::GetFont();
    SelectObject(hdc,font.GetHFONT());

    ::DrawText(hdc,GetLabel(),GetLabel().length(),&rectBtn,DT_SINGLELINE|DT_RIGHT|DT_VCENTER);

    SetBkMode(hdc, modeOld);
    SetTextColor(hdc, colOld);

    ::DeleteObject(hbrushBackground);
}

//--------------------------------------------------------------------------------
void CIwStyleButtonOD::OnMouse(wxMouseEvent& event)
{
    event.Skip(true);
    Refresh();
}
//--------------------------------------------------------------------------------
void CIwStyleCheckOD::OnMouse(wxMouseEvent& event)
{
    event.Skip(true);
    Refresh();
}

//--------------------------------------------------------------------------------
void CIwStyleCheckOD::OnPaint(wxPaintEvent& event)
{
    wxPaintDC wxdc(this);
    HDC hdc=GetHdcOf(wxdc);
    wxSize Size=GetClientSize();
    RECT rect={0,0,Size.x,Size.y};
    RECT rectCheck,
         rectLabel;
    rectCheck.top =
        rectLabel.top = rect.top;
    rectCheck.bottom =
        rectLabel.bottom = rect.bottom;
    const int checkSize = GetBestSize().y;
    const int MARGIN = 3;

    rectCheck.left = rect.left;
    rectCheck.right = rectCheck.left + checkSize;

    rectLabel.left = rectCheck.right + MARGIN;
    rectLabel.right = rect.right;

    UINT state = DFCS_BUTTONCHECK;
    if ( !IsEnabled() )
        state |= DFCS_INACTIVE;

    switch ( Get3StateValue() )
    {
    case wxCHK_CHECKED:
        state |= DFCS_CHECKED;
        break;

    case wxCHK_UNDETERMINED:
        state |= DFCS_PUSHED;
        break;

    default:
        wxFAIL_MSG( _T("unexpected Get3StateValue() return value") );
    // fall through

    case wxCHK_UNCHECKED:
        // no extra styles needed
        break;
    }

    HBRUSH hbrushBackground = ::CreateSolidBrush(wxColourToRGB(GetBackgroundColour()));

    FillRect(hdc, &rect, hbrushBackground);

    if ( !::DrawFrameControl(hdc, &rectCheck, DFC_BUTTON, state) )
        wxLogLastError(_T("DrawFrameControl(DFC_BUTTON)"));

    // draw the text
    const wxString& label = GetLabel();

    // first we need to measure it
    UINT fmt = DT_NOCLIP;

    if ( !IsEnabled() )
        ::SetTextColor(hdc, ::GetSysColor(COLOR_GRAYTEXT));

    wxFont font=CIwStyleHeader::GetFont();
    SelectObject(hdc,font.GetHFONT());

    if ( !::DrawText(hdc, label, label.length(), &rectLabel, fmt) )
        wxLogLastError(_T("DrawText()"));

    ::DeleteObject(hbrushBackground);
}
#endif

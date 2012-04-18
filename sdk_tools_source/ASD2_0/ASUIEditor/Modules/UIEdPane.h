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
#ifndef UIEDPANE_H
#define UIEDPANE_H

//------------------------------------------------------------------------------
class CIwUIEdFrame : public wxPanel
{
    CIwHost& m_Host;
    bool m_Started;
    bool mCapturingMouse;
    enum { CTRLID_REMOVE };
public:
    int m_OldMode;
    bool m_OnDrag;
    bool m_Focussed;
    wxMenu* m_PopupMenu;
public:
    CIwUIEdFrame(CIwHost& host,wxWindow *parent,wxSize& sz) : wxPanel(parent,wxID_ANY,wxPoint(-1,-1),sz,wxNO_BORDER|wxWANTS_CHARS),
        m_Host(host),m_Started(false),mCapturingMouse(false),m_OldMode(0),m_OnDrag(false),m_Focussed(false),m_PopupMenu(NULL)
    {
        m_Host.m_Shared.m_ViewerFrame=this;
    }

    ~CIwUIEdFrame()
    {
        m_Host.s_Active=false;
        if (m_PopupMenu!=NULL) delete m_PopupMenu;
    }

    virtual WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);

    void OnMouse(wxMouseEvent& event);
    void OnMouseCaptureChanged( wxMouseCaptureChangedEvent& event );
    void GainFocus(wxMouseEvent& fe);
    void LoseFocus(wxMouseEvent& fe);
    void OnNav(wxNavigationKeyEvent&);

    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnMenu(wxMouseEvent& event);

    void LoadVia();
    void SetViewerSize(int x,int y);

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
class CIwUIEdDropTarget : public wxTextDropTarget
{
    CIwHost& m_Host;
    CIwUIEdFrame* m_Frame;
public:
    static char s_CurrData[256];
public:
    CIwUIEdDropTarget(CIwHost& host,CIwUIEdFrame* frame) : m_Host(host),m_Frame(frame)
    {
    }
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& data)
    {
        char text[256];
        sprintf(text,"%d|%d|%d|%s",0,x,y,s_CurrData);
        if (m_Host.m_Link!=NULL)
            m_Host.m_Link->SetData(CIwViewerUI::MOUSE_POS,text);

        m_Frame->m_OnDrag=false;
        return true;
    }
    virtual wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult def)
    {
        char text[256];
        sprintf(text,"%d|%d|%d|%s",1,x,y,s_CurrData);
        if (m_Host.m_Link!=NULL)
            m_Host.m_Link->SetData(CIwViewerUI::MOUSE_POS,text);

        m_Frame->m_OnDrag=true;

        return wxTextDropTarget::OnEnter(x,y,def);
    }
    virtual void OnLeave()
    {
        char text[256];
        sprintf(text,"%d|%d|%d|%s",2,0,0,"");
        if (m_Host.m_Link!=NULL)
            m_Host.m_Link->SetData(CIwViewerUI::MOUSE_POS,text);

        m_Frame->m_OnDrag=false;

        wxTextDropTarget::OnLeave();
    }
    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
    {
        char text[256];
        sprintf(text,"%d|%d|%d|%s",3,x,y,s_CurrData);
        if (m_Host.m_Link==NULL)
            return wxDragNone;

        m_Host.m_Link->SetData(CIwViewerUI::MOUSE_POS,text);
        return wxDragCopy;
    }
};

class CUIEdPane;

//------------------------------------------------------------------------------
class CUIEdPaneData : public CIwASDData
{
public:
    CUIEdPane* m_Parent;
public:
    CUIEdPaneData() : CIwASDData(UIEDTYPE_PANE) { SetState(CIwASDData::STATE_NOTSAVEABLE,CIwASDData::STATE_NOTSAVEABLE); }
    virtual void CheckSave(bool force);
};

//------------------------------------------------------------------------------
class CUIEdPane : public CIwLayoutElement
{
    CUIEdPaneData m_Data;
public:
    CUIEdPane(CIwHost& host) : m_Host(host) { m_Data.m_Parent=this; }

    // CIwLayoutElement virtual overrides
    void        Load(std::vector<wxString>& argv){}
    void        DoLayout() { m_FramePanel->Layout(); }
    wxWindow*   GetControl() { return m_FramePanel; }
    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList);

    wxSizerItem* Create(wxWindow* parent);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
    void RemoveViewerTemp(wxString path,bool removeAll);
public:
    wxScrolledWindow* m_FramePanel;
    CIwUIEdFrame* m_ViewPanel;
    CIwHost& m_Host;
};

#endif

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
#include "IwUIEdHeader.h"

//------------------------------------------------------------------------------
WXLRESULT CIwUIEdFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    // Forward messages such as S3E_WM_RUNONMAINTHREAD
    if (message >= WM_APP)
        CIwTheHost.ProcessMessage(message, wParam, lParam);

    return wxPanel::MSWWindowProc(message,wParam,lParam);
}
//------------------------------------------------------------------------------
void CIwUIEdFrame::OnMouse(wxMouseEvent& event)
{
    if (CIwTheHost.m_Link==NULL)
        return;

    int mode=128;

    if (!m_OnDrag)
    {
        if (event.LeftIsDown())
            mode|=1;

        if (event.MiddleIsDown())
            mode|=2;

        if (event.RightIsDown())
            mode|=4;

        if (event.ShiftDown())
            mode|=8;

        if (event.ControlDown())
            mode|=16;

        if (event.MetaDown())
            mode|=32;
    }

    int x = event.m_x;
    int y = event.m_y;


    int mouse[]={mode,x,y,0};
    //CIwTheHost.m_Link->SetData(CIwViewerUI::MOUSE_POS,(void*)mouse);

    if (mode!=128 || m_OldMode!=128)
    {
        if ( AcceptsFocus() )
            SetFocus();

        if (!mCapturingMouse)
        {
            CaptureMouse();
            mCapturingMouse = true;
        }
    }
    else
    {
        if (mCapturingMouse)
        {
            mCapturingMouse = false;
            if (HasCapture())
                ReleaseMouse();
        }
    }

    m_OldMode=mode;
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::OnMouseCaptureChanged( wxMouseCaptureChangedEvent& event )
{
    if (mCapturingMouse)
    {
        mCapturingMouse = false;
        if (HasCapture())
            ReleaseMouse();
    }
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::GainFocus(wxMouseEvent& fe)
{
    m_Focussed=true;
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::LoseFocus(wxMouseEvent& fe)
{
    m_Focussed=false;
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::OnNav(wxNavigationKeyEvent& e)
{
    e.Skip();
}

char CIwUIEdDropTarget::s_CurrData[256]="";

//------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwUIEdFrame, wxPanel)
    EVT_ENTER_WINDOW(CIwUIEdFrame::GainFocus)
    EVT_LEAVE_WINDOW(CIwUIEdFrame::LoseFocus)

    //EVT_MOUSE_EVENTS(CIwUIEdFrame::OnMouse)
    //EVT_MOUSE_CAPTURE_CHANGED(CIwUIEdFrame::OnMouseCaptureChanged)
    //EVT_SET_FOCUS(CIwUIEdFrame::GainFocus)
    //EVT_KILL_FOCUS(CIwUIEdFrame::LoseFocus)
    EVT_NAVIGATION_KEY(CIwUIEdFrame::OnNav)

    EVT_KEY_DOWN(CIwUIEdFrame::OnKeyDown)
    EVT_KEY_UP(CIwUIEdFrame::OnKeyUp)
    EVT_CHAR(CIwUIEdFrame::OnChar)
    EVT_RIGHT_DOWN(CIwUIEdFrame::OnMenu)
END_EVENT_TABLE()


//------------------------------------------------------------------------------
void CIwUIEdFrame::OnMenu(wxMouseEvent& event)
{
    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    m_PopupMenu=CIwTheFrame->MakeMenu(CIwTheFrame->m_MainMenu,L"&Edit",this);

    PopupMenu(m_PopupMenu);
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode()==WXK_DELETE)
        CIwTheHost.m_KeyDown=127;

    event.Skip();
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::OnChar(wxKeyEvent& event)
{
    if (event.GetKeyCode()<=128)
        CIwTheHost.m_KeyDown=event.GetKeyCode();

    event.Skip();
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::OnKeyUp(wxKeyEvent& event)
{
    CIwTheHost.m_KeyDown=0;
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::LoadVia()
{
    wxSetWorkingDirectory(m_Host.m_Cwd);

    if (m_Started)
    {

        CIwTheHost.m_Shared.SaveProject();
        //char text[256];
        //if(CIwTheHost.m_Link!=NULL)
        //	CIwTheHost.m_Link->SetData(CIwViewerUI::RESET_UI,text);

        m_Host.End(true);
        return;
    }

    wxString via;
    int i;

    for (i=1; i<CIwTheApp->argc; i++)
    {
        wxString arg=CIwTheApp->argv[i];
        if (arg.StartsWith(L"-via:",&via))
            break;
    }
    if (via.empty())
        via=L"data/_viewertemp.via";

    m_Host.Setup(via,m_Host.m_Shared.m_Project.m_Group,m_Host.m_Shared.m_Project.m_RootDir,this);
    m_Host.Start();
    m_Started=true;

    CIwTheFrame->m_StatusWindow->SetStatusText( _T("Loading...") );
}

//------------------------------------------------------------------------------
void CIwUIEdFrame::SetViewerSize(int x,int y)
{
    m_Host.m_Shared.m_Project.m_Width=x;
    m_Host.m_Shared.m_Project.m_Height=y;
    m_Host.m_Shared.m_Project.SetChanged(true);

    if (!m_Host.End(true))
        return;
}

//------------------------------------------------------------------------------
wxSizerItem* CUIEdPane::Create(wxWindow* parent)
{
    //m_FramePanel=new wxPanel(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxNO_BORDER|wxWANTS_CHARS);
    m_FramePanel=new wxScrolledWindow(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxNO_BORDER|wxWANTS_CHARS);

    wxSize size(m_Host.m_Shared.m_Project.m_Width,m_Host.m_Shared.m_Project.m_Height);
    m_ViewPanel=new CIwUIEdFrame(m_Host,m_FramePanel,size);
    m_ViewPanel->SetSizeHints(-1,-1,-1,-1);
    m_ViewPanel->SetDropTarget(new CIwUIEdDropTarget(m_Host,(CIwUIEdFrame*)m_ViewPanel));

    wxSizer* sizer=new wxBoxSizer(wxHORIZONTAL);
    m_FramePanel->SetSizer(sizer);
    //sizer->AddStretchSpacer();
    sizer->Add(m_ViewPanel,0,wxALIGN_TOP);
    //sizer->AddStretchSpacer();

    m_ViewPanel->SetSize(size);
    m_FramePanel->SetBackgroundColour(0xFF3f3f3f);
    m_ViewPanel->SetBackgroundColour(0xFF000000);

    m_FramePanel->SetAutoLayout(true);
    m_FramePanel->SetScrollRate(20,20);
    m_FramePanel->SetVirtualSize(size);

    wxStatusBar* sb = CIwTheFrame->m_StatusWindow->CreateStatusBar();
    static int ws[6]={-1,80,80,80,80,120};
    sb->SetFieldsCount(6,ws);
    CIwTheFrame->m_StatusWindow->SetStatusText( _T("") );

    return new wxSizerItem(m_FramePanel,1,wxEXPAND|wxALL,0,NULL);
}

#define VIEWER_TEMP L"_viewertemp"

//--------------------------------------------------------------------------------
void CUIEdPane::RemoveViewerTemp(wxString path,bool removeAll)
{
#ifdef _DEBUG
    return;
#endif

    if (!path.EndsWith(L"\\") && !path.EndsWith(L"/"))
        path.Append(L"/");

    wxDir dir(path);

    wxString file;
    bool doing=dir.GetFirst(&file,L"",wxDIR_FILES);

    while (doing)
    {
        if (file.Lower().StartsWith(VIEWER_TEMP) || removeAll)
        {
#ifdef I3D_OS_WINDOWS
            SetFileAttributes((path+file).c_str(),FILE_ATTRIBUTE_NORMAL);
#else
            chmod((path+file).mb_str(),S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif
            wxRemoveFile(path+file);
        }

        doing=dir.GetNext(&file);
    }
    doing=dir.GetFirst(&file,L"",wxDIR_DIRS);

    while (doing)
    {
        if (file.IsSameAs(VIEWER_TEMP,false) || removeAll)
        {
            RemoveViewerTemp(path+file,true);
            wxFileName::Rmdir(path+file);
        }
        else
            RemoveViewerTemp(path+file,false);

        doing=dir.GetNext(&file);
    }
}

//------------------------------------------------------------------------------
void CUIEdPane::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    dataList.push_back(&m_Data);
}

//------------------------------------------------------------------------------
void CUIEdPaneData::CheckSave(bool force)
{
    char text[256]="";

    CIwTheHost.m_Restarting=false;
    if (CIwTheHost.m_Link!=NULL && CIwTheHost.m_InViewer)
    {
        CIwTheHost.m_Link->SetData(CIwViewerUI::SHUTDOWN,text);

        CIwTheHost.m_ShutdownSemaphore.WaitTimeout(100);
    }

#ifdef I3D_OS_WINDOWS
    m_Parent->RemoveViewerTemp(CIwTheHost.m_Shared.m_Project.m_RootDir,false);
#endif
}

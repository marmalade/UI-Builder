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
#include "IwASDBase.h"

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutPanel, wxPanel)
    EVT_RIGHT_DOWN(CIwLayoutPanel::OnMouseClick)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutSash, wxSashLayoutWindow)
    EVT_RIGHT_DOWN(CIwLayoutSash::OnMouseClick)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutNotebook, wxNotebook)
    EVT_MOUSE_EVENTS(CIwLayoutNotebook::OnMouse)
    EVT_NOTEBOOK_PAGE_CHANGED(CTRLID_NOTEBOOK,CIwLayoutNotebook::OnChanged)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDnDPageTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    wxDragResult Result=m_Elem->DoDrop(Pt2,def);
    if (Result==wxDragNone && m_Elem->m_Parent!=NULL)
        Result=m_Elem->m_Parent->DoDrop(Pt2,def,m_Elem);

    return Result;
}

//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDnDPageTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    CIwLayoutDnDObject* Data=(CIwLayoutDnDObject*)GetDataObject();
    if (Data==NULL)
        return wxDragNone;

    wxDragResult Result=m_Elem->DoDrop(Pt2,def,Data);
    if (Result==Result && m_Elem->m_Parent!=NULL)
        Result=m_Elem->m_Parent->DoDrop(Pt2,def,m_Elem,Data);

    return Result;
}

//--------------------------------------------------------------------------------
CIwLayoutPanel::CIwLayoutPanel(wxWindow* parent,CIwLayoutElement* Elem,int style) : wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),style),m_Elem(Elem)
{
    SetDropTarget(new CIwLayoutDnDPageTarget(Elem,this));
}

//--------------------------------------------------------------------------------
void CIwLayoutPanel::OnMouseClick(wxMouseEvent& event)
{
    CIwTheFrame->OnRMouse(m_Elem);
}

//--------------------------------------------------------------------------------
CIwLayoutSash::CIwLayoutSash(wxWindow* parent,CIwLayoutElement* Elem,int Id) : wxSashLayoutWindow(parent,Id,wxPoint(-1,-1),wxSize(-1,-1)),m_Elem(Elem)
{
    SetDropTarget(new CIwLayoutDnDPageTarget(Elem,this));
}

//--------------------------------------------------------------------------------
void CIwLayoutSash::OnMouseClick(wxMouseEvent& event)
{
    CIwTheFrame->OnRMouse(m_Elem);
}

//--------------------------------------------------------------------------------
CIwLayoutNotebook::CIwLayoutNotebook(wxWindow* parent,CIwLayoutElementNotebook* Elem) :
    wxNotebook(parent,CTRLID_NOTEBOOK,wxPoint(-1,-1),wxSize(-1,-1)),m_Elem(Elem),m_Curr(-1)
{
    SetDropTarget(new CIwLayoutDnDPageTarget(Elem,this));

    wxString path=CIwTheApp->MakeAbsoluteFilename(L"{viewer}");
    path.Replace(L"\\",L"/");

    wxImageList* List=new wxImageList(14,16,true,5);

    wxBitmap bmp(path+L"normal.bmp",wxBITMAP_TYPE_BMP);
    List->Add(bmp,wxColour(L"WHITE"));

    bmp.LoadFile(path+L"disabled.bmp",wxBITMAP_TYPE_BMP);
    List->Add(bmp,wxColour(L"WHITE"));

    bmp.LoadFile(path+L"rollOver.bmp",wxBITMAP_TYPE_BMP);
    List->Add(bmp,wxColour(L"WHITE"));

    bmp.LoadFile(path+L"clicked.bmp",wxBITMAP_TYPE_BMP);
    List->Add(bmp,wxColour(L"WHITE"));

    bmp.LoadFile(path+L"disabled.bmp",wxBITMAP_TYPE_BMP);
    List->Add(bmp,wxColour(L"WHITE"));

    AssignImageList(List);

    Update();
}

//--------------------------------------------------------------------------------
void CIwLayoutNotebook::Update()
{
    if (GetPageCount()!=m_Elem->m_Children.size())
        return;

    wxPoint Pt=wxGetMousePosition();
    Pt=ScreenToClient(Pt);
    int Hit=HitTest(Pt);

    for (int i=0; i<(int)GetPageCount(); i++)
    {
        if (i==GetSelection())
        {
            if (m_Elem->m_Children[i]->Query(ELEMENT_QUERY_NODRAGNDROP) || m_Elem->m_Children[i]->Query(ELEMENT_QUERY_NOICON))
                SetPageImage(i,-1);
            else if (m_Elem->m_Children[i]->Query(ELEMENT_QUERY_CANNOTCLOSE))
                SetPageImage(i,1);
            else
                SetPageImage(i,0);

            m_Elem->m_Children[i]->Layout();
        }
        else
            SetPageImage(i,-1);

        if (i==Hit)
            SetToolTip(m_Elem->m_Children[i]->GetTitle());
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutNotebook::OnChanged(wxNotebookEvent& event)
{
    m_Elem->m_Children[event.GetSelection()]->Selected();
    event.Skip();
}

//--------------------------------------------------------------------------------
void CIwLayoutNotebook::OnMouse(wxMouseEvent& event)
{
    long flags=0;
    int New=HitTest(event.GetPosition(),&flags);

    if (m_Elem->HandleMouseEvent(event))
        return;

    if (event.Leaving() || m_Curr!=New)
        Update();

    m_Curr=New;
    if (m_Curr==wxNOT_FOUND || m_Curr>=(int)m_Elem->m_Children.size())
    {
        Update();
        return;
    }

    if (event.MiddleDown())
    {
        if (GetPageImage(m_Curr)!=1)
            SetPageImage(m_Curr,3);
    }
    else if (event.MiddleUp())
    {
        if (GetPageImage(m_Curr)!=1)
            m_Elem->RemoveChild(m_Elem->m_Children[m_Curr],true);
    }
    else if (event.RightDown())
        CIwTheFrame->OnRMouse(m_Elem->m_Children[m_Curr]);
    else if (event.LeftDown())
    {
        SetSelection(m_Curr);
        Update();

        if (flags==wxNB_HITTEST_ONICON)
        {
            if (GetPageImage(m_Curr)!=1)
                SetPageImage(m_Curr,3);
        }
        else
            StartDrag(m_Elem->m_Children[m_Curr]);
    }
    else if (event.LeftUp())
    {
        if (flags==wxNB_HITTEST_ONICON)
            if (GetPageImage(m_Curr)!=1)
                m_Elem->RemoveChild(m_Elem->m_Children[m_Curr],true);

    }
    else
    {
        if (flags==wxNB_HITTEST_ONICON)
        {
            if (GetPageImage(m_Curr)==0)
                SetPageImage(m_Curr,2);
        }
        else if (GetPageImage(m_Curr)==2)
            SetPageImage(m_Curr,0);
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutNotebook::StartDrag(CIwLayoutElement* Elem)
{
#ifdef I3D_OS_WINDOWS
    if (Elem->Query(ELEMENT_QUERY_NODRAGNDROP))
#endif
    return;

    CIwTheFrame->m_CurrDnDElem=Elem;
    CIwTheFrame->m_CurrDnDData=NULL;

    CIwLayoutDnDObject Data(Elem);
    wxDropSource Source(Data,this);
    Source.DoDragDrop(wxDrag_DefaultMove);

    CIwTheFrame->m_CurrDnDElem=NULL;
    CIwTheFrame->m_CurrDnDData=NULL;
}

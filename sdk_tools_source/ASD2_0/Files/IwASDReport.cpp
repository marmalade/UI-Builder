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
#include "IwASDFileHeader.h"

//--------------------------------------------------------------------------------
// CIwASDTreePanel
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDReportPanel, wxPanel)
    EVT_LIST_BEGIN_DRAG(CTRLID_LIST,CIwASDReportPanel::OnDrag)
    EVT_LIST_ITEM_RIGHT_CLICK(CTRLID_LIST,CIwASDReportPanel::OnRClick)
    EVT_LIST_ITEM_SELECTED(CTRLID_LIST,CIwASDReportPanel::OnLClick)
    EVT_LIST_KEY_DOWN(CTRLID_LIST,CIwASDReportPanel::OnKeyDown)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
CIwASDReportPanel::CIwASDReportPanel(wxWindow* parent,CIwASDReportLayout* layout) : wxPanel(parent),m_Layout(layout),m_PopupMenu(NULL)
{
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    m_List=new wxListCtrl(this,CTRLID_LIST,wxPoint(-1,-1),wxSize(-1,-1),wxLC_REPORT);
    m_Sizer->Add(m_List,1,wxALL|wxEXPAND,0);

    wxImageList* list=new wxImageList(16,16);
    std::vector<wxString> names;
    CIwTheApp->GetFileTypeInfo(list,m_IconTypes,names,16);
    m_List->AssignImageList(list,wxIMAGE_LIST_SMALL);

    m_Sizer->Layout();

    SetDropTarget(new CIwLayoutDataOrFileTarget(layout,m_List));
}

//--------------------------------------------------------------------------------
void CIwASDReportPanel::OnDrag(wxListEvent& e)
{
    int item=-1;
    CIwTheFrame->m_CurrDnDElem=NULL;
    CIwTheFrame->m_CurrDnDData=NULL;
    CIwLayoutDnDObject Data(m_Layout);
    std::vector<int> sel;
    while (true)
    {
        item=m_List->GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
        if (item==-1) break;

        sel.push_back(item);

        CIwASDData* data2=(CIwASDData*)m_List->GetItemData(item);

        if (CIwTheFrame->m_CurrDnDData==NULL)
            CIwTheFrame->m_CurrDnDData=data2;

        Data.m_Data.push_back(data2);
    }
    if (CIwTheFrame->m_CurrDnDData==NULL)
    {
        CIwASDData* data2=(CIwASDData*)e.GetData();

        CIwTheFrame->m_CurrDnDData=data2;
        Data.m_Data.push_back(data2);
    }

    wxDropSource Source(Data,this);
    wxDragResult res=Source.DoDragDrop(wxDrag_DefaultMove);

    if (res==wxDragCopy) //erase and delete
    {
        for (int i=(int)sel.size()-1; i>=0; i--)
        {
            if (m_Layout->m_InsertPoint!=-1 && sel[i]>m_Layout->m_InsertPoint)
                m_Layout->m_Connection->erase(sel[i]+m_Layout->m_InsertLen,true);
            else
                m_Layout->m_Connection->erase(sel[i],true);
        }

        m_Layout->m_Root->RefreshData(m_Layout->m_Root,true,false);
    }

    if (res==wxDragMove) //signal that we just want to erase
    {
        for (int i=(int)sel.size()-1; i>=0; i--)
        {
            if (m_Layout->m_InsertPoint!=-1 && sel[i]>m_Layout->m_InsertPoint)
                m_Layout->m_Connection->erase(sel[i]+m_Layout->m_InsertLen,false);
            else
                m_Layout->m_Connection->erase(sel[i],false);
        }

        m_Layout->m_Root->RefreshData(m_Layout->m_Root,true,false);
    }

    CIwTheFrame->m_CurrDnDElem=NULL;
    CIwTheFrame->m_CurrDnDData=NULL;
}

//--------------------------------------------------------------------------------
void CIwASDReportPanel::OnRClick(wxListEvent& e)
{
    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    m_PopupMenu=NULL;

    CIwASDData* data2=NULL;
    int flags;
    int item=m_List->HitTest(e.GetPoint(),flags);
    if (item!=-1)
    {
        data2=(CIwASDData*)m_List->GetItemData(item);
        for (int i=0; i<(int)m_List->GetItemCount(); i++)
        {
            if (i!=item)
                m_List->SetItemState(i,0,wxLIST_STATE_SELECTED);
            else
                m_List->SetItemState(i,wxLIST_STATE_SELECTED,wxLIST_STATE_SELECTED);
        }
    }

    m_PopupMenu=CIwTheFrame->MakeMenu(CIwTheFrame->m_RightMenu,m_Layout->m_MenuType,data2);

    if (m_PopupMenu!=NULL)
        PopupMenu(m_PopupMenu);
}

//--------------------------------------------------------------------------------
void CIwASDReportPanel::OnLClick(wxListEvent& e)
{
    CIwASDData* data2=(CIwASDData*)e.GetData();

    std::vector<CIwASDData*> sel;
    int item=-1;
    while (true)
    {
        item=m_List->GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
        if (item==-1) break;

        CIwASDData* data=(CIwASDData*)m_List->GetItemData(item);

        if (data2!=data && data!=NULL)
            sel.push_back(data);
    }

    m_Layout->Selected(data2,sel);
}

//--------------------------------------------------------------------------------
void CIwASDReportPanel::OnKeyDown(wxListEvent& e)
{
    if (e.GetKeyCode()!=WXK_DELETE && e.GetKeyCode()!=WXK_BACK)
        return;

    std::vector<int> sel;
    int item=-1;

    while (true)
    {
        item=m_List->GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
        if (item==-1) break;

        sel.push_back(item);
    }

    for (int i=(int)sel.size()-1; i>=0; i--)
    {
        m_Layout->m_Root->RefreshData((*m_Layout->m_Connection)[sel[i]],false,true);
        m_Layout->m_Connection->erase(sel[i],true);
    }

    m_Layout->m_Root->RefreshData(m_Layout->m_Root,true,false);
}

//--------------------------------------------------------------------------------
int CIwASDReportPanel::GetIcon(unsigned int type)
{
    for (int i=0; i<(int)m_IconTypes.size(); i++)
    {
        if (m_IconTypes[i]==type)
            return i;
    }
    return -1;
}

//--------------------------------------------------------------------------------
// CIwASDTreeLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwASDReportLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwASDReportPanel(parent,this);

    wxSizerItem* sizer=DoCreate();
    if (sizer==NULL)
        sizer=new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);

    return sizer;
}

//--------------------------------------------------------------------------------
void CIwASDReportLayout::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    if (data==m_Root)
    {
        if (base && Delete)
        {
            m_Root=NULL;
            m_Connection=NULL;
        }
    }
    else if (m_Connection!=NULL)
    {
        int j;
        for (j=0; j<(int)m_Connection->size(); j++)
        {
            if (data==(*m_Connection)[j])
                break;
        }
        if (j==(int)m_Connection->size())
            return;
    }

    m_NeedReset=true;
    wxWakeUpIdle();
}

//--------------------------------------------------------------------------------
void CIwASDReportLayout::DoSetData(CIwASDData* data)
{
    if (data->m_Type!=m_RootType) return;

    m_Root=data;
    m_Connection=m_Root->m_Connections[m_ConnectionType];
    Reset();
}

//--------------------------------------------------------------------------------
void CIwASDReportLayout::Reset()
{
    m_InsertPoint=-1;

    m_Panel->m_List->Freeze();

    m_Panel->m_List->DeleteAllItems();
    m_Panel->m_List->DeleteAllColumns();

    if (m_Root==NULL)
    {
        m_Panel->m_List->Thaw();
        return;
    }

    int i,item=0;
    for (i=0; i<(int)m_Columns.size(); i++)
    {
        item=m_Panel->m_List->InsertColumn(item,m_Columns[i].m_Title);
        item++;
    }

    item=0;
    for (i=0; i<(int)m_Connection->size(); i++)
    {
        CIwASDData* data2=(*m_Connection)[i];

        wxColour col(L"BLACK");
        std::map<wxString,wxString> opts;
        data2->GetOptions(opts,col);

        item=m_Panel->m_List->InsertItem(item,ReplaceOptions(m_Columns[0].m_Type,opts),m_Panel->GetIcon(data2->m_Type));
        m_Panel->m_List->SetItemTextColour(item,col);
        m_Panel->m_List->SetItemData(item,(long)data2);

        for (int k=1; k<(int)m_Columns.size(); k++)
        {
            m_Panel->m_List->SetItem(item,k,ReplaceOptions(m_Columns[k].m_Type,opts));
        }

        item++;
    }

    for (int j=0; j<(int)m_Columns.size(); j++)
    {
        m_Panel->m_List->SetColumnWidth(j,m_Columns[j].m_Width);
    }
    m_NeedReset=false;

    m_Panel->m_List->Thaw();
}

/*
   tree1 -> tree1		c c same
   c _ cc
   m e r  wxDragLink
   tree1 -> tree2		c c cross
   c _ cc
   m d cc
   tree1 -> report		c r cross
   c _ r
   m _ r  wxDragCopy
   report -> report	r r same
   c _ r
   m e r  [wxDragLink]
   report -> tree1		r c cross
   c _ cc
   m e cc [wxDragLink]
   tree1 -> derbh		c c change
   c _ cr
   m _ cr wxDragCopy
   derbh -> report		c r change
   c _ _  wxDragNone
   m _ _  wxDragNone
   report -> derbh		r c change
   c _ cr
   m _ cr wxDragCopy
   report -> dreport	r r change
   c _ _  wxDragNone
   m _ _  wxDragNone

   copy same\cross
   C	Cc
   C	R
   R	Cc
   R	R
   copy cross change
   C	Cr
   C	R	wxDragNone
   R	Cr
   R	R	wxDragNone
   move same
   Ce	C	wxDragLink
   Re	R	wxDragLink
   move cross
   Cd	Cc
   C	R	wxDragCopy
   Rd	Cc
   R	R	wxDragCopy
   move cross change
   C	Cr	wxDragCopy
   C	R	wxDragNone
   R	Cr	wxDragCopy
   R	R	wxDragNone
 */
//--------------------------------------------------------------------------------
bool CIwASDReportLayout::DoDataDropStart(wxPoint Pt,wxDragResult& def)
{
    int flags;
    Pt=m_Panel->m_List->ScreenToClient(Pt);
    m_InsertPoint=m_Panel->m_List->HitTest(Pt,flags);
    m_InsertLen=0;

    switch (flags)
    {
    case wxLIST_HITTEST_ABOVE:
    case wxLIST_HITTEST_BELOW:
    case wxLIST_HITTEST_TOLEFT:
    case wxLIST_HITTEST_TORIGHT:
        def=wxDragNone;
        return false;
    }
    if (def==wxDragCopy || def==wxDragMove)
        return true;

    def=wxDragNone;
    return false;
}

//--------------------------------------------------------------------------------
wxDragResult CIwASDReportLayout::DoDataDrop(wxPoint Pt,wxDragResult def,const wxArrayString& files)
{
    if (!DoDataDropStart(Pt,def))
        return def;

    std::vector<CIwASDData*> dataList;

    for (int i=0; i<(int)files.size(); i++)
    {
        CIwASDData* data=GetDataFromFilename(files[i]);
        if (data!=NULL)
            dataList.push_back(data);
    }
    if (dataList.size()<1)
        return wxDragNone;

    return DoDropData(wxDragCopy,dataList,NULL);
}

//--------------------------------------------------------------------------------
wxDragResult CIwASDReportLayout::DoDataDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data)
{
    if (!DoDataDropStart(Pt,def))
        return def;

    if (Data==NULL)
        return def;

    return DoDropData(def,Data->m_Data,Data->m_Elem);
}

//--------------------------------------------------------------------------------
wxDragResult CIwASDReportLayout::DoDropData(wxDragResult def,std::vector<CIwASDData*>& dataList,CIwLayoutElement* elem)
{
    if (elem!=this)
        FilterAndExpand(dataList,m_ExpandTypes,m_FilterShow);

    for (int i=0; i<(int)dataList.size(); i++)
    {
        if (dataList[i]==NULL)
            continue;

        CIwASDData* data=DropDataItem(def,dataList[i],elem,m_InsertType,m_Connection,m_Root);
        if (data==NULL) continue;

        if (m_InsertPoint==-1)
            m_Connection->push_back(data);
        else
            m_Connection->insert(m_InsertPoint,data);

        m_InsertLen++;
    }
    switch (def) //adjust as wxDragLink is not transmitted
    {
    case wxDragCopy:
        def=wxDragNone; break;
    case wxDragMove:
        def=wxDragCopy; break;
    case wxDragLink:
        def=wxDragMove; break;
    default:
        break;
    }

    m_Root->RefreshData(m_Root,true,false);

    return def;
}

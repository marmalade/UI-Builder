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
BEGIN_EVENT_TABLE(CIwASDTreePanel, wxPanel)
    EVT_TREE_ITEM_EXPANDING(CTRLID_TREE,CIwASDTreePanel::OnExpanding)
    EVT_TREE_BEGIN_DRAG(CTRLID_TREE,CIwASDTreePanel::OnDrag)
    EVT_TREE_ITEM_MENU(CTRLID_TREE,CIwASDTreePanel::OnMenu)
    EVT_TREE_SEL_CHANGED(CTRLID_TREE,CIwASDTreePanel::OnSelected)
    EVT_TREE_KEY_DOWN(CTRLID_TREE,CIwASDTreePanel::OnKeyDown)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwASDTreePanel::OnSelected(wxTreeEvent& e)
{
    wxTreeItemId id=e.GetItem();
    if (!id) return;

    if (m_Tree->GetChildrenCount(id,false)>0) return;

    wxArrayTreeItemIds sel;
    std::vector<CIwASDData*> selData;
    m_Tree->GetSelections(sel);

    CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(id);

    for (int i=0; i<(int)sel.size(); i++)
    {
        CIwASDTreeData* data2=(CIwASDTreeData*)m_Tree->GetItemData(sel[i]);
        if (data2!=data && data2!=NULL)
            selData.push_back(data2->m_Data);
    }

    m_Layout->Selected(data->m_Data,selData);
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::OnKeyDown(wxTreeEvent& e)
{
    if (e.GetKeyCode()!=WXK_DELETE && e.GetKeyCode()!=WXK_BACK)
        return;

    wxArrayTreeItemIds sel;
    if (m_Tree->GetSelections(sel)<1) return;

    for (int i=0; i<(int)sel.size(); i++)
    {
        CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(sel[i]);
        if (data==NULL || data->m_Data==NULL) continue;

        m_Layout->Delete(data->m_Data);
    }
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::OnExpanding(wxTreeEvent& e)
{
    wxTreeItemId id=e.GetItem();
    if (!id) return;

    if (m_Tree->GetChildrenCount(id,false)>0) return;

    CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(id);

    m_Layout->Expand(data->m_Data,id);
}
//--------------------------------------------------------------------------------
void CIwASDTreePanel::OnDrag(wxTreeEvent& e)
{
    wxArrayTreeItemIds sel;
    if (m_Tree->GetSelections(sel)<1) return;

    CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(sel[0]);
    CIwLayoutDnDObject Data(data->m_Data);
    for (int i=1; i<(int)sel.size(); i++)
    {
        data=(CIwASDTreeData*)m_Tree->GetItemData(sel[i]);
        Data.m_Data.push_back(data->m_Data);
    }

    CIwTheFrame->m_CurrDnDElem=NULL;
    CIwTheFrame->m_CurrDnDData=data->m_Data;

    wxDropSource Source(Data,this);
    Source.DoDragDrop(wxDrag_DefaultMove);

    CIwTheFrame->m_CurrDnDElem=NULL;
    CIwTheFrame->m_CurrDnDData=NULL;
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::OnMenu(wxTreeEvent& e)
{
    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    m_PopupMenu=NULL;

    CIwASDTreeData* data2=NULL;
    int flags;
    wxPoint pos=wxGetMousePosition();
    pos=m_Tree->ScreenToClient(pos);
    wxTreeItemId item=m_Tree->HitTest(pos,flags);
    if (item.IsOk())
        data2=(CIwASDTreeData*)m_Tree->GetItemData(item);

    m_PopupMenu=CIwTheFrame->MakeMenu(CIwTheFrame->m_RightMenu,m_Layout->m_MenuType,data2->m_Data);

    if (m_PopupMenu!=NULL)
        PopupMenu(m_PopupMenu);
}

//--------------------------------------------------------------------------------
CIwASDTreePanel::CIwASDTreePanel(wxWindow* parent,CIwASDTreeLayout* layout,int style) : wxPanel(parent),m_Layout(layout),m_PopupMenu(NULL)
{
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    m_Tree=new wxTreeCtrl(this,CTRLID_TREE,wxPoint(-1,-1),wxSize(-1,-1),style);
    m_Sizer->Add(m_Tree,1,wxALL|wxEXPAND,0);

    wxImageList* list=new wxImageList(16,16);
    std::vector<wxString> names;
    CIwTheApp->GetFileTypeInfo(list,m_IconTypes,names,16);
    m_Tree->AssignImageList(list);

    m_Sizer->Layout();

    SetDropTarget(new CIwLayoutDataDnDTarget(layout,m_Tree));
}

//--------------------------------------------------------------------------------
int CIwASDTreePanel::GetIcon(unsigned int type)
{
    for (int i=0; i<(int)m_IconTypes.size(); i++)
    {
        if (m_IconTypes[i]==type)
            return i;
    }
    return -1;
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::StoreExpanded()
{
    m_ExpandList.clear();
    StoreExpanded(m_Tree->GetRootItem());
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::RestoreExpanded()
{
    RestoreExpanded(m_Tree->GetRootItem());
    m_ExpandList.clear();
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::StoreExpanded(wxTreeItemId id)
{
    if (!id || !m_Tree->ItemHasChildren(id) || !m_Tree->IsExpanded(id))
        return;

    CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(id);
    m_ExpandList.push_back(data->m_Data);

    wxTreeItemIdValue cookie;
    wxTreeItemId id2=m_Tree->GetFirstChild(id,cookie);
    while (id2)
    {
        StoreExpanded(id2);
        id2=m_Tree->GetNextChild(id,cookie);
    }
}

//--------------------------------------------------------------------------------
void CIwASDTreePanel::RestoreExpanded(wxTreeItemId id)
{
    if (!id)
        return;

    CIwASDTreeData* data=(CIwASDTreeData*)m_Tree->GetItemData(id);

    for (int i=0; i<(int)m_ExpandList.size(); i++)
    {
        if (m_ExpandList[i]==data->m_Data)
        {
            if (!m_Tree->HasFlag(wxTR_HIDE_ROOT) || id!=m_Tree->GetRootItem())
                m_Tree->Expand(id);
            else
                m_Layout->Expand(data->m_Data,id);

            wxTreeItemIdValue cookie;
            wxTreeItemId id2=m_Tree->GetFirstChild(id,cookie);
            while (id2)
            {
                RestoreExpanded(id2);
                id2=m_Tree->GetNextChild(id,cookie);
            }
            break;
        }
    }
}

//--------------------------------------------------------------------------------
// CIwASDTreeLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwASDTreeLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwASDTreePanel(parent,this,wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_MULTIPLE);

    wxSizerItem* sizer=DoCreate();
    if (sizer==NULL)
        sizer=new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);

    return sizer;
}

//--------------------------------------------------------------------------------
bool CIwASDTreeLayout::Find(CIwASDData* data,CIwASDData* from)
{
    if (data==from)
        return true;

    for (int i=0; i<(int)m_ConnectionTypes.size(); i++)
    {
        if (from->m_Connections.find(m_ConnectionTypes[i])!=from->m_Connections.end())
        {
            CIwASDDataConnection* conx=from->m_Connections[m_ConnectionTypes[i]];
            for (int j=0; j<(int)conx->size(); j++)
            {
                if (Find(data,(*conx)[j]))
                    return true;
            }
        }
    }

    return false;
}

//--------------------------------------------------------------------------------
void CIwASDTreeLayout::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    if (m_NeedReset) return;

    if (data==m_Root && base && Delete)
        m_Root=NULL;

    if (!Find(data,GetRoot()))
        return;

    m_NeedReset=true;
    wxWakeUpIdle();
}

//--------------------------------------------------------------------------------
void CIwASDTreeLayout::DoSetData(CIwASDData* data)
{
    if (data->m_Type!=m_RootType) return;

    m_Root=data;
    Reset();
}

//--------------------------------------------------------------------------------
bool CIwASDTreeLayout::HasChildren(CIwASDData* data)
{
    bool found=false;
    for (int i=0; i<(int)m_ConnectionTypes.size(); i++)
    {
        if (data->m_Connections.find(m_ConnectionTypes[i])!=data->m_Connections.end())
            if (data->m_Connections[m_ConnectionTypes[i]]->size()>0)
                found=true;

    }

    return found;
}

//--------------------------------------------------------------------------------
wxTreeItemId CIwASDTreeLayout::MakeRootNode()
{
    wxColour col(L"BLACK");
    std::map<wxString,wxString> opts;
    GetRoot()->GetOptions(opts,col);

    wxTreeItemId id=m_Panel->m_Tree->AddRoot(ReplaceOptions(m_TitleType,opts),
                                             m_Panel->GetIcon(GetRoot()->m_Type),-1,new CIwASDTreePanel::CIwASDTreeData(GetRoot()));
    if (!id) return id;

    m_Panel->m_Tree->SetItemTextColour(id,col);

    return id;
}

//--------------------------------------------------------------------------------
void CIwASDTreeLayout::Reset()
{
    m_NeedReset=false;

    bool delOnly=(m_Root==NULL);
    if (!delOnly)
        m_Panel->StoreExpanded();

    m_Panel->m_Tree->DeleteAllItems();

    if (delOnly) return;

    wxTreeItemId id=MakeRootNode();
    if (!id) return;

    m_Panel->m_Tree->SetItemHasChildren(id,HasChildren(GetRoot()));
    m_Panel->RestoreExpanded();
    m_Panel->m_Tree->Expand(id);
}

//--------------------------------------------------------------------------------
void CIwASDTreeLayout::Expand(CIwASDData* data,wxTreeItemId id)
{
    for (int i=0; i<(int)m_ConnectionTypes.size(); i++)
    {
        if (data->m_Connections.find(m_ConnectionTypes[i])!=data->m_Connections.end())
        {
            for (int j=0; j<(int)data->m_Connections[m_ConnectionTypes[i]]->size(); j++)
            {
                CIwASDData* data2=(*data->m_Connections[m_ConnectionTypes[i]])[j];

                wxColour col(L"BLACK");
                std::map<wxString,wxString> opts;
                data2->GetOptions(opts,col);
                wxTreeItemId id2=m_Panel->m_Tree->AppendItem(id,ReplaceOptions(m_TitleType,opts),
                                                             m_Panel->GetIcon(data2->m_Type),-1,new CIwASDTreePanel::CIwASDTreeData(data2));

                m_Panel->m_Tree->SetItemTextColour(id2,col);
                m_Panel->m_Tree->SetItemHasChildren(id2,HasChildren(data2));
            }
        }
    }
}

//--------------------------------------------------------------------------------
wxDragResult CIwASDTreeLayout::DoDataDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data)
{
    if (Data==NULL)
        return wxDragNone;

    m_NeedReset=true;
    wxWakeUpIdle();
    return def;
}

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
BEGIN_EVENT_TABLE(CIwUIEdHierFrame, wxTreeCtrl)
    EVT_TREE_SEL_CHANGED(CTRLID_TREE,CIwUIEdHierFrame::OnSelect)
    EVT_RIGHT_DOWN(CIwUIEdHierFrame::OnMenu)
    EVT_MENU(CTRLID_REMOVE,CIwUIEdHierFrame::OnRemove)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
void CIwUIEdHierFrame::OnMenu(wxMouseEvent& e)
{
    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    currData=NULL;
    int flags;
    wxPoint pos=wxGetMousePosition();
    pos=ScreenToClient(pos);
    wxTreeItemId item=HitTest(pos,flags);
    if (item.IsOk())
    {
        CIwAttrTreeItem* data2=(CIwAttrTreeItem*)GetItemData(item);
        if (data2!=NULL)
            currData=data2->m_Inst;
    }

    //temp
    if (currData==NULL) return;

    m_PopupMenu=new wxMenu;

    if (currData!=NULL)
        //m_PopupMenu->AppendSeparator();
        m_PopupMenu->Append(CTRLID_REMOVE,L"&Remove",L"remove the current element",false);

    PopupMenu(m_PopupMenu);
}

//------------------------------------------------------------------------------
void CIwUIEdHierFrame::OnRemove(wxCommandEvent&)
{
    if (currData==NULL) return;

    m_Shared->Change(currData,CHANGETYPE_REMOVE);

    /*m_Selecting=true;

       CIwAttrInstance* inst;
       CIwAttrInstance* parent=m_Shared->Remove(currData);

       for(inst=m_Shared->m_CurrElem;inst->m_Parent!=NULL;inst=inst->m_Parent->m_Instance)
        if(inst==currData) {
            m_Shared->m_CurrElem=parent;
            break;
        }
       if(m_Shared->m_CurrElem==m_Shared->m_CurrUI->m_Group.m_Inst)
        m_Shared->m_CurrElem=m_Shared->GetFirst();

       for(inst=m_Shared->m_SelElem;inst->m_Parent!=NULL;inst=inst->m_Parent->m_Instance)
        if(inst==currData) {
            m_Shared->m_SelElem=parent;
            break;
        }
       if(m_Shared->m_SelElem==m_Shared->m_CurrUI->m_Group.m_Inst)
        m_Shared->m_SelElem=m_Shared->GetFirst();

       Reset();
       m_Shared->RefreshCurr();

       if(parent==m_Shared->m_CurrUI->m_Group.m_Inst)
        m_Shared->SendChanges(NULL);
       else
        m_Shared->SendChanges(parent);

       m_Selecting=false;*/
    currData=NULL;
}

//------------------------------------------------------------------------------
bool CIwUIEdHierDropTarget::OnDropText(wxCoord x, wxCoord y, const wxString& data)
{
    CIwAttrInstance* inst=m_Frame->m_Shared->m_Project.m_Palette.GetItem(data);
    CIwAttrInstance* parent=m_Frame->m_Shared->m_CurrUI->m_Group.m_Inst;
    if (inst==NULL)
        return false;

    int flags;
    wxTreeItemId id=m_Frame->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
    {
        CIwAttrTreeItem *data=(CIwAttrTreeItem*)m_Frame->GetItemData(id);
        if (data!=NULL && data->m_Inst!=NULL)
            parent=data->m_Inst;
    }

    m_Frame->m_Shared->Change(inst,CHANGETYPE_ADD,parent);

    /*m_Frame->m_Selecting=true;

       CIwAttrInstance* newInst=m_Frame->m_Shared->m_CurrUI->Add(inst,parent);
       if(parent==m_Frame->m_Shared->m_CurrUI->m_Group.m_Inst)
        m_Frame->m_Shared->m_CurrElem=newInst;
       else
        m_Frame->m_Shared->m_CurrElem=parent;
       m_Frame->m_Shared->m_SelElem=newInst;

       m_Frame->Reset();
       m_Frame->m_Shared->RefreshCurr();
       m_Frame->m_Selecting=false;

       m_Frame->m_Shared->SendChanges(newInst);*/
    return true;
}

//------------------------------------------------------------------------------
wxDragResult CIwUIEdHierDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    if (m_Frame->m_Shared->m_CurrUI==NULL)
        return wxDragNone;

    if (lastId.IsOk())
        m_Frame->SetItemDropHighlight(lastId,false);

    int flags;
    wxTreeItemId id=m_Frame->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
        m_Frame->SetItemDropHighlight(id,true);

    lastId=id;

    return wxDragCopy;
}

//------------------------------------------------------------------------------
void CIwUIEdHierFrame::OnSelect(wxTreeEvent& e)
{
    if (m_Shared->m_Selecting) return;

    CIwAttrTreeItem *data=(CIwAttrTreeItem*)GetItemData(e.GetItem());
    if (data==NULL || data->m_Inst==NULL)
        return;

    //m_Shared->SetSelection(data->m_Inst,SELSOURCE_HIER);
    /*
        m_Selecting=true;
        m_Shared->m_CurrElem=data->m_Inst;
        m_Shared->RefreshCurr();
        m_Shared->SetSelected(data->m_Inst,false);
        m_Selecting=false;

        wxString name=m_Shared->GetFullName(data->m_Inst);

        char text[256];
        strcpy(text,name.mb_str());
        if(CIwTheHost.m_Link!=NULL)
            CIwTheHost.m_Link->SetData(CIwViewerUI::CURRENT_ELEMENT,text);*/
}

//------------------------------------------------------------------------------
void CIwUIEdHierFrame::Add(wxTreeItemId parent,CIwAttrInstance* inst)
{
    int i;
    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwUIElement");

    for (i=0; i<(int)inst->m_Data.size(); i++)
    {
        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        CIwAttrInstance* inst2=inst->m_Data[i]->m_Items[0].m_Inst;
        if (inst2==NULL)
            continue;

        CIwAttrClass* klass2;

        for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
        {
            if (klass2==klass)
                break;
        }
        if (klass2==NULL)
            continue;

        wxTreeItemId id=AppendItem(parent,inst2->GetTreeName(),-1,-1,new CIwAttrTreeItem(inst2));
        Add(id,inst2);
    }
    /*if(m_Shared->m_SelElem==inst && parent!=GetRootItem()) {
        SelectItem(parent);
        for(wxTreeItemId id=parent;id!=GetRootItem();id=GetItemParent(id))
            Expand(id);
       }*/
}

//------------------------------------------------------------------------------
CIwUIEdHierFrame::CIwUIEdHierFrame(wxWindow *parent,CUIEdAttrShared* shared) :
    CIwUIEdTreeFrame(parent,CTRLID_TREE,shared),m_Selecting(false),currData(NULL)
{
    SetDropTarget(new CIwUIEdHierDropTarget(this));

    wxTreeItemId root=AddRoot(L"");
    AppendItem(root,L"<Open a UI file first>");
}

//------------------------------------------------------------------------------
void CIwUIEdHierFrame::Reset()
{
    bool empty=GetCount()<2;
    std::vector<wxString> list;
    GetOpened(list,GetRootItem());

    DeleteAllItems();
    wxTreeItemId root=AddRoot(L"");
    if (m_Shared->m_CurrUI!=NULL && m_Shared->m_CurrUI->m_Group.m_Inst!=NULL)
        Add(root,m_Shared->m_CurrUI->m_Group.m_Inst);

    if (GetCount()==0)
        AppendItem(root,L"<Drop CIwUIElement(s) Here>");

    int offset=0;
    if (empty)
        SetOpenedAll(GetRootItem());
    else
        SetOpened(list,offset,GetRootItem());
}

//------------------------------------------------------------------------------
wxSizerItem* CUIEdHierPane::Create(wxWindow* parent)
{
    m_Tree=new CIwUIEdHierFrame(parent,m_Shared);

    return new wxSizerItem(m_Tree,1,wxEXPAND|wxALL,0,NULL);
}

//------------------------------------------------------------------------------
void CUIEdHierPane::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Shared->m_CurrUI==NULL) return;

    if (m_Shared->m_CurrUI->HasChanged())
        dataList.push_back(m_Shared->m_CurrUI);
}

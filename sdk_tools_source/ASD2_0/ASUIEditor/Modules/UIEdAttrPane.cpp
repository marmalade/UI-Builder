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

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDUIEdAttrPanel, wxPanel)
    EVT_BUTTON(CTRLID_SAVE,CIwASDUIEdAttrPanel::OnSave)
    EVT_BUTTON(CTRLID_RELOAD,CIwASDUIEdAttrPanel::OnReset)
    EVT_BUTTON(CTRLID_REFRESH,CIwASDUIEdAttrPanel::OnRefresh)
    EVT_BUTTON(CTRLID_REMOVE,CIwASDUIEdAttrPanel::OnRemove)
    EVT_BUTTON(CTRLID_CHECKNAMES,CIwASDUIEdAttrPanel::OnCheckNames)
    EVT_MENU(CTRLID_REMOVE,CIwASDUIEdAttrPanel::OnRemove2)
    //EVT_IDLE(CIwASDUIEdAttrPanel::OnIdle)
    EVT_CHECKBOX(CTRLID_LIVEUPDATE,CIwASDUIEdAttrPanel::OnLiveUpdate)
    EVT_COMBOBOX(CTRLID_SELECTOR,CIwASDUIEdAttrPanel::OnSelector)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDUIEdAttrPanel::CTree, wxTreeCtrl)
    EVT_LEFT_UP(CIwASDUIEdAttrPanel::CTree::OnMouseUp)
    EVT_TREE_SEL_CHANGED(CTRLID_TREE,CIwASDUIEdAttrPanel::CTree::OnItemSelected)
    EVT_TREE_BEGIN_DRAG(CTRLID_TREE,CIwASDUIEdAttrPanel::CTree::OnDrag)
    //EVT_TREE_ITEM_ACTIVATED(CTRLID_TREE,CIwASDUIEdAttrPanel::CTree::OnItemActivated)
    EVT_TREE_ITEM_RIGHT_CLICK(CTRLID_TREE,CIwASDUIEdAttrPanel::CTree::OnMenu)
    EVT_KEY_DOWN(CIwASDUIEdAttrPanel::CTree::OnKey)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDUIEdAttrStylePanel, CIwASDUIEdAttrPanel)
    EVT_COMBOBOX(CTRLID_STYLE,CIwASDUIEdAttrStylePanel::OnStyle)
    //EVT_BUTTON(CTRLID_SETDEFAULT,CIwASDUIEdAttrStylePanel::OnToDefault)
    EVT_BUTTON(CTRLID_ADD,CIwASDUIEdAttrStylePanel::OnAdd)
    EVT_BUTTON(CTRLID_CHECKPROPUSAGE,CIwASDUIEdAttrStylePanel::OnCheckPropUsage)
    EVT_BUTTON(CTRLID_ADDSTYLESHEET,CIwASDUIEdAttrStylePanel::OnAddStyleSheet)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDUIEdAttrMaterialPanel, CIwASDUIEdAttrPanel)
    EVT_BUTTON(CTRLID_ADD,CIwASDUIEdAttrMaterialPanel::OnAdd)
    EVT_BUTTON(CTRLID_CHECKPROPUSAGE,CIwASDUIEdAttrMaterialPanel::OnCheckPropUsage)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
wxDragResult CIwUIEdAttrDropTarget::OnData(wxCoord x,wxCoord y,wxDragResult def)
{
    if (!GetData())
        return wxDragNone;

    wxTextDataObject *dobj=(wxTextDataObject*)m_dataObject;
    return OnDropText(x,y,dobj->GetText(),def);
}

//------------------------------------------------------------------------------
wxDragResult CIwUIEdAttrDropTarget::OnDropText(wxCoord x, wxCoord y, wxString data,wxDragResult def)
{
    bool insert=false,move=(def==wxDragMove);
    CIwAttrInstance* inst=m_Frame->m_Shared->m_DragInst;
    if (data.StartsWith(L"!"))
    {
        data=data.Mid(1);
        move=false;
    }

    if (lastId.IsOk())
        m_Frame->m_Tree->SetItemDropHighlight(lastId,false);

    lastId.Unset();

    if (move && !data.StartsWith(m_Frame->m_Type))
        return wxDragNone;

    CIwAttrInstance* parent=m_Frame->m_Base;

    int flags;
    wxTreeItemId id=m_Frame->m_Tree->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
    {
        CIwAttrTreeItem *data=(CIwAttrTreeItem*)m_Frame->m_Tree->GetItemData(id);
        if (data==NULL || data->m_Inst==NULL)
        {
            if (m_Frame->m_Tree->GetItemParent(id)!=m_Frame->m_Tree->GetRootItem())
            {
                id=m_Frame->m_Tree->GetItemParent(id);
                data=(CIwAttrTreeItem*)m_Frame->m_Tree->GetItemData(id);
            }
        }

        if (data!=NULL && data->m_Inst!=NULL)
        {
            parent=data->m_Inst;

            wxRect rect;
            m_Frame->m_Tree->GetBoundingRect(id,rect);

            if (y<=rect.y+3)
                insert=true;

            if (y>=rect.y+rect.height-3)
            {
                CIwAttrInstance* item=GetFirstChild(parent);
                if (item==NULL)
                    item=GetNext(parent);

                if (item!=NULL)
                {
                    parent=item;
                    insert=true;
                }
            }
        }
    }

    if (inst==NULL)
    {
        m_Frame->Drop(data.BeforeFirst(' '),data.AfterFirst(' '),parent);
        return def;
    }

    if (move)
    {
        for (CIwAttrInstance* item=parent; item!=NULL && item->m_Parent!=NULL; item=item->m_Parent->m_Instance)
        {
            if (item==inst)
                return wxDragNone;
        }
    }

    m_Frame->m_Shared->m_Selecting=true;
    m_Frame->m_Tree->SelectItem(wxTreeItemId());
    m_Frame->m_Shared->m_Selecting=false;

    m_Frame->Drop(inst,parent,insert,move);
    return def;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CIwUIEdAttrDropTarget::GetFirstChild(CIwAttrInstance* inst)
{
    for (int i=0; i<(int)inst->m_Data.size(); i++)
    {
        if (inst->m_Data[i]->m_Member->m_Name==L"CIwUIElement")
            return inst->m_Data[i]->m_Items[0].m_Inst;

        if (inst->m_Data[i]->m_Member->m_Name==L"CIwUIFocusHandler")
            return inst->m_Data[i]->m_Items[0].m_Inst;

        if (inst->m_Data[i]->m_Member->m_Name==L"CIwUILayout")
            return inst->m_Data[i]->m_Items[0].m_Inst;

        if (inst->m_Data[i]->m_Member->m_Name==L"CIwUILayoutItem")
            return inst->m_Data[i]->m_Items[0].m_Inst;

        if (inst->m_Data[i]->m_Member->m_Name==L"element")
            return inst->m_Data[i]->m_Items[0].m_Inst;
    }

    return NULL;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CIwUIEdAttrDropTarget::GetNext(CIwAttrInstance* inst)
{
    if (inst->m_Parent==NULL)
        return NULL;

    bool found=false;
    for (int i=0; i<(int)inst->m_Parent->m_Instance->m_Data.size(); i++)
    {
        if (!found && inst->m_Parent->m_Instance->m_Data[i]==inst->m_Parent)
            found=true;
        else if (found && inst->m_Parent->m_Instance->m_Data[i]->m_Member==inst->m_Parent->m_Member)
            return inst->m_Parent->m_Instance->m_Data[i]->m_Items[0].m_Inst;
    }
    return GetNext(inst->m_Parent->m_Instance);
}

//-----------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnDrag(wxTreeEvent& e)
{
    wxTreeItemId id=e.GetItem();
    if (!id.IsOk()) return;

    CIwAttrTreeItem* data=(CIwAttrTreeItem*)m_Tree->GetItemData(id);
    if (data==NULL || data->m_Inst==NULL) return;

    std::vector<CIwAttrInstance*> add;
    std::vector<CIwAttrInstance*> remove;
    UpdateSelList(add,remove);

    m_Shared->m_DragInst=data->m_Inst;
    m_Shared->m_DragMode=m_Type;

    wxString name=m_Type+L" "+m_Shared->GetFullName(data->m_Inst);

    strcpy(CIwUIEdDropTarget::s_CurrData,name.mb_str());
    wxTextDataObject text(name);
    wxDropSource source(text,this);
    source.DoDragDrop(wxDrag_DefaultMove);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::Drop(const wxString& type,const wxString& data,CIwAttrInstance* parent)
{
    if (parent==NULL || parent->FindData(type,0)==NULL) return;

    CIwAttrNote Note;
    std::vector<wxString> argv2;

    Note.m_Name=type;
    Note.m_Data=data;
    argv2.push_back(Note.m_Data);

    parent->AddFromNote(Note.m_Name,Note,argv2,m_Sect);
    Select(m_Tree->GetRootItem(),parent);
    ResetProp();

    m_Shared->SetSelection(m_Shared->m_SelElem);
}
//------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::Drop(CIwAttrInstance* inst,CIwAttrInstance* parent,bool insert,bool move)
{
    if (move)
    {
        if (insert)
            m_Shared->Change(inst,CHANGETYPE_MOVE|SELSOURCE_INSERT|SELSOURCE_SCHEDULE,parent);
        else
            m_Shared->Change(inst,CHANGETYPE_MOVE|SELSOURCE_SCHEDULE,parent);
    }
    else
    {
        if (insert)
            m_Shared->Change(inst,CHANGETYPE_ADD|SELSOURCE_INSERT|SELSOURCE_SCHEDULE,parent);
        else
            m_Shared->Change(inst,CHANGETYPE_ADD|SELSOURCE_SCHEDULE,parent);
    }
}

//------------------------------------------------------------------------------
void CIwUIEdAttrDropTarget::OnLeave()
{
    if (lastId.IsOk())
        m_Frame->m_Tree->SetItemDropHighlight(lastId,false);

    lastId.Unset();
}

//------------------------------------------------------------------------------
wxDragResult CIwUIEdAttrDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    if (m_Frame->m_Shared->m_CurrUI==NULL)
        return wxDragNone;

    if (lastId.IsOk())
        m_Frame->m_Tree->SetItemDropHighlight(lastId,false);

    int flags;
    wxTreeItemId id=m_Frame->m_Tree->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
    {
        wxRect rect;
        m_Frame->m_Tree->GetBoundingRect(id,rect);
        if ((y<=rect.y+3 || y>=rect.y+rect.height-3))
            id.Unset();
        else
            m_Frame->m_Tree->SetItemDropHighlight(id,true);
    }

    lastId=id;

    if (m_Frame->m_Shared->m_DragMode.StartsWith(L"!"))
        return wxDragCopy;

    return def;
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnKey(wxKeyEvent& e)
{
    if (e.GetKeyCode()!=WXK_DELETE)
        return;

    if (m_Sect!=NULL)
        Remove(m_Sect);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnReset(wxCommandEvent&)
{
    m_Shared->ResetUI();
}
//------------------------------------------------------------------------------
class CIwASDUIAttrCheckNameCtrl : public wxTextCtrl
{
    CUIEdAttrLinkData* m_Data;
    enum { CTRLID_TEXT };
public:
    CIwASDUIAttrCheckNameCtrl(wxWindow* parent,CUIEdAttrLinkData* data) : wxTextCtrl(parent,CTRLID_TEXT,data->m_Name,wxPoint(-1,-1),wxSize(100,-1)),m_Data(data) {}
    void OnText(wxCommandEvent&)
    {
        m_Data->m_Name=GetValue();
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CIwASDUIAttrCheckNameCtrl, wxTextCtrl)
    EVT_TEXT(CTRLID_TEXT,CIwASDUIAttrCheckNameCtrl::OnText)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
class CIwASDUIAttrCheckNames : public CIwStyleDialog
{
    CUIEdProjectUI* m_File;
public:
    CIwASDUIAttrCheckNames(wxWindow* parent,CUIEdProjectUI* file) : CIwStyleDialog(parent,L"Please Change Names that are duplicates in the current UI file"),m_File(file)
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        CIwStyleCtrlGroup* group=new CIwStyleCtrlGroup(true,true);
        sizer->Add(group);

        std::vector<int> done;

        //TEMP
        /*
           for(int i=0;i<(int)m_File->m_Elements.size();i++) {
            wxSizer* sizer2=NULL;
            int j;

            for(j=0;j<(int)done.size();j++)
                if(done[j]==i)
                    break;
            if(j<(int)done.size())
                continue;

            for(j=i+1;j<(int)m_File->m_Elements.size();j++)
                if(m_File->m_Elements[i]->m_Name.IsSameAs(m_File->m_Elements[j]->m_Name,false)) {
                    if(sizer2==NULL) {
                        sizer2=new wxFlexGridSizer(2);
                        group->Add(sizer2,m_File->m_Elements[i]->m_Name+L":",this);

                        sizer2->Add(new wxStaticText(this,wxID_ANY,CIwTheHost.m_Shared.GetFullName(m_File->m_Elements[i]->m_Element)+L" - "+
                            m_File->m_Elements[i]->m_Element->m_Class->m_Name,wxPoint(-1,-1),wxSize(300,-1)),0,wxALIGN_CENTER);
                        sizer2->Add(new CIwASDUIAttrCheckNameCtrl(this,m_File->m_Elements[i]));
                    }
                    done.push_back(j);
                    sizer2->Add(new wxStaticText(this,wxID_ANY,CIwTheHost.m_Shared.GetFullName(m_File->m_Elements[j]->m_Element)+L" - "+
                        m_File->m_Elements[i]->m_Element->m_Class->m_Name,wxPoint(-1,-1),wxSize(300,-1)),0,wxALIGN_CENTER);
                    sizer2->Add(new CIwASDUIAttrCheckNameCtrl(this,m_File->m_Elements[j]));
                }
           }*/

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"OK"),CIwStyleButtonBar::SPACE_PROP);
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"));
        sizer->Add(bar,0,wxEXPAND);

        sizer->Fit(this);
    }
};


//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnCheckNames(wxCommandEvent&)
{
    //int i,j;
    //bool found=false;
    //CIwAttrData* data;
    //TEMP
    /*

       if(m_Shared->m_CurrUI==NULL)
        return;

       m_Shared->m_CurrUI->SetupElements();

       for(i=0;i<(int)m_Shared->m_CurrUI->m_Elements.size() && !found;i++) {
        for(j=i+1;j<(int)m_Shared->m_CurrUI->m_Elements.size() && !found;j++)
            if(m_Shared->m_CurrUI->m_Elements[i]->m_Name.IsSameAs(m_Shared->m_CurrUI->m_Elements[j]->m_Name,false))
                found=true;
       }
       if(!found) return;

       CIwASDUIAttrCheckNames dlg(CIwTheFrame,m_Shared->m_CurrUI);
       if(dlg.ShowModal()==wxID_OK)
       {
        for(int i=0;i<(int)m_Shared->m_CurrUI->m_Elements.size();i++) {
            if(m_Shared->m_CurrUI->m_Elements[i]->m_Element!=NULL) {
                data=m_Shared->m_CurrUI->m_Elements[i]->m_Element->FindData(L"name",0);
                if(data!=NULL)
                    data->m_Items[0].m_String=m_Shared->m_CurrUI->m_Elements[i]->m_Name;
            }
            if(m_Shared->m_CurrUI->m_Elements[i]->m_LayoutItem!=NULL) {
                data=m_Shared->m_CurrUI->m_Elements[i]->m_LayoutItem->FindData(L"name",0);
                if(data!=NULL)
                    data->m_Items[0].m_String=m_Shared->m_CurrUI->m_Elements[i]->m_Name;
            }
        }

        if(m_Shared->m_UIEdAttrUIPanel!=NULL)
            m_Shared->m_UIEdAttrUIPanel->Reset();
       } else {
        for(int i=0;i<(int)m_Shared->m_CurrUI->m_Elements.size();i++) {
            if(m_Shared->m_CurrUI->m_Elements[i]->m_Element!=NULL) {
                data=m_Shared->m_CurrUI->m_Elements[i]->m_Element->FindData(L"name",0);
                if(data!=NULL)
                    m_Shared->m_CurrUI->m_Elements[i]->m_Name=data->m_Items[0].m_String;
            }
        }
       }
     */
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnMenu(wxTreeEvent& e)
{
    if (!m_Shared->m_Selecting)
    {
        wxTreeItemId Id=e.GetItem();
        if (Id)
        {
            if (!m_Tree->GetItemText(Id).IsSameAs(L"<back>",false))
            {
                CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(Id);

                if (Data!=NULL)
                {
                    m_Sect=Data->m_Inst;
                    ResetProp();
                }
            }
        }
    }

    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    m_PopupMenu=CIwTheFrame->MakeMenu(CIwTheFrame->m_MainMenu,L"&Edit",this);

    PopupMenu(m_PopupMenu);
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnLiveUpdate(wxCommandEvent&)
{
    CIwAttrDescMgr::s_LiveEditing=m_CheckBox->GetValue();
    OnChanged(true);
    //Reset();
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnRemove(wxCommandEvent&)
{
    if (m_Sect!=NULL)
        Remove(m_Sect);
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnRemove2(wxCommandEvent&)
{
    if (currData!=NULL)
        Remove(currData);

    currData=NULL;
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::Selected()
{
    m_Shared->m_OpenEditor=0;
    m_Shared->m_Project.SetOutlineLayouts(false);
    m_Shared->m_Project.SetOutlineElements(true);
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::Remove(CIwAttrInstance* inst)
{
    m_Shared->Change(inst,CHANGETYPE_REMOVE);
}
//--------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::Remove(CIwAttrInstance* inst)
{
    CIwAttrData* data=inst->FindData(L"name",0);
    if (data==NULL) return;

    if (m_Shared->m_PropSetDict.find(data->m_Items[0].m_String)==m_Shared->m_PropSetDict.end())
        return;

    m_Shared->m_PropSetDict[data->m_Items[0].m_String].m_UI->SetChanged(true);
    m_Shared->m_PropSetDict.erase(data->m_Items[0].m_String);
    m_Shared->Remove(inst);
    delete inst;
    m_Sect=NULL;
    m_Shared->Signal(CHANGETYPE_STYLE|SELSOURCE_SCHEDULE);
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrMaterialPanel::Remove(CIwAttrInstance* inst)
{
    CIwAttrData* data=inst->FindData(L"name",0);
    if (data==NULL) return;

    if (m_Shared->m_MaterialDict.find(data->m_Items[0].m_String)==m_Shared->m_MaterialDict.end())
        return;

    m_Shared->m_MaterialDict[data->m_Items[0].m_String].m_UI->SetChanged(true);
    m_Shared->m_MaterialDict.erase(data->m_Items[0].m_String);
    m_Shared->Remove(inst);
    delete inst;
    m_Sect=NULL;
    m_Shared->Signal(CHANGETYPE_MAT|SELSOURCE_SCHEDULE);
}

//------------------------------------------------------------------------------
wxString CIwASDUIEdAttrPanel::FilterAttrText(const wxString& curr,const wxString& old)
{
    int i,j;
    wxString out;
    std::vector<wxString> lines;
    std::vector<wxString> oldLines;

    Split(curr,lines,L"\n\r");
    Split(old,oldLines,L"\n\r");

    for (i=0; i<(int)lines.size(); i++)
    {
        std::vector<wxString> args;
        if (SuperSplit(lines[i],args,L" \t")<1) continue;

        if (args[0].IsSameAs(L"class",false) || args[0].IsSameAs(L"name",false))
        {
            out+=lines[i]+L"\n";
            continue;
        }

        for (j=0; j<(int)oldLines.size(); j++)
        {
            std::vector<wxString> oldArgs;
            if (SuperSplit(oldLines[j],oldArgs,L" \t")<1) continue;

            if (args[0]==oldArgs[0])
            {
                if (args.size()!=oldArgs.size())
                    out+=lines[i]+L"\n";
                else
                {
                    for (int k=1; k<(int)args.size(); k++)
                    {
                        if (args[k]!=oldArgs[k])
                        {
                            out+=lines[i]+L"\n";
                            break;
                        }
                    }
                }

                break;
            }
        }
        if (j==(int)oldLines.size())
            out+=lines[i]+L"\n";
    }
    return out;
}

//------------------------------------------------------------------------------
wxString CIwASDUIEdAttrPanel::GetAttrText()
{
    if (m_Sect==NULL)
        return L"";

    if (m_OldSect!=m_Sect)
    {
        m_OldSect=m_Sect;
        return L"";
    }

    wxString outstr=wxString::Format(L"class %s\n",m_Sect->m_Class->m_Name.c_str());
    for (CIwAttrClass* c=m_Sect->m_Class; c!=NULL; c=c->m_Parent)
    {
        if (c->m_Flags&ATTRCLASS_VIEWERGROUP_F)
        {
            outstr=wxString::Format(L"class %s\n",c->m_Name.c_str());
            break;
        }
    }

    return outstr+m_Sect->WriteNotes(0,NULL,true,true,false,true);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnChanged(bool Changed)
{
    CUIEdProjectUI* ui=m_Shared->m_CurrUI;

    if (m_Sect!=NULL && m_Sect->m_File!=NULL)
    {
        CUIEdProjectUI::Group* group=(CUIEdProjectUI::Group*)m_Sect->m_File;
        while (group->m_Parent!=NULL)
            group=(CUIEdProjectUI::Group*)group->m_Parent;
        ui=group->m_UI;
    }

    if (ui==NULL || ui->m_Group.m_Inst==NULL)
        return;

    ui->SetChanged(Changed);

    if (CIwTheHost.m_Link==NULL) return;

    if (m_CheckBox!=NULL && !m_CheckBox->GetValue()) return;

    CIwAttrInstance* old=m_Sect;
    if (m_Selector!=NULL && m_Selector->GetSelection()==2)
        m_Shared->SendChanges(m_Sect);
    else if ((m_Sect->m_Parent->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
        m_Shared->SendChanges(m_Sect);
    else
    {
        wxString outstr=GetAttrText();
        wxString filtered=FilterAttrText(outstr,m_OldAttrText);
        m_OldAttrText=outstr;

        char ch[1024];
        strncpy(ch,filtered.mb_str(),1023);
        CIwTheHost.m_Link->SetData(CIwViewerUI::PARSE_UPDATE,ch);
    }
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::CShim::DealWithChange(CIwAttrData* data)
{
    if (data->m_Member->m_Name.IsSameAs(L"name",false))
        m_Frame->NameChanged(data,true);

    if (data->m_Member->m_Name.IsSameAs(L"parent",false))
    {
        CIwAttrInstance* inst=data->m_Instance;
        CIwAttrData* data2=inst->FindData(L"extraclass");

        if (data->m_Items.empty()) return;

        if (data2!=NULL)
        {
            int i;
            std::vector<CIwAttrNote> notes;
            inst->FillNotes(notes,false,0,true);

            for (i=0; i<(int)notes.size(); i++)
            {
                if (!notes[i].m_Name.IsSameAs(L"classes") && !notes[i].m_Name.IsSameAs(L"extraclass") &&
                    !notes[i].m_Name.IsSameAs(L"parent") && !notes[i].m_Name.IsSameAs(L"name"))
                    break;

                notes.erase(notes.begin()+i);
                i--;
            }
            if (i<(int)notes.size())
                return;

            for (i=0; i<(int)inst->m_Data.size(); )
            {
                if (!inst->m_Data[i]->m_Member->m_Name.IsSameAs(L"name",false) && !inst->m_Data[i]->m_Member->m_Name.IsSameAs(L"parent",false))
                {
                    delete inst->m_Data[i];
                    inst->m_Data.erase(inst->m_Data.begin()+i);
                }
                else
                    i++;
            }
            inst->m_ExtraData.clear();
            inst->GetFromNotes(notes);
        }

        // Possible for the parent to not be set
        if (data->m_Items[0].m_Inst)
        {
            data2=data->m_Items[0].m_Inst->FindData(L"extraclass");

            std::vector<wxString> args;
            CIwAttrNote note;
            note.m_Name=L"extraclass";
            if (data2==NULL)
                note.m_Data=data->m_Items[0].m_Inst->m_Class->m_Name;
            else
                note.m_Data=data2->m_Items[0].m_Class->m_Name;

            note.m_Info=note.m_ID=-1;
            args.push_back(note.m_Data);
            inst->AddFromNote(note.m_Name,note,args,inst);
        }
    }

    CUIEdAttrLinkData* parent=(CUIEdAttrLinkData*)data->m_Instance->m_TempClass;
    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR && parent!=NULL)
    {
        int i,j,k=0;

        for (i=0; i<(int)parent->m_Children.size(); )
        {
            if (!parent->m_Children[i]->m_Name.StartsWith(data->m_Member->m_Name))
            {
                i++;
                continue;
            }

            for (j=0; j<(int)data->m_Items.size(); j++)
            {
                if (parent->m_Children[i]->m_Element==data->m_Items[j].m_Inst)
                    break;
            }
            if (j==(int)data->m_Items.size())
            {
                delete parent->m_Children[i];
                parent->m_Children.erase(parent->m_Children.begin()+i);
                continue;
            }

            if (parent->m_Children[i]->m_Type==LAYOUTITEM_PROPERTYSET)
            {
                parent->m_Children[i]->m_Name=data->m_Member->m_Name;
                if (data->m_Member->m_Type&ATTRMEMBER_LIST)
                    parent->m_Children[i]->m_Name+=wxString::Format(L"[%d]",k++);

                CIwAttrData* name=data->m_Items[j].m_Inst->FindData(L"name",0);
                if (name!=NULL)
                    name->m_Items[0].m_String=parent->m_Children[i]->m_Name;
            }

            i++;
        }
    }
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::NameChanged(CIwAttrData* data,bool forward)
{
    wxString name=m_Tree->GetItemText(data->m_Instance->m_TreeId).BeforeFirst(' ');
    m_Tree->SetItemText(data->m_Instance->m_TreeId,data->m_Instance->GetTreeName(true));
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::NameChanged(CIwAttrData* data,bool forward)
{
    if (data->m_Instance->m_TreeId.IsOk())
        m_Tree->SetItemText(data->m_Instance->m_TreeId,data->m_Instance->GetTreeName(true));

    CUIEdAttrLinkData* link=(CUIEdAttrLinkData*)data->m_Instance->m_TempClass;

    if (forward && link!=NULL)
    {
        link->m_Name=data->m_Items[0].m_String;

        if (link->m_LayoutItem!=NULL)
        {
            CIwAttrData* data2=link->m_LayoutItem->FindData(L"name",0);
            data2->m_Items[0].m_String=data->m_Items[0].m_String;
            link->m_LayoutItem->SetChanged(true);
        }
    }
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::CShim::SetChanged(bool Changed)
{
    m_Frame->OnChanged(Changed);
    //bool enabled=m_Frame->m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)!=0;
    //m_Frame->m_PropPanel->Enable(enabled);
}
//-----------------------------------------------------------------------------
wxString CIwASDUIEdAttrPanel::CShim::GetBaseDir(bool fileBase)
{
    return m_Frame->m_Shared->m_Project.m_RootDir;
}

//--------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnSave(wxCommandEvent&)
{
    if (m_Shared->m_CurrUI==NULL) return;

    m_Shared->m_CurrUI->CheckSave(true);

    int num=0;
    m_Shared->m_Project.m_GroupData->SaveFiltered(CIwTheHost.m_AltGroupFile,num);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnRefresh(wxCommandEvent&)
{
    if (m_Sect!=NULL)
        m_Shared->SendChanges(m_Sect);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::Select(wxTreeItemId parent,CIwAttrInstance* inst,bool single)
{
    if (!parent.IsOk()) return;

    if (single)
        m_Tree->SelectItem(parent,false);

    CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(parent);
    if (Data!=NULL)
    {
        if (Data->m_Inst==inst)
        {
            m_Sect=Data->m_Inst;
            m_Tree->SelectItem(parent);
        }

        CUIEdAttrLinkData* link=(CUIEdAttrLinkData*)Data->m_Inst->m_TempClass;
        if (link!=NULL && (link->m_Element==inst || link->m_LayoutItem==inst) && inst!=NULL)
        {
            m_Sect=Data->m_Inst;
            m_Tree->SelectItem(parent);
        }
    }

    wxTreeItemIdValue cookie;
    wxTreeItemId id=m_Tree->GetFirstChild(parent,cookie);

    while (id.IsOk())
    {
        Select(id,inst,single);

        id=m_Tree->GetNextChild(id,cookie);
    }
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnItemSelected(wxTreeEvent& event)
{
    if (m_Shared->m_Selecting) return;

    wxTreeItemId Id=event.GetItem();
    if (!Id) return;

    if (m_Tree->GetItemText(Id).IsSameAs(L"<back>",false))
        return;

    CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(Id);
    if (Data==NULL) return;

    m_Sect=Data->m_Inst;
    ResetProp();
}
//-----------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnMouseUp(wxMouseEvent& e)
{
}

//-----------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::OnMouseUp(wxMouseEvent& e)
{
    if (m_Shared->m_Selecting) return;

    std::vector<CIwAttrInstance*> add;
    std::vector<CIwAttrInstance*> remove;
    UpdateSelList(add,remove);
    if (m_Shared->m_SelList.size()==1)
        m_Shared->SetSelection(m_Shared->m_SelList[0],SELSOURCE_UIED);
    else if (m_Shared->m_SelList.size()==0)
        m_Shared->SetSelection(NULL,SELSOURCE_UIED);
    else
    {
        m_Shared->SendSelection(add,remove);
        ResetProp();
    }
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::UpdateSelList(std::vector<CIwAttrInstance*>& add,std::vector<CIwAttrInstance*>& remove)
{
    int i,j;
    wxArrayTreeItemIds sel;
    m_Tree->GetSelections(sel);

    for (i=0; i<(int)sel.size(); i++)
    {
        CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(sel[i]);
        if (Data==NULL) continue;

        for (j=0; j<(int)m_Shared->m_SelList.size(); j++)
        {
            if (Data->m_Inst==m_Shared->m_SelList[j])
                break;
        }
        if (j==(int)m_Shared->m_SelList.size())
            add.push_back(Data->m_Inst);
    }
    for (j=0; j<(int)m_Shared->m_SelList.size(); )
    {
        for (i=0; i<(int)sel.size(); i++)
        {
            CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(sel[i]);
            if (Data==NULL) continue;

            if (Data->m_Inst==m_Shared->m_SelList[j])
                break;
        }
        if (i==(int)sel.size())
        {
            remove.push_back(m_Shared->m_SelList[j]);
            m_Shared->m_SelList.erase(m_Shared->m_SelList.begin()+j);
        }
        else
            j++;
    }
    for (j=0; j<(int)add.size(); j++)
    {
        m_Shared->m_SelList.push_back(add[j]);
    }
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::OnItemSelected(wxTreeEvent& event)
{
}

static const wxChar* iconList[]={
    L"button16x16.png;CIwUIButton",
    L"dialogue16x16.png;CIwUIAlertDialog;dialogs",
    L"elements16x16.png;CIwUIElement;controls",
    L"focus16x16.png;CIwUIFocusHandler;changefocus",
    L"gridlayout16x16.png;CIwUILayoutGrid",
    L"horizontallayout16x16.png;CIwUILayoutVertical",
    L"verticallayout16x16.png;CIwUILayoutHorizontal",
    L"image16x16.png;CIwUIImage",
    L"label16x16.png;CIwUILabel",
    L"layoutnone16x16.png;CIwUILayout;changelayout;layouts",
    L"progress16x16.png;CIwUIProgressBar",
    L"scrollview16x16.png;CIwUIScrollableView",
    L"slider16x16.png;CIwUISlider",
    L"focus16x16.png;CIwUILayoutItemContainer",
    L"goto16x16.png;CIwPropertySet;CIwUIPropertySet",
    NULL
};

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::SetupIcons()
{
    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/16x16/");

    wxImageList* list=new wxImageList(16,16);

    for (int i=0; iconList[i]!=NULL; i++)
    {
        wxString file=dir+wxString(iconList[i]).BeforeFirst(';');
        file.Replace(L"\\",L"/");
        list->Add(wxBitmap(file,wxBITMAP_TYPE_PNG));
    }

    m_Tree->AssignImageList(list);
}

//------------------------------------------------------------------------------
int CIwASDUIEdAttrPanel::GetIcon(CIwAttrInstance* inst)
{
    int j,k;
    if (inst==NULL) return -1;

    CIwAttrData* data=inst->FindData(L"name",0);
    if (data!=NULL && data->m_Items[0].m_String.Find('/')!=-1)
    {
        for (k=0; iconList[k]!=NULL; k++)
        {
            std::vector<wxString> args;
            Split(wxString(iconList[k]),args,L";");

            if (args.size()==1)
                return k;

            for (j=1; j<(int)args.size(); j++)
            {
                if (data->m_Items[0].m_String.Lower().EndsWith(args[j].Lower()))
                    break;
            }
            if (j<(int)args.size())
                return k;
        }
    }
    else if (inst->m_Parent!=NULL)
    {
        data=inst->m_Parent->m_Instance->FindData(L"name",0);

        if (data!=NULL && data->m_Items[0].m_String.Find('/')!=-1)
        {
            for (int i=0; i<(int)inst->m_Data.size(); i++)
            {
                if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
                {
                    j=GetIcon(inst->m_Data[i]->m_Items[0].m_Inst);
                    if (j!=-1)
                        return j;
                }
            }
        }
    }

    for (CIwAttrClass* klass=inst->m_Class; klass!=NULL; klass=klass->m_Parent)
    {
        for (k=0; iconList[k]!=NULL; k++)
        {
            std::vector<wxString> args;
            Split(wxString(iconList[k]),args,L";");

            if (args.size()==1)
                return k;

            for (j=1; j<(int)args.size(); j++)
            {
                if (klass->m_Name.IsSameAs(args[j],false))
                    break;
            }
            if (j<(int)args.size())
                return k;
        }
    }
    return -1;
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::OnSelector(wxCommandEvent&)
{
    ResetProp();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrUnityPanel::BuildTree(CUIEdAttrLinkData* link,wxTreeItemId id)
{
    int icon=GetIcon(link->m_Element);
    link->m_Element->m_TreeId=m_Tree->AppendItem(id,link->m_Name,icon,icon,new CIwAttrTreeItem(link->m_Element));

    for (int i=0; i<(int)link->m_Children.size(); i++)
    {
        BuildTree(link->m_Children[i],link->m_Element->m_TreeId);
    }
}

//------------------------------------------------------------------------------
CIwAttrInstance* CIwASDUIEdAttrUnityPanel::BuildTree()
{
    if (m_Shared->m_CurrUI==NULL || m_Shared->m_CurrUI->m_Group.m_Inst==NULL)
    {
        m_Tree->AppendItem(m_Tree->GetRootItem(),L"-= Open a UI file first =-");
        return NULL;
    }

    for (int i=0; i<(int)m_Shared->m_CurrUI->m_BaseElement.m_Children.size(); i++)
    {
        BuildTree(m_Shared->m_CurrUI->m_BaseElement.m_Children[i],m_Tree->GetRootItem());
    }

    if (m_Tree->GetChildrenCount(m_Tree->GetRootItem())==0)
        m_Tree->AppendItem(m_Tree->GetRootItem(),L"-= Drop CIwUIElement(s) Here =-");

    return m_Shared->m_CurrUI->m_Group.m_Inst;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CIwASDUIEdAttrStylePanel::BuildTree()
{
    wxArrayString strings;

    m_Style->Clear();
    m_Style->AppendString(L"");
    for (int i=0; i<(int)m_Shared->m_StyleList.size(); i++)
    {
        m_Style->AppendString(m_Shared->m_StyleList[i].m_StyleSheet);
    }
    m_Style->SetStringSelection(m_Shared->m_Project.m_StyleSheet);

    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_Shared->m_PropSetDict.begin(); it!=m_Shared->m_PropSetDict.end(); ++it)
    {
        CIwAttrInstance* inst2=it->second.m_Inst;

        inst2->AddToTree(m_Tree,m_Tree->GetRootItem());
    }
    return NULL;
}
//------------------------------------------------------------------------------
CIwAttrInstance* CIwASDUIEdAttrMaterialPanel::BuildTree()
{
    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_Shared->m_MaterialDict.begin(); it!=m_Shared->m_MaterialDict.end(); ++it)
    {
        CIwAttrInstance* inst2=it->second.m_Inst;

        inst2->AddToTree(m_Tree,m_Tree->GetRootItem());
    }
    return NULL;
}

//------------------------------------------------------------------------------
wxString CIwASDUIEdAttrPanel::GetName(wxTreeItemId id)
{
    if (!id || id==m_Tree->GetRootItem()) return L"";

    wxString me=m_Tree->GetItemText(id);
    wxString parent=GetName(m_Tree->GetItemParent(id));
    if (parent.empty())
        return me;

    return parent+L"|"+me;
}

//------------------------------------------------------------------------------
wxTreeItemId CIwASDUIEdAttrPanel::FindName(std::vector<wxString>& args,int offset,wxTreeItemId parent)
{
    if (!parent) return wxTreeItemId();

    if (offset==(int)args.size())
        return wxTreeItemId();

    wxTreeItemIdValue cookie;
    wxTreeItemId id=m_Tree->GetFirstChild(parent,cookie);
    while (id.IsOk()) {
        if (args[offset]==m_Tree->GetItemText(id))
        {
            if (offset+1==(int)args.size())
                return id;
            else
            {
                wxTreeItemId id2=FindName(args,offset+1,id);
                if (id2.IsOk())
                    return id2;
            }
        }

        id=m_Tree->GetNextChild(parent,cookie);
    }
    return wxTreeItemId();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::Reset()
{
    Freeze();

    m_Tree->DeleteAllItems();
    wxTreeItemId Id=m_Tree->AddRoot(L"");

    m_Sect=m_Base=BuildTree();

    SetOpenedAll(m_Tree->GetRootItem());

    Thaw();

    m_Sizer->Layout();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::SetOpenedAll(wxTreeItemId parent)
{
    if (!parent) return;

    wxTreeItemIdValue cookie;
    wxTreeItemId id=m_Tree->GetFirstChild(parent,cookie);
    while (id.IsOk()) {
        m_Tree->Expand(id);
        SetOpenedAll(id);

        id=m_Tree->GetNextChild(parent,cookie);
    }
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::SetCurrent(CIwAttrInstance* inst)
{
    m_Sect=inst;
    ResetProp();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::ResetProp()
{
    Freeze();

    m_OldAttrText=GetAttrText();

    m_PropPanel->Clean();

    wxArrayTreeItemIds sel;
    m_Tree->GetSelections(sel);

    if (m_Sect==NULL || sel.size()!=1)
    {
        Thaw();
        return;
    }

    int mode=1;

    if (m_Selector!=NULL)
        mode=m_Selector->GetSelection();

    CUIEdAttrLinkData* link=(CUIEdAttrLinkData*)m_Sect->m_TempClass;
    switch (mode)
    {
    case 0:     //basic
        m_Sect->SetupDlg(&m_Shim);
        m_PropPanel->Fill(m_Sect,NULL,true);
        break;
    case 1:     //advanced
        m_Sect->SetupDlg(&m_Shim);
        m_PropPanel->Fill(m_Sect,NULL,false);
        break;
    case 2:     //container
        if (link!=NULL && link->m_Element==m_Sect && link->m_LayoutItem!=NULL)
        {
            link->m_LayoutItem->SetupDlg(&m_Shim);
            m_PropPanel->Fill(link->m_LayoutItem,NULL,false);
        }
        else if (m_Sect->m_Parent!=NULL && m_Sect->m_Parent->m_Instance->m_Class->m_Name.IsSameAs(L"CIwUILayoutItemContainer",false))
        {
            m_Sect->m_Parent->m_Instance->SetupDlg(&m_Shim);
            m_PropPanel->Fill(m_Sect->m_Parent->m_Instance,NULL,false);
        }

        break;
    }

    //if(m_Sect!=m_Base)
    //	m_PropSizer->Add(new CIwStyleButton(m_PropPanel,CTRLID_REMOVE,L"Remove This"),L"");

    //bool enabled=m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)!=0;
    //m_PropPanel->Enable(enabled);

    Thaw();
}

//------------------------------------------------------------------------------
bool CIwASDUIEdAttrPanel::CShim::GridOverrideActivate(wxGrid* grid,CIwAttrData* data,std::vector<CIwAttrData*>& alts,int row,int col)
{
    if (col==4 && grid->GetCellValue(row,col).IsSameAs(L"Change",false))
    {
        CUIEdProjectUI::Group* group=(CUIEdProjectUI::Group*)data->m_Instance->m_File;
        if (group==NULL) return true;

        wxFileName name(group->m_FileName);
        wxFileName name1(m_Frame->m_Shared->m_Project.m_RootDir);
        wxString path;
        if (data->m_Items[0].m_String[0]=='.')
            path=name.GetPath()+data->m_Items[0].m_String.Mid(1);
        else if (data->m_Items[0].m_String[0]=='\\' || data->m_Items[0].m_String[0]=='/')
            path=m_Frame->m_Shared->m_Project.m_RootDir+data->m_Items[0].m_String;
        else
            path=name.GetPath()+data->m_Items[0].m_String;

        wxFileName name2(path);

        while (true) {
            wxFileDialog dlg(CIwTheFrame,L"Please select texture file",name2.GetPath(),name2.GetFullName(),
                             L"Texture files (*.tga;*.bmp;*.png;*,gif)|*.tga;*.bmp;*.png;*,gif|All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);

            if (dlg.ShowModal()==wxID_OK)
            {
                wxString rest1;
                if (dlg.GetPath().Lower().StartsWith(name1.GetFullPath().Lower(),&rest1))
                {
                    wxString rest;
                    if (dlg.GetPath().Lower().StartsWith(name.GetPath().Lower(),&rest))
                        data->m_Items[0].m_String=L"."+rest;
                    else
                        data->m_Items[0].m_String=L"/"+rest1;

                    data->SetChanged();
                    grid->SetCellValue(data->m_Items[0].m_String,row,1);
                    break;
                }
            }
            else
                break;
        }
        return true;
    }

    if (col==4 && grid->GetCellValue(row,col).IsSameAs(L"Go To",false))
    {
        if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
        {
            if (data->m_Member->m_Items[0].m_Class->m_Name==L"CIwPropertySet")
            {
                if (m_Frame->m_Shared->m_UIEdAttrStylePanel!=NULL && !data->m_Items.empty() && data->m_Items[0].m_Inst!=NULL)
                {
                    m_Frame->m_Shared->m_UIEdAttrStylePanel->Select(
                        m_Frame->m_Shared->m_UIEdAttrStylePanel->m_Tree->GetRootItem(),data->m_Items[0].m_Inst);
                    m_Frame->m_Shared->m_UIEdAttrStylePanel->SetCurrent(data->m_Items[0].m_Inst);

                    CIwTheFrame->ShowControl(m_Frame->m_Shared->m_UIEdAttrStylePanel);
                }
            }

            if (data->m_Member->m_Items[0].m_Class->m_Name==L"CIwMaterial")
            {
                if (m_Frame->m_Shared->m_UIEdAttrMaterialPanel!=NULL && !data->m_Items.empty() && data->m_Items[0].m_Inst!=NULL)
                {
                    m_Frame->m_Shared->m_UIEdAttrMaterialPanel->Select(
                        m_Frame->m_Shared->m_UIEdAttrMaterialPanel->m_Tree->GetRootItem(),data->m_Items[0].m_Inst);
                    m_Frame->m_Shared->m_UIEdAttrMaterialPanel->SetCurrent(data->m_Items[0].m_Inst);

                    CIwTheFrame->ShowControl(m_Frame->m_Shared->m_UIEdAttrMaterialPanel);
                }
            }
        }

        return true;
    }

    CIwAttrData* data2=OverrideDataForGrid(data,false);
    if (data2==NULL) return false;

    if (col==0) return false;

    if (col!=4)
        return data->m_FromDefault;

    if (grid->GetCellValue(row,col).IsSameAs(L"Override",false))
    {
        wxString line;
        std::vector<wxString> args;

        data2->ToString2(line);
        SuperSplit(line,args,L" \t\n,{}");

        data->SetDefault(true);
        data->Set(args,NULL);
        data->m_FromDefault=false;
        Reset();
    }
    else if (grid->GetCellValue(row,col).IsSameAs(L"Reset",false))
    {
        data->SetDefault(true);
        data->m_FromDefault=true;
        Reset();
    }

    return true;
}

//------------------------------------------------------------------------------
CIwAttrData* CIwASDUIEdAttrPanel::CShim::OverrideDataForGrid(CIwAttrData* data,bool checkFirst)
{
    if (data->m_Member->m_Name==L"name")
        return NULL;

    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        return NULL;

    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_EXTRACLASS)
        return NULL;

    // No need for "Override" button when editing property sets.
    if (data->m_Instance)
    {
        const wxString& className = data->m_Instance->m_Class->m_Name;
        if (className == L"CIwUIPropertySet" || className == L"CIwPropertySet")
            return NULL;
    }

    if (!data->m_FromDefault && checkFirst) return data;

    CIwAttrData* data2=data->m_Instance->FindData(L"style",CIwAttrInstance::FINDMODE_EXPORTERTAG);
    if (data2==NULL || data2->m_Items.empty()) return NULL;

    CIwAttrInstance* set=data2->m_Items[0].m_Inst;
    if (set==NULL) return NULL;

    data2=set->FindData(data->m_Member->m_Name,0);
    if (data2==NULL) return NULL;

    if (!data2->m_FromDefault)
        return data2;

    CIwAttrData* data1=OverrideDataForGrid(data2,true);
    if (data1==NULL) return NULL;

    return data1;
}

//-----------------------------------------------------------------------------
bool CIwASDUIEdAttrPanel::CShim::GridOverrideCheck(CIwAttrData* data)
{
    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS && data->m_Member->m_Name.IsSameAs(L"element",false))
        return true;

    if (data->m_Member->m_ExportSection==L"!layoutsize")
    {
        CIwAttrData* data1=data->m_Instance->FindData(L"sizeToContent",0);
        if (data1!=NULL && data1->m_Items[0].m_Int==1)
            return true;

        if (data->m_Instance->m_Parent!=NULL)
        {
            CIwAttrData* data2=data->m_Instance->m_Parent->m_Instance->FindData(L"CIwUILayout",0);
            if (data2!=NULL)
                return true;
        }
    }

    if (data->m_Member->m_ExportSection==L"!layout")
    {
        if (data->m_Instance->m_Parent!=NULL)
        {
            CIwAttrData* data2=data->m_Instance->m_Parent->m_Instance->FindData(L"CIwUILayout",0);
            if (data2!=NULL)
                return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::CShim::MakeButton(wxGrid* grid,CIwAttrData* data,const wxString& textName,int currRow,int currCol)
{
    wxBitmap* bmp=NULL;
    if (data->m_Instance->m_File!=NULL)
        bmp=data->m_Instance->m_File->GetIcon(textName);

    if (bmp!=NULL)
        grid->SetCellRenderer(currRow,currCol,new wxGridCellButtonIconRenderer(bmp));
    else
        grid->SetCellRenderer(currRow,currCol,new wxGridCellButtonRenderer);

    grid->SetCellValue(textName,currRow,currCol);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrPanel::CShim::GridOverride(wxGrid* grid,int startLine,int endLine,CIwAttrData* data)
{
    if (data->m_Member->m_Name==L"texture0" || data->m_Member->m_Name==L"texture1")
    {
        MakeButton(grid,data,L"Change",startLine,4);
        grid->SetReadOnly(startLine,4);
        return;
    }

    bool found=false;
    if (data->m_Member->m_ExportSection==L"!layoutsize")
    {
        CIwAttrData* data1=data->m_Instance->FindData(L"sizeToContent",0);
        if (data1!=NULL && data1->m_Items[0].m_Int==1)
            found=true;

        if (data->m_Instance->m_Parent!=NULL)
        {
            CIwAttrData* data2=data->m_Instance->m_Parent->m_Instance->FindData(L"CIwUILayout",0);
            if (data2!=NULL)
                found=true;
        }
    }

    if (data->m_Member->m_ExportSection==L"!layout")
    {
        if (data->m_Instance->m_Parent!=NULL)
        {
            CIwAttrData* data2=data->m_Instance->m_Parent->m_Instance->FindData(L"CIwUILayout",0);
            if (data2!=NULL)
                found=true;
        }
    }

    if (found)
    {
        for (int i=startLine; i<=endLine; i++)
        {
            for (int j=0; j<3; j++)
            {
                grid->SetCellTextColour(wxColour(150,150,150),i,j);
                grid->SetReadOnly(i,j);
            }
        }
        return;
    }

    CIwAttrData* data2=OverrideDataForGrid(data,false);
    if (data2!=NULL)
    {
        if (data->m_FromDefault)
        {
            for (int i=startLine; i<=endLine; i++)
            {
                for (int j=0; j<3; j++)
                {
                    grid->SetCellTextColour(wxColour(150,150,150),i,j);
                    grid->SetReadOnly(i,j);
                }
            }
            MakeButton(grid,data,L"Override",startLine,4);
        }
        else
            MakeButton(grid,data,L"Reset",startLine,4);

        return;
    }

    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
    {
        if (data->m_Instance->m_Class)
        {
            const wxString& className = data->m_Instance->m_Class->m_Name;
            if (className == L"CIwUIPropertySet" ||
                className == L"CIwPropertySet" ||
                className == L"CIwMaterial")
                MakeButton(grid,data,L"Go To",startLine,4);
        }
    }
}

//------------------------------------------------------------------------------
CIwASDUIEdAttrUnityPanel::CIwASDUIEdAttrUnityPanel(wxWindow* parent,CUIEdAttrShared* shared) : CIwASDUIEdAttrPanel(parent,shared,true)
{
    m_Shared->m_UIEdAttrUIPanel=this;
    m_Tree->SetDropTarget(new CIwUIEdAttrDropTarget(this));

    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/32x32/");

    m_Selector->Append(L"Layout Container View");
    m_PropPanel->m_HideInline=true;

    m_CheckBox=new wxCustomButton(this,CTRLID_LIVEUPDATE,wxBitmap(dir+L"liveupdate_off32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    m_CheckBox->SetBitmapSelected(wxBitmap(dir+L"liveupdate_on32x32.png",wxBITMAP_TYPE_PNG));
    m_CheckBox->SetValue(true);
    m_CheckBox->SetToolTip(L"Live Update On/Off");
    m_Bar->Add(m_CheckBox);

    wxBitmapButton* button=new wxBitmapButton(this,CTRLID_CHECKNAMES,wxBitmap(dir+L"check_names32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Check Names...");
    m_Bar->Add(button);

    m_Type=L"ui";

    m_Tree->AddRoot(L"");
    m_Tree->AppendItem(m_Tree->GetRootItem(),L"-= Open a UI file first =-");
}

//------------------------------------------------------------------------------
CIwASDUIEdAttrStylePanel::CIwASDUIEdAttrStylePanel(wxWindow* parent,CUIEdAttrShared* shared) : CIwASDUIEdAttrPanel(parent,shared,true)
{
    m_Shared->m_UIEdAttrStylePanel=this;
    wxArrayString strings;

    m_PropPanel->m_HideInline=false;

    strings.Add(L"");
    for (int i=0; i<(int)m_Shared->m_StyleList.size(); i++)
    {
        strings.Add(shared->m_StyleList[i].m_StyleSheet);
    }

    m_Style=new wxComboBox(this,CTRLID_STYLE,m_Shared->m_Project.m_StyleSheet,wxPoint(-1,-1),wxSize(100,-1),strings,wxCB_READONLY);
    m_Bar->AddCentred(m_Style);

    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/32x32/");

    wxBitmapButton* button=new wxBitmapButton(this,CTRLID_ADD,wxBitmap(dir+L"add32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Add Property Set...");
    m_Bar->Add(button);

    button=new wxBitmapButton(this,CTRLID_CHECKPROPUSAGE,wxBitmap(dir+L"check_names32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Check Usage...");
    m_Bar->Add(button);

    button=new wxBitmapButton(this,CTRLID_ADDSTYLESHEET,wxBitmap(dir+L"add_stylesheet32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Add Stylesheet...");
    m_Bar->Add(button);
    m_Type=L"style";
}

//------------------------------------------------------------------------------
class CIwASDUICheckPropUsage : public CIwStyleDialog
{
public:
    CIwASDUICheckPropUsage(wxWindow* parent,std::map<wxString,CUIEdAttrPropSet>& sets,const wxString& title) : CIwStyleDialog(parent,title)
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        wxSizer* sizer2=new wxFlexGridSizer(2);
        sizer->Add(sizer2,1,wxEXPAND);

        sizer2->Add(new wxStaticText(this,wxID_ANY,L"In Use"),0,wxEXPAND|wxALIGN_CENTER);
        sizer2->Add(new wxStaticText(this,wxID_ANY,L"Not In Use"),0,wxEXPAND|wxALIGN_CENTER);

        wxArrayString a,b;
        for (std::map<wxString,CUIEdAttrPropSet>::iterator it=sets.begin(); it!=sets.end(); ++it)
        {
            if (it->second.m_Temp>0)
                a.Add(it->first);
            else
                b.Add(it->first);
        }

        sizer2->Add(new wxListBox(this,wxID_ANY,wxPoint(-1,-1),wxSize(200,200),a),1,wxEXPAND);
        sizer2->Add(new wxListBox(this,wxID_ANY,wxPoint(-1,-1),wxSize(200,200),b),1,wxEXPAND);


        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"Done"),CIwStyleButtonBar::SPACE_PROP);
        sizer->Add(bar,0,wxEXPAND);

        sizer->Fit(this);
    }
};

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnAddStyleSheet(wxCommandEvent&)
{
    wxFileDialog dlg(this,L"Choose name and location for new stylesheet file",m_Shared->m_Project.m_RootDir,L"*.itx",
                     L"stylesheet files (*.itx)|*.itx|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);

    if (dlg.ShowModal()!=wxID_OK)
        return;

    if (!CIwTheHost.StartsWith(dlg.GetPath(),m_Shared->m_Project.m_RootDir))
    {
        wxMessageBox(L"File must be under the Root Directory",L"Add Stylesheet");
        return;
    }

    int i;
    wxString select,parent;
    wxArrayString strings;
    m_Shared->GetFileList(strings,m_Shared->m_Project.m_GroupData,2);
    wxSingleChoiceDialog dlg2(this,L"Which group file should this style sheet be added to?",L"Add stylesheet",strings);

    if (strings.size()>1)
    {
        if (dlg2.ShowModal()!=wxID_OK)
            return;

        select=dlg2.GetStringSelection();
    }
    else
        select=strings[0];

    CUIEdProjectGroup* group=m_Shared->GetFileGroup(select,m_Shared->m_Project.m_GroupData);
    if (group==NULL) return;

    strings.clear();
    strings.Add(L"-= none =-");
    for (i=0; i<(int)m_Shared->m_StyleList.size(); i++)
    {
        strings.Add(m_Shared->m_StyleList[i].m_StyleSheet);
    }
    wxSingleChoiceDialog dlg3(this,L"Which style sheet should this style sheet be parented to?",L"Parent of stylesheet",strings);
    if (strings.size()>1)
    {
        if (dlg3.ShowModal()!=wxID_OK)
            return;

        parent=dlg3.GetStringSelection();
    }
    else
        parent=strings[0];

    wxFileName name(dlg.GetPath());
    wxTextFile fp(dlg.GetPath());
    fp.AddLine(L"// IwUI Stylesheet file");
    fp.AddLine(L"");
    fp.AddLine(L"CIwUIStylesheet");
    fp.AddLine(L"{");
    fp.AddLine(wxString::Format(L"\tname %s",name.GetName().c_str()));
    if (parent!=L"-= none =-")
        fp.AddLine(wxString::Format(L"\tparent %s",parent.c_str()));

    fp.AddLine(L"}");
    fp.Write();
    wxString line=dlg.GetPath().Mid(m_Shared->m_Project.m_RootDir.size());
    line.Replace(L"\\",L"/");
    if (!group->HasFile(line))
    {
        int lineNum=-2;
        for (i=0; i<(int)m_Shared->m_StyleList.size(); i++)
        {
            if (m_Shared->m_StyleList[i].m_StyleSheet==parent)
                break;
        }
        if (i<(int)m_Shared->m_StyleList.size())
        {
            CUIEdProjectGroupLine* line=m_Shared->m_Project.m_GroupData->GetLine(m_Shared->m_StyleList[i].m_UI);
            for (; line!=NULL && line->m_Parent->m_ParentLine!=NULL; line=line->m_Parent->m_ParentLine)
            {
                if (line->m_Parent==group)
                    break;
            }
            if (line!=NULL)
                for (lineNum=0; lineNum<(int)line->m_Parent->m_Lines.size(); lineNum++)
                {
                    if (line==line->m_Parent->m_Lines[lineNum])
                        break;
                }

            if (lineNum==(int)line->m_Parent->m_Lines.size())
                lineNum=-2;
        }

        group->AddLineStart(L"\t\""+line+L"\"",lineNum+1);
    }

    m_Shared->SetStyleSheet(name.GetName());
    m_Shared->m_ProjectFrame->Reset();
    Reset();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnCheckPropUsage(wxCommandEvent&)
{
    std::map<wxString,CUIEdAttrPropSet>::iterator it;

    for (it=m_Shared->m_PropSetDict.begin(); it!=m_Shared->m_PropSetDict.end(); ++it)
    {
        it->second.m_Temp=0;
    }
    for (it=m_Shared->m_MaterialDict.begin(); it!=m_Shared->m_MaterialDict.end(); ++it)
    {
        it->second.m_Temp=0;
    }

    m_Shared->m_Project.m_GroupData->CheckUsage();

    CIwASDUICheckPropUsage dlg(this,m_Shared->m_PropSetDict,L"Property Set Usage");
    dlg.ShowModal();
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrMaterialPanel::OnCheckPropUsage(wxCommandEvent&)
{
    std::map<wxString,CUIEdAttrPropSet>::iterator it;

    for (it=m_Shared->m_PropSetDict.begin(); it!=m_Shared->m_PropSetDict.end(); ++it)
    {
        it->second.m_Temp=0;
    }
    for (it=m_Shared->m_MaterialDict.begin(); it!=m_Shared->m_MaterialDict.end(); ++it)
    {
        it->second.m_Temp=0;
    }

    m_Shared->m_Project.m_GroupData->CheckUsage();

    CIwASDUICheckPropUsage dlg(this,m_Shared->m_MaterialDict,L"Material Usage");
    dlg.ShowModal();
}

//------------------------------------------------------------------------------
CIwASDUIEdAttrMaterialPanel::CIwASDUIEdAttrMaterialPanel(wxWindow* parent,CUIEdAttrShared* shared) : CIwASDUIEdAttrPanel(parent,shared,false)
{
    m_Shared->m_UIEdAttrMaterialPanel=this;

    m_PropPanel->m_HideInline=false;

    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/32x32/");

    wxBitmapButton* button=new wxBitmapButton(this,CTRLID_ADD,wxBitmap(dir+L"add32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Add Material...");
    m_Bar->Add(button);

    button=new wxBitmapButton(this,CTRLID_CHECKPROPUSAGE,wxBitmap(dir+L"check_names32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Check Usage...");
    m_Bar->Add(button);
    m_Type=L"material";
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrMaterialPanel::OnAdd(wxCommandEvent&)
{
    m_Shared->MakeNew(ATTRSTRING_PTR,L"CIwMaterial");
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnAdd(wxCommandEvent&)
{
    m_Shared->MakeNew(ATTRSTRING_PTR,L"CIwPropertySet");
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnStyle(wxCommandEvent&)
{
    m_Shared->m_Project.SetChanged(true);
    m_Shared->SetStyleSheet(m_Style->GetStringSelection());
    m_Shared->SetSelection(m_Shared->m_SelElem,SELSOURCE_DATA);
    Reset();
    ResetProp();

    if (CIwTheHost.m_Link==NULL) return;

    char text[256];
    strcpy(text,CIwTheHost.m_Shared.m_Project.m_StyleSheet.mb_str());
    CIwTheHost.m_Link->SetData(CIwViewerUI::SET_STYLESHEET,text);
}

//------------------------------------------------------------------------------
class CIwASDUIEdAttrStyleToDefault : public CIwStyleDialog
{
public:
    CIwASDUIEdAttrStyleToDefault(wxWindow* parent) : CIwStyleDialog(parent,L"Copy From Stylesheet to Default")
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        sizer->AddSpacer(10);
        wxStaticText* text=new wxStaticText(this,wxID_ANY,
                                            L"Do you wish to copy the current property set in this style sheet into the default stylesheet,",wxPoint(-1,-1),wxSize(600,-1),wxALIGN_CENTRE);
        sizer->Add(text,0,wxEXPAND);
        text=new wxStaticText(this,wxID_ANY,
                              L"or copy all the property sets in this style sheet into the default stylesheet?",wxPoint(-1,-1),wxSize(600,-1),wxALIGN_CENTRE);
        sizer->Add(text,0,wxEXPAND);
        sizer->AddSpacer(10);

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"Current"));
        bar->Add(new CIwStyleButton(this,wxID_APPLY,L"All"));
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"),CIwStyleButtonBar::SPACE_PROP);
        sizer->Add(bar,0,wxEXPAND);

        sizer->Fit(this);
    }
    void OnApply(wxCommandEvent& e)
    {
        EndModal(e.GetId());
    }
    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDUIEdAttrStyleToDefault, CIwStyleDialog)
    EVT_BUTTON(wxID_APPLY,CIwASDUIEdAttrStyleToDefault::OnApply)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnToDefault(wxCommandEvent&)
{
    if (m_Sect==NULL) return;

    CIwASDUIEdAttrStyleToDefault dlg(this);
    int ret=dlg.ShowModal();
    if (ret==wxID_CANCEL) return;

    CIwAttrData* data=m_Sect->FindData(L"name",0);
    if (data==NULL) return;

    CIwAttrInstance* first=NULL;
    if (ret==wxID_OK)
        first=m_Shared->ToDefault(m_Shared->m_Project.m_StyleSheet,data->m_Items[0].m_String,m_Sect);
    else
        first=m_Shared->ToDefault(m_Shared->m_Project.m_StyleSheet);

    m_Shared->SaveDefault(first);
    m_Shared->SetStyleSheet(m_Shared->m_Project.m_StyleSheet);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrStylePanel::OnSave(wxCommandEvent&)
{
    CUIEdProjectUI* ui=NULL;

    if (m_Sect!=NULL && m_Sect->m_File!=NULL)
    {
        CUIEdProjectUI::Group* group=(CUIEdProjectUI::Group*)m_Sect->m_File;
        while (group->m_Parent!=NULL)
            group=(CUIEdProjectUI::Group*)group->m_Parent;
        ui=group->m_UI;
    }

    if (ui==NULL) return;

    ui->CheckSave(true);

    int num=0;
    m_Shared->m_Project.m_GroupData->SaveFiltered(CIwTheHost.m_AltGroupFile,num);
}

//------------------------------------------------------------------------------
void CIwASDUIEdAttrMaterialPanel::OnSave(wxCommandEvent&)
{
    CUIEdProjectUI* ui=m_Shared->m_CurrUI;

    if (m_Sect!=NULL && m_Sect->m_File!=NULL)
    {
        CUIEdProjectUI::Group* group=(CUIEdProjectUI::Group*)m_Sect->m_File;
        while (group->m_Parent!=NULL)
            group=(CUIEdProjectUI::Group*)group->m_Parent;
        ui=group->m_UI;
    }

    ui->CheckSave(true);

    int num=0;
    m_Shared->m_Project.m_GroupData->SaveFiltered(CIwTheHost.m_AltGroupFile,num);
}

//------------------------------------------------------------------------------
CIwASDUIEdAttrPanel::CIwASDUIEdAttrPanel(wxWindow* parent,CUIEdAttrShared* shared,bool hasCombo) : wxPanel(parent),m_GameMgrMeta(CIwTheFileMetaMgr),
    m_CheckBox(NULL),m_Selecting(false),m_NeedsReset(false),m_Shared(shared),m_Sect(NULL),m_OldSect(NULL),currData(NULL),m_PopupMenu(NULL)
{
    m_Shim.m_Frame=this;
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    m_Bar=new CIwStyleButtonBar(this,false);
    m_Sizer->Add(m_Bar,0,wxEXPAND);

    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/32x32/");

    wxBitmapButton* button=new wxBitmapButton(this,CTRLID_SAVE,wxBitmap(dir+L"saveui32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Save UI File");
    m_Bar->Add(button);

    button=new wxBitmapButton(this,CTRLID_RELOAD,wxBitmap(dir+L"restore32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Reload UI File");
    m_Bar->Add(button);

    //m_OpenEdit=new CIwStyleButton(this,CTRLID_OPENEDIT,L"Open for Edit");
    //m_OpenEdit->Enable(false);
    //m_Bar->Add(m_OpenEdit);

    button=new wxBitmapButton(this,CTRLID_REFRESH,wxBitmap(dir+L"refresh32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Refresh Viewer Data");
    m_Bar->Add(button);

    m_AreaSizer=new wxBoxSizer(wxHORIZONTAL);
    m_Sizer->Add(m_AreaSizer,1,wxALL|wxEXPAND,0);

    m_Tree=new CTree(this);
    m_AreaSizer->Add(m_Tree,2,wxEXPAND);

    if (hasCombo)
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);

        wxArrayString strings;
        strings.Add(L"Basic Element View");
        strings.Add(L"Advanced Element View");
        m_Selector=new wxComboBox(this,CTRLID_SELECTOR,strings[0],wxPoint(-1,-1),wxSize(-1,-1),strings,wxCB_READONLY);
        m_PropPanel=new CIwAttrGrid(this,m_GameMgrMeta);

        sizer->Add(m_Selector,0,wxEXPAND);
        sizer->Add(m_PropPanel,1,wxEXPAND);

        m_AreaSizer->Add(sizer,3,wxEXPAND);
    }
    else
    {
        m_Selector=NULL;
        m_PropPanel=new CIwAttrGrid(this,m_GameMgrMeta);
        m_AreaSizer->Add(m_PropPanel,3,wxEXPAND);
    }

    m_Sizer->Layout();

    SetupIcons();
}

//------------------------------------------------------------------------------
wxSizerItem* CUIEdAttrPane::Create(wxWindow* parent)
{
    switch (m_Type)
    {
    case ATTRPANETYPE_UNITY:
        m_Panel=new CIwASDUIEdAttrUnityPanel(parent,m_Shared);
        break;
    case ATTRPANETYPE_STYLE:
        m_Panel=new CIwASDUIEdAttrStylePanel(parent,m_Shared);
        break;
    case ATTRPANETYPE_MATERIAL:
        m_Panel=new CIwASDUIEdAttrMaterialPanel(parent,m_Shared);
        break;
    }

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,0,NULL);
}

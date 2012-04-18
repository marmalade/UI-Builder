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
BEGIN_EVENT_TABLE(CIwUIEdPaletteFrame, wxTreeCtrl)
    EVT_TREE_BEGIN_DRAG(CTRLID_TREE,CIwUIEdPaletteFrame::OnDrag)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwUIEdMediaFrame, wxPanel)
    EVT_COMBOBOX(CTRLID_SELECT,CIwUIEdMediaFrame::OnSelect)
    EVT_LIST_BEGIN_DRAG(CTRLID_LIST,CIwUIEdMediaFrame::OnDrag)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
wxDragResult CIwUIEdPaletteDropTarget::OnData(wxCoord x,wxCoord y,wxDragResult def)
{
    if (!GetData())
        return wxDragNone;

    wxTextDataObject *dobj=(wxTextDataObject*)m_dataObject;
    return OnDropText(x,y,dobj->GetText(),def);
}

//------------------------------------------------------------------------------
wxDragResult CIwUIEdPaletteDropTarget::OnDropText(wxCoord x, wxCoord y, wxString data,wxDragResult def)
{
    bool move=false;
    CIwAttrInstance* inst=m_Frame->m_Shared->m_DragInst;
    if (data.StartsWith(L"!"))
        return wxDragNone;

    if (lastId.IsOk())
        m_Frame->SetItemDropHighlight(lastId,false);

    lastId.Unset();

    CIwAttrInstance* parent=m_Frame->m_Shared->m_Project.m_Palette.m_Group.m_Inst;

    int flags;
    wxTreeItemId id=m_Frame->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
    {
        if (m_Frame->GetItemParent(id)!=m_Frame->GetRootItem())
            id=m_Frame->GetItemParent(id);

        CIwAttrTreeItem *data=(CIwAttrTreeItem*)m_Frame->GetItemData(id);
        if (data!=NULL && data->m_Inst!=NULL)
            parent=data->m_Inst;
    }

    m_Frame->Drop(inst,parent);
    return def;
}

//------------------------------------------------------------------------------
void CIwUIEdPaletteFrame::Drop(CIwAttrInstance* inst,CIwAttrInstance* parent)
{
    if (parent==m_Shared->m_Project.m_Palette.m_Group.m_Inst)
    {
        wxTextEntryDialog dlg(CIwTheFrame,L"Please supply a name for the section",L"Add new section");
        if (dlg.ShowModal()!=wxID_OK)
            return;

        CIwAttrNote Note;
        std::vector<wxString> argv2;

        Note.m_Name=L"CIwUIElement";
        Note.m_Data=L"{";
        argv2.push_back(Note.m_Data);

        parent=parent->AddFromNote(Note.m_Name,Note,argv2,parent);

        argv2.clear();
        Note.m_Name=L"name";
        Note.m_Data=L"ExpMetaPalette/"+dlg.GetValue();
        argv2.push_back(Note.m_Data);
        parent->AddFromNote(Note.m_Name,Note,argv2,parent);
    }

    m_Shared->m_Project.m_Palette.Add(inst,parent);
    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdPaletteDropTarget::OnLeave()
{
    if (lastId.IsOk())
        m_Frame->SetItemDropHighlight(lastId,false);

    lastId.Unset();
}

//------------------------------------------------------------------------------
wxDragResult CIwUIEdPaletteDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    if (m_Frame->m_Shared->m_CurrUI==NULL)
        return wxDragNone;

    if (m_Frame->m_Shared->m_DragMode.StartsWith(L"!"))
        return wxDragNone;

    if (lastId.IsOk())
        m_Frame->SetItemDropHighlight(lastId,false);

    int flags;
    wxTreeItemId id=m_Frame->HitTest(wxPoint(x,y),flags);
    if (id.IsOk())
    {
        if (m_Frame->GetItemParent(id)!=m_Frame->GetRootItem())
            id=m_Frame->GetItemParent(id);

        m_Frame->SetItemDropHighlight(id,true);
    }

    lastId=id;

    return wxDragCopy;
}

//-----------------------------------------------------------------------------
void CIwUIEdPaletteFrame::OnDrag(wxTreeEvent& e)
{
    wxTreeItemId id=e.GetItem();
    if (!id.IsOk()) return;

    CIwAttrTreeItem* data=(CIwAttrTreeItem*)GetItemData(id);
    if (data==NULL || data->m_Inst==NULL) return;

    CIwAttrInstance* parent=data->m_Inst->m_Parent->m_Instance;
    if (parent==NULL || !parent->m_Class->m_Name.IsSameAs(L"CIwUIElement",false))
        return;

    m_Shared->m_DragInst=data->m_Inst;
    m_Shared->m_DragMode=L"!ExpMetaPalette";

    CIwAttrData* dataName=data->m_Inst->FindData(L"name");
    CIwAttrData* parentName=parent->FindData(L"name");

    wxString name=L"!"+parentName->m_Items[0].m_String+L" "+dataName->m_Items[0].m_String;

    strcpy(CIwUIEdDropTarget::s_CurrData,name.Mid(1).mb_str());
    wxTextDataObject text(name);
    wxDropSource source(text,this);
    source.DoDragDrop(wxDrag_CopyOnly);
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
    NULL
};

//------------------------------------------------------------------------------
void CIwUIEdPaletteFrame::SetupIcons()
{
    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/16x16/");

    wxImageList* list=new wxImageList(16,16);

    for (int i=0; iconList[i]!=NULL; i++)
    {
        wxString file=dir+wxString(iconList[i]).BeforeFirst(';');
        file.Replace(L"\\",L"/");
        list->Add(wxBitmap(file,wxBITMAP_TYPE_PNG));
    }

    AssignImageList(list);
}

//------------------------------------------------------------------------------
int CIwUIEdPaletteFrame::GetIcon(CIwAttrInstance* inst)
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
void CIwUIEdPaletteFrame::Add(wxTreeItemId parent,CIwAttrInstance* inst,int level)
{
    for (int i=0; i<(int)inst->m_Data.size(); i++)
    {
        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
        {
            wxTreeItemId id;
            CIwAttrInstance* inst2=inst->m_Data[i]->m_Items[0].m_Inst;
            CIwAttrData* data=inst2->FindData(L"name",0);
            if (data==NULL)
                continue;

            int icon=GetIcon(inst2);
            wxString name=data->m_Items[0].m_String;

            if (name.Contains(L"/"))
            {
                if (name.Contains(L":") && m_Mode==1)
                    continue;

                if (!name.Contains(L":") && m_Mode==2)
                    continue;

                if (name.Contains(L":p") && m_Mode==2)
                    continue;

                id=AppendItem(parent,name.AfterFirst('/'),icon,icon,new CIwAttrTreeItem(inst2));
            }
            else
                id=AppendItem(parent,name,icon,icon,new CIwAttrTreeItem(inst2));

            if (level<1)
                Add(id,inst2,level+1);
        }
    }
}

//------------------------------------------------------------------------------
void CIwUIEdPaletteFrame::Reset()
{
    bool empty=GetCount()==0;
    std::vector<wxString> list;
    GetOpened(list,GetRootItem());

    DeleteAllItems();
    wxTreeItemId root=AddRoot(L"");
    if (m_Shared->m_Project.m_Palette.m_Group.m_Inst!=NULL)
        Add(root,m_Shared->m_Project.m_Palette.m_Group.m_Inst,0);

    int offset=0;
    if (empty)
        SetOpenedAll(GetRootItem());
    else
        SetOpened(list,offset,GetRootItem());
}

//------------------------------------------------------------------------------
wxSizerItem* CUIEdPalettePane::Create(wxWindow* parent)
{
    m_Tree=new CIwUIEdPaletteFrame(parent,m_Shared,m_Mode);
    wxTreeItemId root=m_Tree->AddRoot(L"");

    return new wxSizerItem(m_Tree,1,wxEXPAND|wxALL,0,NULL);
}

//------------------------------------------------------------------------------
void CUIEdPalettePane::Load(std::vector<wxString>& argv)
{
    if (argv.empty()) return;

    if (argv[0].IsSameAs(L"elements"))
        m_Mode=1;

    if (argv[0].IsSameAs(L"others"))
        m_Mode=2;
}

//------------------------------------------------------------------------------
void CUIEdPalettePane::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Shared->m_Project.m_Palette.HasChanged())
        dataList.push_back(&m_Shared->m_Project.m_Palette);
}

//-----------------------------------------------------------------------------
void CIwUIEdMediaFrame::OnDrag(wxListEvent& e)
{
    m_Shared->m_DragInst=NULL;
    m_Shared->m_DragMode=L"!"+m_Select->GetValue();

    wxString name=L"!"+m_Select->GetValue()+L" "+m_List->GetItemText(e.GetIndex());

    strcpy(CIwUIEdDropTarget::s_CurrData,name.Mid(1).mb_str());
    wxTextDataObject text(name);
    wxDropSource source(text,this);
    source.DoDragDrop(wxDrag_CopyOnly);
}

//------------------------------------------------------------------------------
void CIwUIEdMediaFrame::OnSelect(wxCommandEvent&)
{
    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdMediaFrame::Add(std::vector<wxString>& exts)
{
    if (m_Shared->m_Project.m_GroupData==NULL) return;

    wxArrayString strings;
    m_Shared->m_Project.m_GroupData->GetStrings(strings,exts);

    for (int i=0; i<(int)strings.size(); i++)
    {
        int icon=-1;
        wxBitmap* bmp=m_Shared->GetFileBitmap(strings[i],m_Shared->m_Project.m_GroupData);
        if (bmp!=NULL)
        {
            icon=m_Images->GetImageCount();
            m_Images->Add(*bmp);
        }

        m_List->InsertItem(i,strings[i],icon);
    }
}

//------------------------------------------------------------------------------
void CIwUIEdMediaFrame::Add(std::map<wxString,CUIEdAttrPropSet>& dict)
{
    int i=0;
    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=dict.begin(); it!=dict.end(); ++it)
    {
        int icon=-1;
        if (it->second.m_Bmp.IsOk())
        {
            icon=m_Images->GetImageCount();
            m_Images->Add(it->second.m_Bmp);
        }

        m_List->InsertItem(i++,it->first,icon);
    }
}

//------------------------------------------------------------------------------
CIwUIEdMediaFrame::CIwUIEdMediaFrame(wxWindow *parent,CUIEdAttrShared* shared) : wxPanel(parent),m_Shared(shared)
{
    m_Shared->m_MediaFrame=this;
    m_Images=new wxImageList(20,20);

    wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);

    wxArrayString strings;
    strings.Add(L"texture");
    strings.Add(L"gxfont");
    strings.Add(L"material");
    strings.Add(L"style");
    m_Select=new wxComboBox(this,CTRLID_SELECT,strings[0],wxPoint(-1,-1),wxSize(-1,-1),strings,wxCB_READONLY);
    sizer->Add(m_Select,0,wxEXPAND);

    m_List=new wxListCtrl(this,CTRLID_LIST,wxPoint(-1,-1),wxSize(-1,-1),wxLC_SINGLE_SEL|wxLC_SMALL_ICON);
    m_List->AssignImageList(m_Images,wxIMAGE_LIST_SMALL);
    sizer->Add(m_List,1,wxEXPAND);

    sizer->Layout();
}

//------------------------------------------------------------------------------
void CIwUIEdMediaFrame::Reset()
{
    std::vector<wxString> exts;

    m_List->ClearAll();
    m_Images->RemoveAll();

    if (m_Select->GetValue()==L"texture")
    {
        exts.push_back(L"tga");
        exts.push_back(L"bmp");
        exts.push_back(L"png");
        exts.push_back(L"gif");
        Add(exts);
    }
    else if (m_Select->GetValue()==L"gxfont")
    {
        exts.push_back(L"gxfont");
        Add(exts);
    }
    else if (m_Select->GetValue()==L"material")
        Add(m_Shared->m_MaterialDict);
    else if (m_Select->GetValue()==L"style")
        Add(m_Shared->m_PropSetDict);
}

//------------------------------------------------------------------------------
wxSizerItem* CIwUIEdMediaPane::Create(wxWindow* parent)
{
    m_Tree=new CIwUIEdMediaFrame(parent,m_Shared);

    return new wxSizerItem(m_Tree,1,wxEXPAND|wxALL,0,NULL);
}

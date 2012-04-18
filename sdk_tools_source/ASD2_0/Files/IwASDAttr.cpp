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
// CIwASDSourcePanel
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDAttrPanel, wxPanel)
    EVT_STC_SAVEPOINTLEFT(CTRLID_SOURCE,CIwASDAttrPanel::OnSaveOff)
    EVT_STC_SAVEPOINTREACHED(CTRLID_SOURCE,CIwASDAttrPanel::OnSaveOn)
    EVT_BUTTON(CTRLID_SAVE,CIwASDAttrPanel::OnSave)
    EVT_BUTTON(CTRLID_RELOAD,CIwASDAttrPanel::OnReload)

    EVT_TREE_SEL_CHANGED(CTRLID_TREE,CIwASDAttrPanel::OnItemActivated)
    //EVT_TREE_ITEM_MENU(CTRLID_TREE,CIwASDAttrPanel::OnPopupMenu)
    EVT_BUTTON(CTRLID_REMOVE,CIwASDAttrPanel::OnRemove)
    EVT_BUTTON(CTRLID_SWAP,CIwASDAttrPanel::OnSwap)
    EVT_BUTTON(CTRLID_OPENEDIT,CIwASDAttrPanel::OnOpenEdit)
    //EVT_IDLE(CIwASDAttrPanel::OnIdle)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::CShim::SetChanged(bool Changed)
{
    m_Frame->m_Layout->m_Data->SetChanged(Changed);
    m_Frame->OnChanged();
    bool enabled=m_Frame->m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)!=0;
    m_Frame->m_PropPanel->Enable(enabled);
    /*if(!enabled)
        m_Frame->m_PropPanel->SetBackgroundColour(wxColour(128,128,128));
       else
        m_Frame->m_PropPanel->SetBackgroundColour(m_Frame->GetBackgroundColour());
       if(m_Frame->m_Text!=NULL)
       {
        if(!enabled)
            m_Frame->m_Text->StyleSetForeground(wxSTC_STYLE_DEFAULT,*wxBLACK);
        else
            m_Frame->m_Text->StyleSetForeground(wxSTC_STYLE_DEFAULT,*wxWHITE);
       }*/
}


//-----------------------------------------------------------------------------
wxString CIwASDAttrPanel::CShim::GetBaseDir(bool fileBase)
{
    if (fileBase)
        return m_Frame->m_Layout->m_Data->m_File->m_Paths[0]->m_Parent->c_str();
    else
        return m_Frame->m_Layout->m_Data->m_File->m_Paths[0]->GetRoot()->c_str();
}

//-----------------------------------------------------------------------------
// text data control
class CIwAttrTextComboResTemplate : public CIwAttrTextCombo
{
protected:
    CIwAttrTextCombo* m_ExtList;
    wxArrayString m_Exts;
public:
    CIwAttrTextComboResTemplate(wxWindow* Parent,CIwAttrData* Data,int Num,wxArrayString& Strings,std::vector<CIwAttrData*>& Alts,CIwAttrTextCombo* extList,
                                wxArrayString& exts) : CIwAttrTextCombo(Parent,Data,Num,Strings,Alts),m_ExtList(extList),m_Exts(exts)
    {
        SetStringSelection(m_Data->m_Items[m_Num].m_String);
        Reset();
    }

    virtual void OnCombo(wxCommandEvent& e)
    {
        CIwAttrTextCombo::OnCombo(e);
        Reset();
    }
    void Reset()
    {
        wxArrayString exts;
        int found=GetSelection(),num=0;
        if (found!=wxNOT_FOUND)
            if (GetStringSelection()!=GetString(found))
                found=wxNOT_FOUND;

        if (found!=wxNOT_FOUND)
        {
            m_Data->m_Items[m_Num-1].m_String=m_Exts[found];
            m_Data->m_Items[m_Num].m_String=GetStringSelection();

            for (int i=0; i<(int)GetCount(); i++)
            {
                if (GetString(i)==m_Data->m_Items[m_Num].m_String)
                {
                    if (exts.Index(m_Exts[i])==wxNOT_FOUND)
                        exts.Add(m_Exts[i]);

                    num++;
                }
            }
        }
        else
        {
            for (int i=0; i<(int)GetCount(); i++)
            {
                if (GetString(i)==m_Data->m_Items[m_Num].m_String)
                {
                    if (exts.Index(m_Exts[i])==wxNOT_FOUND)
                        exts.Add(m_Exts[i]);

                    num++;
                    if (m_Data->m_Items[m_Num-1].m_String==m_Exts[i])
                        found=i;
                }
            }
        }

        if (found==-1)
        {
            exts.clear();
            m_Data->m_Mgr->GetResTemplateStrings(exts);
            m_ExtList->Enable(true);
        }
        else if (num==1)
        {
            exts.clear();
            m_Data->m_Mgr->GetResTemplateStrings(exts);
            m_ExtList->Enable(false);
        }
        else
            m_ExtList->Enable(true);

        m_ExtList->Clear();
        m_ExtList->Append(exts);
        m_ExtList->SetStringSelection(m_Data->m_Items[m_Num-1].m_String);
    }
};

//--------------------------------------------------------------------------------
wxWindow* CIwASDAttrPanel::CShim::GetDlgItem(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts)
{
    if ((Data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_USETEMPLATE)
    {
        wxPanel* panel=new wxPanel(Parent);
        wxSizer* sizer=new wxBoxSizer(wxHORIZONTAL);

        wxArrayString exts,templates,templateExts;
        Data->m_Mgr->GetResTemplateStrings(exts);

        CIwAttrInstance* inst=m_Frame->GetResTemplate();
        if (inst!=NULL)
        {
            for (int i=0; i<(int)inst->m_Data.size(); i++)
            {
                if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
                    continue;

                CIwAttrInstance* inst2=inst->m_Data[i]->m_Items[0].m_Inst;

                for (int j=0; j<(int)inst2->m_Data.size(); j++)
                {
                    if ((inst2->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_STRING)
                    {
                        if (inst2->m_Data[j]->m_Member->m_Name.IsSameAs(L"name",false))
                        {
                            templates.Add(inst2->m_Data[j]->m_Items[0].m_String);
                            templateExts.Add(Data->m_Mgr->FindResTemplateString(inst2->m_Class->m_Name));
                            break;
                        }
                    }
                }
            }
        }

        CIwAttrTextCombo* ext=new CIwAttrTextCombo(panel,Data,0,exts,Alts);
        sizer->Add(ext);
        sizer->Add(new CIwAttrTextComboResTemplate(panel,Data,1,templates,Alts,ext,templateExts));
        panel->SetSizer(sizer);

        return panel;
    }

    return NULL;
}


class CIwASDAttrPanelOpenEdit : public CIwStyleDialog
{
public:
    CIwASDAttrPanelOpenEdit(wxWindow* Parent,const wxString& fileName) : CIwStyleDialog(Parent,L"Opening Read-only File...")
    {
        wxSizer *sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);
        sizer->Add(new wxStaticText(this,wxID_ANY,wxString::Format(L"Editing file: %s",fileName.c_str())),0,wxALIGN_CENTER_HORIZONTAL|wxTOP,16);
        sizer->Add(new wxStaticText(this,wxID_ANY,L"Please choose how to open this file for editing..."),0,wxALIGN_CENTER_HORIZONTAL|wxBOTTOM,16);
        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        sizer->Add(bar,0,wxEXPAND);

        CIwStyleButton* sc=new CIwStyleButton(this,wxID_YES,L"Check out of Source Control",wxSize(3,1));
        if (!CIwTheFileSrcCtrl.m_Active)
            sc->Enable(false);

        bar->Add(sc,CIwStyleButtonBar::SPACE_PROP);

        bar->Add(new CIwStyleButton(this,wxID_NO,L"Make Read-write",wxSize(2,1)));
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel",wxSize(2,1)));
        bar->AddSpace(CIwStyleButtonBar::SPACE_PROP);

        sizer->Layout();
        sizer->Fit(this);
    }
    void OnClose(wxCommandEvent& e) { EndDialog(e.GetId()); }

    DECLARE_EVENT_TABLE()
};
BEGIN_EVENT_TABLE(CIwASDAttrPanelOpenEdit,CIwStyleDialog)
    EVT_BUTTON(wxID_YES,CIwASDAttrPanelOpenEdit::OnClose)
    EVT_BUTTON(wxID_NO,CIwASDAttrPanelOpenEdit::OnClose)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnOpenEdit(wxCommandEvent& e)
{
    CIwASDAttrPanelOpenEdit dlg(this,m_Layout->m_Data->m_File->m_Paths[0]->c_str(true));
    switch (dlg.ShowModal())
    {
    case wxID_YES:
        CIwTheFileSrcCtrl.Run(L"edit",m_Layout->m_Data->m_File->m_Paths[0]->c_str());
        break;
    case wxID_NO:
#ifdef I3D_OS_WINDOWS
        SetFileAttributes(m_Layout->m_Data->m_File->m_Paths[0]->c_str().c_str(),FILE_ATTRIBUTE_NORMAL);
#else
        chmod(m_Layout->m_Data->m_File->m_Paths[0]->c_str().mb_str(),S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif
        break;
    default:
        return;
    }
    Reset();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnSwap(wxCommandEvent& e)
{
    OnSave(e);

    if (m_Layout->m_DataAttr!=NULL)
        m_Layout->SetData(m_Layout->m_DataAttr->m_File,true);
    else
        m_Layout->SetData(m_Layout->m_DataSource->m_File,false);

    OnReload(e);
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnRemove(wxCommandEvent& event)
{
    if (m_Sect==m_Layout->m_DataAttr->m_Group.m_Inst) return;

    std::vector<CIwAttrData*>::iterator it=m_Sect->m_Parent->m_Instance->m_Data.begin();
    for (; it<m_Sect->m_Parent->m_Instance->m_Data.end(); )
    {
        if ((*it)->m_Group==m_Sect->m_Parent)
            m_Sect->m_Parent->m_Instance->m_Data.erase(it);
        else if ((*it)==m_Sect->m_Parent)
            m_Sect->m_Parent->m_Instance->m_Data.erase(it);
        else
            ++it;
    }
    delete m_Sect;
    m_Sect=m_Layout->m_DataAttr->m_Group.m_Inst;
    m_Sect->SetupDlg(&m_Shim);

    m_Sect->SetChanged(true);
    Reset();
}
//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnItemActivated(wxTreeEvent& event)
{
    wxTreeItemId Id=event.GetItem();
    if (!Id) return;

    CIwAttrTreeItem *Data=(CIwAttrTreeItem*)m_Tree->GetItemData(Id);

    if (Data==NULL)
        return;

    m_Sect=Data->m_Inst;
    m_Sect->SetupDlg(&m_Shim);
    ResetProp();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnSaveOn(wxStyledTextEvent& e)
{
    if (m_ReLoaded) return;

    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnSaveOff(wxStyledTextEvent& e)
{
    m_Layout->m_Data->SetChanged(true);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnSave(wxCommandEvent&)
{
    if (!GetText()) return;

    m_Layout->m_Data->Save();

    if (m_Text!=NULL)
        m_Text->SetSavePoint();

    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
bool CIwASDAttrPanel::GetText()
{
    if (!m_Layout->m_Data->HasChanged()) return false;

    if (m_Layout->m_Data==NULL) return false;

    if (m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)==0) return false;

    if (m_Layout->m_DataSource!=NULL)
        m_Layout->m_DataSource->m_Buffer=m_Text->GetText();

    return true;
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::OnReload(wxCommandEvent&)
{
    m_Layout->m_Data->Load(true);

    m_ReLoaded=false;

    if (m_Layout->m_DataAttr!=NULL)
    {
        m_Sect=m_Layout->m_DataAttr->m_Group.m_Inst;
        m_Sect->SetupDlg(&m_Shim);
    }
    else
        m_Sect=NULL;

    Reset();

    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
CIwASDAttrPanel::CIwASDAttrPanel(wxWindow* parent,CIwASDAttrLayout* layout) : wxPanel(parent),m_GameMgrMeta(CIwTheFileMetaMgr),
    m_Text(NULL),m_Tree(NULL),m_Sect(NULL),m_Layout(layout)
{
    m_Shim.m_Frame=this;

    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    m_Bar=new CIwStyleButtonBar(this,false);
    m_Sizer->Add(m_Bar,0,wxEXPAND);

    m_Bar->Add(new CIwStyleButton(this,CTRLID_SAVE,L"Save"));
    m_Bar->Add(new CIwStyleButton(this,CTRLID_RELOAD,L"Restore"));
    m_Swap=new CIwStyleButton(this,CTRLID_SWAP,L"To Text");
    m_Swap->Enable(false);
    m_Bar->Add(m_Swap);

    m_OpenEdit=new CIwStyleButton(this,CTRLID_OPENEDIT,L"Open for Edit");
    m_OpenEdit->Enable(false);
    m_Bar->Add(m_OpenEdit);

    m_AreaSizer=new wxBoxSizer(wxHORIZONTAL);
    m_Sizer->Add(m_AreaSizer,1,wxALL|wxEXPAND,0);

    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::Reset()
{
    Freeze();
    if (m_Text!=NULL && m_Layout->m_DataSource==NULL)
    {
        m_AreaSizer->Clear(true);
        m_Text=NULL;
    }

    if (m_Tree!=NULL && m_Layout->m_DataAttr==NULL)
    {
        m_AreaSizer->Clear(true);
        m_Tree=NULL;
    }

    if (m_Text==NULL && m_Layout->m_DataSource!=NULL)
    {
        m_Text=new wxStyledTextCtrl(this,CTRLID_SOURCE);
        wxFont Font(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"courier");
        m_Text->SetReadOnly(true);
        m_Text->SetWrapMode(wxSTC_WRAP_NONE);
        m_Text->SetTabWidth(4);
        m_Text->StyleSetFont(wxSTC_STYLE_DEFAULT,Font);

        m_AreaSizer->Add(m_Text,1,wxALL|wxEXPAND,0);

        m_Swap->SetLabel(L"To Data");

        m_Swap->Enable(CIwTheFileModule->CheckAttrFileType(m_Layout->m_DataSource->m_File->m_Paths[0]->c_str()));
    }

    if (m_Tree==NULL && m_Layout->m_DataAttr!=NULL)
    {
        m_Tree=new wxTreeCtrl(this,CTRLID_TREE,wxPoint(-1,-1),wxSize(-1,-1),wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_HIDE_ROOT);
        m_PropPanel=new wxScrolledWindow(this);
        m_PropPanel->SetAutoLayout(true);

        m_AreaSizer->Add(m_Tree,1,wxEXPAND);
        m_AreaSizer->Add(m_PropPanel,3,wxEXPAND);

        m_PropSizer=new CIwStyleCtrlGroup(true,true,true);
        m_PropPanel->SetSizer(m_PropSizer);
        m_PropSizer->Layout();

        m_Swap->SetLabel(L"To Text");
        m_Swap->Enable(true);
    }

    if (m_Layout->m_Data!=NULL)
    {
        m_Layout->m_Data->m_File->m_File.SetupFileAttr(m_Layout->m_Data->m_File->m_Paths[0]->c_str());

        if (m_Layout->m_Data->m_File->m_File.isReadOnly)
            m_Layout->m_Data->SetState(CIwASDData::STATE_READWRITE,CIwASDData::STATE_READWRITE);
        else
            m_Layout->m_Data->SetState(0,CIwASDData::STATE_READWRITE);

        if (m_Layout->m_Data->GetState(CIwASDData::STATE_NOTEDITABLE)==0 && m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)==0)
            m_OpenEdit->Enable(true);
        else
            m_OpenEdit->Enable(false);
    }

    if (m_Text!=NULL && m_Layout->m_DataSource!=NULL)
    {
        bool read=m_Layout->m_DataSource->GetState(CIwASDData::STATE_READWRITE)==0;
        m_Text->SetReadOnly(false);
        m_Text->Clear();
        //if(read)
        //	m_Text->StyleSetBackground(wxSTC_STYLE_DEFAULT,wxColour(128,128,128));
        //else
        //	m_Text->StyleSetBackground(wxSTC_STYLE_DEFAULT,wxColour(255,255,255));
        m_Text->SetText(m_Layout->m_DataSource->m_Buffer);
        m_Text->SetReadOnly(read);
        m_Text->EmptyUndoBuffer();
    }

    if (m_Tree!=NULL && m_Layout->m_DataAttr!=NULL)
    {
        m_Tree->DeleteAllItems();
        wxTreeItemId Id=m_Tree->AddRoot(L"");

        if (m_Layout->m_DataAttr->m_Group.m_Inst!=NULL)
        {
            m_Layout->m_DataAttr->m_Group.m_Inst->AddToTree(m_Tree,Id);

            if (m_Sect==NULL)
            {
                m_Sect=m_Layout->m_DataAttr->m_Group.m_Inst;
                m_Sect->SetupDlg(&m_Shim);
            }

            m_Tree->EnsureVisible(m_Sect->m_TreeId);
            m_Tree->Expand(m_Sect->m_TreeId);
            m_Tree->SelectItem(m_Sect->m_TreeId);

            ResetProp();
        }
    }

    Thaw();
    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
void CIwASDAttrPanel::ResetProp()
{
    if (m_Layout->m_DataAttr->m_Group.m_Inst==NULL || m_Sect==NULL) return;

    Freeze();

    m_PropSizer->Clear(true);

    //m_Name=NULL;

    m_GameMgrMeta.FillDialog(m_PropSizer,m_PropPanel,m_Sect,NULL); //&Strings);
    if (m_Sect!=m_Layout->m_DataAttr->m_Group.m_Inst)
        m_PropSizer->Add(new CIwStyleButton(m_PropPanel,CTRLID_REMOVE,L"Remove This"),L"");

    //if(m_Sect==m_Layout->m_DataAttr->m_Group.m_Inst)
    //	AddRootEdit();

    wxSize Size=m_PropSizer->GetMinSize();
    m_PropPanel->SetScrollRate(20,20);
    m_PropPanel->SetVirtualSize(-1,Size.y);
    m_PropPanel->Layout();
    m_PropSizer->Layout();

    bool enabled=m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)!=0;
    m_PropPanel->Enable(enabled);
    /*if(!enabled)
        m_PropPanel->SetBackgroundColour(wxColour(128,128,128));
       else
        m_PropPanel->SetBackgroundColour(GetBackgroundColour());
       if(m_Text!=NULL)
       {
        if(!enabled)
            m_Text->StyleSetForeground(wxSTC_STYLE_DEFAULT,*wxBLACK);
        else
            m_Text->StyleSetForeground(wxSTC_STYLE_DEFAULT,*wxWHITE);
       }*/

    Thaw();
}

//--------------------------------------------------------------------------------
CIwASDAttrPanel::~CIwASDAttrPanel()
{
}

//--------------------------------------------------------------------------------
void CIwASDFileDataAttr::Save()
{
    if (m_File->m_Paths.empty()) return;

    wxTextFile fp(m_File->m_Paths[0]->c_str());
    m_Group.m_Inst->SaveExtra(fp);

    CIwTheFileModule->AddToSaveLog(m_File->m_Paths[0]->c_str());
}

//--------------------------------------------------------------------------------
CIwAttrMember* CIwASDFileDataAttr::Group::TryGetMember(const wxString& name,CIwAttrInstance* inst)
{
    int i;
    for (i=0; i<(int)inst->m_Class->m_Members.size(); i++)
    {
        if ((inst->m_Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_RESOURCE)
            break;
    }
    if (i==(int)inst->m_Class->m_Members.size())
        return NULL;

    wxString name2=MakeAbsolute(name);
    if (!wxFileExists(name2))
        return NULL;

    return inst->m_Class->m_Members[i];
}

//--------------------------------------------------------------------------------
wxString CIwASDFileDataAttr::Group::MakeAbsolute(const wxString& fileName)
{
    wxString name2;
    if (fileName.length()>=2 && fileName[0]=='.' && (fileName[1]=='\\' || fileName[1]=='/'))
        name2=m_Parent->m_File->m_Paths[0]->m_Parent->c_str();
    else if (m_Parent->m_File->m_Paths[0]->GetRoot()!=NULL)
        name2=m_Parent->m_File->m_Paths[0]->GetRoot()->c_str();

    if (name2.EndsWith(L"\\") || name2.EndsWith(L"/"))
        name2.RemoveLast();

    if (fileName.length()>=2 && fileName[0]=='.' && (fileName[1]=='\\' || fileName[1]=='/'))
        name2+=fileName.Mid(1);
    else
    {
        if (fileName[0]!='\\' && fileName[1]!='/')
            name2+=L"/";

        name2+=fileName;
    }

    return name2;
}

//--------------------------------------------------------------------------------
void CIwASDFileDataAttr::Group::BrowseForFile(wxString& fileName)
{
    wxFileName temp(MakeAbsolute(fileName));

    wxString root=L"";
    if (m_Parent->m_File->m_Paths[0]->GetRoot()!=NULL)
        root=m_Parent->m_File->m_Paths[0]->GetRoot()->c_str().Lower();

    root.Replace(L"\\",L"/");

    wxString prompt=L"Please choose a file to load...";
    while (true)
    {
        wxFileDialog dlg(NULL,prompt,temp.GetPath(),temp.GetFullName(),L"All files (*.*)|*.*",wxOPEN);

        if (dlg.ShowModal()!=wxID_OK)
            return;

        wxString name=dlg.GetPath().Lower();
        if (name.StartsWith(root))
        {
            fileName=name.Mid(root.size());
            return;
        }

        prompt=L"Please choose a file under the current project directory...";
    }
}

//--------------------------------------------------------------------------------
void CIwASDFileDataAttr::Load(bool force)
{
    if (!force && m_Group.m_Inst!=NULL)
        return;

    m_File->m_File.SetupFileAttr(m_File->m_Paths[0]->c_str());

    bool readOnly=true;
    if (m_File->m_File.isReadOnly)
        readOnly=false;

    m_Group.m_Inst=CIwTheFileModule->GetAttrInstance(m_File->m_Paths[0]->c_str(),&m_Group);

    if (!readOnly)
        SetState(STATE_LOADED|STATE_READWRITE,STATE_LOADED|STATE_READWRITE);
    else
        SetState(STATE_LOADED,STATE_LOADED|STATE_READWRITE);
}

//--------------------------------------------------------------------------------
// CIwASDAttrLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwASDAttrLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwASDAttrPanel(parent,this);

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);
}

//------------------------------------------------------------------------------
void CIwASDAttrLayout::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Data==NULL) return;

    if (m_Panel->GetText())
        dataList.push_back(m_Data);
}

//--------------------------------------------------------------------------------
void CIwASDAttrLayout::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    if (data==m_Data->m_File && base && Delete)
        m_Data=NULL;

    m_NeedReset=true;
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
void CIwASDAttrLayout::SetData(CIwASDData* data)
{
    if (!data->HasType(FILETYPE_FILE)) return;

    CIwASDFile* root=(CIwASDFile*)data;
    if (root->m_Paths.empty()) return;

    bool source=!CIwTheFileModule->CheckAttrFileType(root->m_Paths[0]->c_str());

    SetData(root,source);

    Reset();
}

//------------------------------------------------------------------------------
void CIwASDAttrLayout::SetData(CIwASDFile* root,bool source)
{
    if (source)
    {
        m_Data=root->LoadFileData(FILEDATATYPE_SOURCE);
        if (m_Data==NULL)
        {
            root->AddFileData(FILEDATATYPE_SOURCE,new CIwASDFileDataSource);
            m_Data=root->LoadFileData(FILEDATATYPE_SOURCE);
        }

        m_DataSource=(CIwASDFileDataSource*)m_Data;
        m_DataAttr=NULL;
    }
    else
    {
        m_Data=root->LoadFileData(FILEDATATYPE_ATTR);
        if (m_Data==NULL)
        {
            root->AddFileData(FILEDATATYPE_ATTR,new CIwASDFileDataAttr);
            m_Data=root->LoadFileData(FILEDATATYPE_ATTR);
        }

        m_DataSource=NULL;
        m_DataAttr=(CIwASDFileDataAttr*)m_Data;
    }

    int i;
    for (i=0; i<(int)m_Data->m_Panels.size(); i++)
    {
        if (m_Data->m_Panels[i]==this)
            break;
    }
    if (i==(int)m_Data->m_Panels.size())
    {
        m_Data->m_Panels.push_back(this);
        m_Datas.push_back(m_Data);
    }
}

//--------------------------------------------------------------------------------
void CIwASDAttrLayout::Reset()
{
    if (m_Data==NULL)
        return;

    m_Panel->m_ReLoaded=m_Data->HasChanged();
    m_Panel->Reset();
    m_Data->SetChanged(m_Panel->m_ReLoaded);
    m_Data->UpdateTitle();

    m_NeedReset=false;
}

//--------------------------------------------------------------------------------
CIwASDAttrLayout::~CIwASDAttrLayout()
{
}

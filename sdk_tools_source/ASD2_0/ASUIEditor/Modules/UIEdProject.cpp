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
class CIwUIEdProjectCreate : public CIwStyleDialog
{
    class Traverser : public wxDirTraverser
    {
    public:
        wxArrayString& list;

        Traverser(wxArrayString& _list) : list(_list) {}
        virtual wxDirTraverseResult OnFile(const wxString& filename) { return wxDIR_CONTINUE; }
        virtual wxDirTraverseResult OnDir(const wxString& dirname)
        {
            wxFileName name(dirname);
            list.Add(name.GetName());
            return wxDIR_IGNORE;
        }
    };
    enum { CTRLID_BROWSEDIR,CTRLID_BROWSEUIP,CTRLID_BROWSEGROUP,CTRLID_BROWSEGROUP2,CTRLID_BROWSEPALETTE,CTRLID_BROWSEUI };
public:
    wxTextCtrl* m_MainDir;
    wxTextCtrl* m_GroupFile;
    wxTextCtrl* m_PaletteFile;
    wxTextCtrl* m_UIFile;
    wxComboBox* m_StyleList;
    CUIEdProject& m_Project;
    bool m_FromGroup;
    wxString m_Dir;
public:
    CIwUIEdProjectCreate(wxWindow* parent,CUIEdProject& project,bool fromGroup) : CIwStyleDialog(parent,L"Create a new Project"),
        m_Project(project),m_FromGroup(fromGroup)
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        wxFileName name(CIwTheApp->argv[0]);

        m_Dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}/../styles/");

        wxArrayString files;
        Traverser traverser(files);
        wxDir dir(m_Dir);
        dir.Traverse(traverser);

        CIwStyleCtrlGroup* group=new CIwStyleCtrlGroup(true,true);
        sizer->Add(group);

        m_MainDir=new wxTextCtrl(this,wxID_ANY,L"",wxPoint(-1,-1),wxSize(300,-1));
        group->Add(m_MainDir,L"Project Data Directory:");
        group->Add(new CIwStyleButton(this,CTRLID_BROWSEDIR,L"Browse..."));
        m_MainDir->SetToolTip(L"Project root data directory");

        m_GroupFile=new wxTextCtrl(this,wxID_ANY,L"UI.group",wxPoint(-1,-1),wxSize(300,-1));
        group->Add(m_GroupFile,L"Group File:");
        if (!fromGroup)
            group->Add(new CIwStyleButton(this,CTRLID_BROWSEGROUP,L"Browse..."));
        else
            group->Add(new CIwStyleButton(this,CTRLID_BROWSEGROUP2,L"Browse..."));

        m_GroupFile->SetToolTip(L"Base Group file, must be under project root directory, project file is named this too");

        if (!fromGroup && files.size()>0)
        {
            m_StyleList=new wxComboBox(this,wxID_ANY,files[0],wxPoint(-1,-1),wxSize(300,-1),files,wxCB_READONLY);
            group->Add(m_StyleList,L"Select House Style:");
            m_StyleList->SetToolTip(L"Which set of Properties and Resources should be copied into the project");
            m_PaletteFile=NULL;

            m_UIFile=new wxTextCtrl(this,wxID_ANY,L"",wxPoint(-1,-1),wxSize(300,-1));
            group->Add(m_UIFile,L"UI File:");
            group->Add(new CIwStyleButton(this,CTRLID_BROWSEUI,L"Browse..."));
            m_UIFile->SetToolTip(L"Optional Initial UI file");
        }
        else
        {
            m_StyleList=NULL;
            m_UIFile=NULL;

            m_PaletteFile=new wxTextCtrl(this,wxID_ANY,L"",wxPoint(-1,-1),wxSize(300,-1));
            group->Add(m_PaletteFile,L"Palette File:");
            group->Add(new CIwStyleButton(this,CTRLID_BROWSEPALETTE,L"Browse..."));
            m_PaletteFile->SetToolTip(L"Optional Palette file");
        }

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"OK"),CIwStyleButtonBar::SPACE_PROP);
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"));
        sizer->Add(bar,0,wxEXPAND);

        sizer->Fit(this);
    }
    void OnBrowseDir(wxCommandEvent& e)
    {
        wxString dir=m_MainDir->GetValue();
        if (dir.empty())
            dir=m_Project.m_RootDir;

        wxDirDialog dlg(this,L"Please set the Root Data Directory...",dir);

        if (dlg.ShowModal()==wxID_OK)
            m_MainDir->SetValue(dlg.GetPath());
    }
    void OnBrowseGroup(wxCommandEvent&)
    {
        if (!m_MainDir->GetValue().EndsWith(L"/"))
            m_MainDir->SetValue(m_MainDir->GetValue()+L"/");

        wxFileName name(m_MainDir->GetValue()+m_GroupFile->GetValue());
        if (name.GetPath().empty())
            name.SetPath(m_MainDir->GetValue());

        if (name.GetName().empty())
            name.SetName(L"UI.group");

        wxFileDialog dlg(this,L"Where to Save Group File to...",name.GetPath(),name.GetFullName(),
                         L"Group File (*.group)|*.group|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);

        if (dlg.ShowModal()!=wxID_OK) return;

        wxString mainDir=m_MainDir->GetValue();
        wxString file=dlg.GetPath();

        if (!CIwTheHost.StartsWith(file,mainDir))
            wxMessageBox(L"Group File must be under the Root Directory",L"Set Group File");
        else
            m_GroupFile->SetValue(dlg.GetPath().Mid(mainDir.size()));
    }
    void OnBrowseGroup2(wxCommandEvent&)
    {
        if (!m_MainDir->GetValue().EndsWith(L"/"))
            m_MainDir->SetValue(m_MainDir->GetValue()+L"/");

        wxFileName name(m_MainDir->GetValue()+m_GroupFile->GetValue());
        if (name.GetName().empty())
            name.SetName(L"*.group");

        wxFileDialog dlg(this,L"Where to Load Group File from...",name.GetPath(),name.GetFullName(),
                         L"Group File (*.group)|*.group|All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);

        if (dlg.ShowModal()!=wxID_OK) return;

        wxString mainDir=m_MainDir->GetValue();
        wxString file=dlg.GetPath();

        if (!CIwTheHost.StartsWith(file,mainDir))
            wxMessageBox(L"Group File must be under the Root Directory",L"Set Group File");
        else
            m_GroupFile->SetValue(dlg.GetPath().Mid(mainDir.size()));
    }
    void OnBrowsePalette(wxCommandEvent&)
    {
        if (!m_MainDir->GetValue().EndsWith(L"/"))
            m_MainDir->SetValue(m_MainDir->GetValue()+L"/");

        wxFileName name(m_MainDir->GetValue()+m_PaletteFile->GetValue());
        if (name.GetPath().empty())
            name.SetPath(m_MainDir->GetValue());

        if (name.GetName().empty())
            name.SetName(L"expmetapalette.ui");

        wxFileDialog dlg(this,L"Where to get Palette File from...",name.GetPath(),name.GetFullName(),
                         L"Palette File (*.ui)|*.ui|All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);

        if (dlg.ShowModal()!=wxID_OK) return;

        wxString mainDir=m_MainDir->GetValue();
        wxString file=dlg.GetPath();

        if (!CIwTheHost.StartsWith(file,mainDir))
            wxMessageBox(L"Palette File must be under the Root Directory",L"Set Palette File");
        else
            m_PaletteFile->SetValue(dlg.GetPath().Mid(mainDir.size()));
    }
    void OnBrowseUI(wxCommandEvent&)
    {
        if (!m_MainDir->GetValue().EndsWith(L"/"))
            m_MainDir->SetValue(m_MainDir->GetValue()+L"/");

        wxFileName name(m_MainDir->GetValue()+m_UIFile->GetValue());
        if (name.GetPath().empty())
            name.SetPath(m_MainDir->GetValue());

        if (name.GetName().empty())
            name.SetName(L"main.ui");

        wxFileDialog dlg(this,L"Location of initial UI file...",name.GetPath(),name.GetFullName(),
                         L"UI File (*.ui)|*.ui|All Files (*.*)|*.*",wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (dlg.ShowModal()!=wxID_OK) return;

        wxString mainDir=m_MainDir->GetValue();
        wxString file=dlg.GetPath();

        if (!CIwTheHost.StartsWith(file,mainDir))
            wxMessageBox(L"UI File must be under the Root Directory",L"Make UI File");
        else
            m_UIFile->SetValue(dlg.GetPath().Mid(mainDir.size()));
    }
    void OnOK(wxCommandEvent&)
    {
        if (!wxDirExists(m_MainDir->GetValue()))
        {
            wxMessageBox(L"Please select a valid Base Data Directory",L"Creating new Project...",wxOK|wxCENTRE|wxICON_ERROR);
            return;
        }

        if (!wxFileExists(m_MainDir->GetValue()+m_GroupFile->GetValue()) && m_FromGroup)
        {
            wxMessageBox(L"Please select a valid Group file",L"Creating new Project...",wxOK|wxCENTRE|wxICON_ERROR);
            return;
        }

        if (!m_MainDir->GetValue().EndsWith(L"/"))
            m_MainDir->SetValue(m_MainDir->GetValue()+L"/");

        if (m_GroupFile->GetValue().empty())
            m_GroupFile->SetValue(L"UI.group");

        EndDialog(wxID_OK);
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CIwUIEdProjectCreate, CIwStyleDialog)
    EVT_BUTTON(CTRLID_BROWSEDIR,CIwUIEdProjectCreate::OnBrowseDir)
    EVT_BUTTON(CTRLID_BROWSEGROUP,CIwUIEdProjectCreate::OnBrowseGroup)
    EVT_BUTTON(CTRLID_BROWSEGROUP2,CIwUIEdProjectCreate::OnBrowseGroup2)
    EVT_BUTTON(CTRLID_BROWSEPALETTE,CIwUIEdProjectCreate::OnBrowsePalette)
    EVT_BUTTON(CTRLID_BROWSEUI,CIwUIEdProjectCreate::OnBrowseUI)
    EVT_BUTTON(wxID_OK,CIwUIEdProjectCreate::OnOK)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwUIEdProjectFrame, wxTreeCtrl)
    EVT_TREE_ITEM_ACTIVATED(CTRLID_TREE,CIwUIEdProjectFrame::OnDblClick)
    EVT_RIGHT_DOWN(CIwUIEdProjectFrame::OnMenu)
    EVT_MENU(CTRLID_ADD,CIwUIEdProjectFrame::OnAdd)
    EVT_MENU(CTRLID_MAKEGROUP,CIwUIEdProjectFrame::OnMakeGroup)
    EVT_MENU(CTRLID_MAKEUI,CIwUIEdProjectFrame::OnMakeUI)
    EVT_MENU(CTRLID_MAKEITX,CIwUIEdProjectFrame::OnMakeITX)
    EVT_MENU(CTRLID_MAKEMAT,CIwUIEdProjectFrame::OnMakeMat)
    EVT_MENU(CTRLID_REMOVE,CIwUIEdProjectFrame::OnRemove)
    EVT_MENU(CTRLID_SETACTIVEITX,CIwUIEdProjectFrame::OnActiveITX)
    EVT_MENU(CTRLID_SETACTIVEMAT,CIwUIEdProjectFrame::OnActiveMat)
    EVT_MENU(CTRLID_VIEWSOURCE,CIwUIEdProjectFrame::OnViewSource)
    EVT_IDLE(CIwUIEdProjectFrame::OnIdle)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnIdle(wxIdleEvent&)
{
    m_Shared->Idle();
}
//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnViewSource(wxCommandEvent&)
{
    if (currData==NULL)
        return;

    if (currData->m_Source==NULL)
        currData->m_Source=new CIwUIEdFileDataSource(currData->m_FileName);

    currData->m_Source->Load(true);

    m_Shared->m_UIEdSourcePanel->SetData(currData);
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnAdd(wxCommandEvent&)
{
    CUIEdProjectGroup* group=m_Shared->m_Project.m_GroupData;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    wxFileDialog dlg(this,L"Add File...",m_Shared->m_Project.m_RootDir,L"*.*",L"All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Shared->m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Add File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
            group->AddLine(L"\t\""+line+L"\"");
    }

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMakeGroup(wxCommandEvent&)
{
    CUIEdProjectGroup* group=m_Shared->m_Project.m_GroupData;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    wxFileDialog dlg(this,L"Make Group File...",m_Shared->m_Project.m_RootDir,L"*.group",
                     L"Group File (*.group)|*.group|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Shared->m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Make Group File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
        {
            m_Shared->m_Project.CreateGroup(line);
            group->AddLine(L"\t\""+line+L"\"");
        }
        else
            wxMessageBox(L"File already exists",L"Make Group File");
    }

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMakeUI(wxCommandEvent&)
{
    CUIEdProjectGroup* group=NULL;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    m_Shared->MakeUI(group);
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMakeITX(wxCommandEvent&)
{
    CUIEdProjectGroup* group=m_Shared->m_Project.m_GroupData;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    wxFileDialog dlg(this,L"Make Property Set File...",m_Shared->m_Project.m_RootDir,L"*.itx",
                     L"Property Set File (*.itx)|*.itx|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Shared->m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Make Property Set File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
        {
            m_Shared->m_Project.CreateUI(line);

            if (group->AddLine(L"\t\""+line+L"\""))
            {
                CUIEdProjectGroupLine* line=group->m_Lines.back();
                if (line->m_UI!=NULL)
                    m_Shared->m_Project.m_PropSetData=line->m_UI;
            }
        }
        else
            wxMessageBox(L"File already exists",L"Make Property Set File");
    }

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMakeStyleITX(wxCommandEvent&)
{
    CUIEdProjectGroup* group=m_Shared->m_Project.m_GroupData;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    wxFileDialog dlg(this,L"Make Style Sheet File...",m_Shared->m_Project.m_RootDir,L"*.itx",
                     L"Style Sheet File (*.itx)|*.itx|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Shared->m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Make Style Sheet File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
        {
            m_Shared->m_Project.CreateStyleSheet(line,L""); //TODO

            if (group->AddLine(L"\t\""+line+L"\""))
            {
                CUIEdProjectGroupLine* line=group->m_Lines.back();
                if (line->m_UI!=NULL)
                    m_Shared->m_Project.m_PropSetData=line->m_UI;
            }
        }
        else
            wxMessageBox(L"File already exists",L"Make Style Sheet File");
    }

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnActiveITX(wxCommandEvent&)
{
    if (currData==NULL || currData->m_UI==NULL) return;

    m_Shared->m_Project.m_PropSetData=currData->m_UI;
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnActiveMat(wxCommandEvent&)
{
    if (currData==NULL || currData->m_UI==NULL) return;

    m_Shared->m_Project.m_MaterialData=currData->m_UI;
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMakeMat(wxCommandEvent&)
{
    CUIEdProjectGroup* group=m_Shared->m_Project.m_GroupData;
    if (currData!=NULL && currData->m_Group!=NULL)
        group=currData->m_Group;

    wxFileDialog dlg(this,L"Make Material File...",m_Shared->m_Project.m_RootDir,L"*.mtl",
                     L"Material File (*.mtl)|*.mtl|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Shared->m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Make Material File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
        {
            m_Shared->m_Project.CreateUI(line);

            if (group->AddLine(L"\t\""+line+L"\""))
            {
                CUIEdProjectGroupLine* line=group->m_Lines.back();
                if (line->m_UI!=NULL)
                    m_Shared->m_Project.m_MaterialData=line->m_UI;
            }
        }
        else
            wxMessageBox(L"File already exists",L"Make Material File");
    }

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnRemove(wxCommandEvent&)
{
    for (int i=0; i<(int)currData->m_Parent->m_Lines.size(); i++)
    {
        if (currData->m_Parent->m_Lines[i]==currData)
        {
            if (currData->m_UI!=NULL)
                m_Shared->RemovePropSets(currData->m_UI);

            currData->m_Parent->m_Lines.erase(currData->m_Parent->m_Lines.begin()+i);
            if (currData->m_Parent->m_Insert>=i)
                currData->m_Parent->m_Insert--;

            break;
        }
    }
    if (currData->m_UI==m_Shared->m_CurrUI)
        m_Shared->UnLoadUI();

    Reset();
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnMenu(wxMouseEvent& e)
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
        TreeData* data2=(TreeData*)GetItemData(item);
        if (data2!=NULL)
            currData=data2->m_Line;
    }

    m_PopupMenu=new wxMenu;
    m_PopupMenu->Append(CTRLID_ADD,L"&Add File...",L"Add a file into the current group",false);
    m_PopupMenu->Append(CTRLID_MAKEGROUP,L"New &Group file...",L"Add a new group file into the current group",false);
    m_PopupMenu->Append(CTRLID_MAKEUI,L"New &UI file...",L"Add a new ui file into the current group",false);
    m_PopupMenu->Append(CTRLID_MAKEMAT,L"New &Material file...",L"Add a new material file into the current group",false);
    m_PopupMenu->Append(CTRLID_MAKEITX,L"New &Property Set file...",L"Add a new property set file into the current group",false);

    if (currData!=NULL)
    {
        m_PopupMenu->AppendSeparator();
        m_PopupMenu->Append(CTRLID_REMOVE,L"&Remove",L"remove the current file",false);
        m_PopupMenu->Append(CTRLID_VIEWSOURCE,L"&View Source",L"View this file as Text",false);

        if (currData->m_UI!=NULL)
        {
            if (currData->m_UI->m_FileName.EndsWith(L".itx") && currData->m_UI!=m_Shared->m_Project.m_PropSetData)
                m_PopupMenu->Append(CTRLID_SETACTIVEITX,L"&Set as Active Property Set",L"Set this file as the active property set file",false);

            if (currData->m_UI->m_FileName.EndsWith(L".mtl") && currData->m_UI!=m_Shared->m_Project.m_MaterialData)
                m_PopupMenu->Append(CTRLID_SETACTIVEMAT,L"&Set as Active Material",L"Set this file as the active material file",false);
        }
    }

    PopupMenu(m_PopupMenu);
}

//--------------------------------------------------------------------------------
void CIwUIEdProjectFrame::OnDblClick(wxTreeEvent& e)
{
    wxTreeItemId id=e.GetItem();
    if (!id) return;

    TreeData* data=(TreeData*)GetItemData(id);
    if (data==NULL || data->m_Line==NULL || data->m_Line->m_FileName.empty()) return;

    if (!data->m_Line->m_FileName.Lower().EndsWith(L".ui"))
        return;

    m_Shared->LoadUI(data->m_Line,true);

    if (data->m_Line->m_Source==NULL)
    {
        data->m_Line->m_Source=new CIwUIEdFileDataSource(data->m_Line->m_FileName);
        data->m_Line->m_Source->Load(true);
    }

    if (m_Shared->m_UIEdSourcePanel)
        m_Shared->m_UIEdSourcePanel->SetData(data->m_Line);
}

//------------------------------------------------------------------------------
bool CIwUIEdProjectFrame::MakeProject(bool fromGroup)
{
    CIwUIEdProjectCreate dlg(this,m_Shared->m_Project,fromGroup);
    if (dlg.ShowModal()!=wxID_OK)
        return false;

    wxFileName name(dlg.m_GroupFile->GetValue());
    name.SetExt(L"uip");

    m_Shared->Clear();
    m_Shared->m_Project.Setup(dlg.m_MainDir->GetValue(),name.GetFullPath(),dlg.m_GroupFile->GetValue());

    if (dlg.m_PaletteFile!=NULL)
    {
        m_Shared->m_Project.m_Palette.m_FileName=dlg.m_MainDir->GetValue()+dlg.m_PaletteFile->GetValue();
        m_Shared->m_Project.SetChanged(true);
        if (!dlg.m_PaletteFile->GetValue().empty())
        {
            m_Shared->m_Project.m_Palette.Load();
            m_Shared->m_Project.m_Palette.SetupElements();
        }
    }

    if (dlg.m_StyleList!=NULL)
        m_Shared->m_Project.Create(fromGroup,dlg.m_Dir+dlg.m_StyleList->GetStringSelection());
    else
        m_Shared->m_Project.Create(fromGroup);

    if (dlg.m_UIFile!=NULL && !dlg.m_UIFile->GetValue().empty())
    {
        wxString line=dlg.m_UIFile->GetValue();
        line.Replace(L"\\",L"/");
        if (line.AfterLast('.')==line)
            line+=L".ui";

        m_Shared->m_Project.CreateUI(line);
        m_Shared->m_Project.m_GroupData->AddLine(L"\t\""+line+L"\"");
    }

    return true;
}

//------------------------------------------------------------------------------
void CIwUIEdTreeFrame::GetOpened(std::vector<wxString>& list,wxTreeItemId parent)
{
    if (!parent) return;

    wxTreeItemIdValue cookie;
    wxTreeItemId id=GetFirstChild(parent,cookie);
    while (id.IsOk()) {
        if (IsExpanded(id))
        {
            list.push_back(GetItemText(id));
            GetOpened(list,id);
            list.push_back(L"^");
        }

        id=GetNextChild(parent,cookie);
    }
}

//------------------------------------------------------------------------------
void CIwUIEdTreeFrame::SetOpenedAll(wxTreeItemId parent)
{
    if (!parent) return;

    wxTreeItemIdValue cookie;
    wxTreeItemId id=GetFirstChild(parent,cookie);
    while (id.IsOk()) {
        Expand(id);
        SetOpenedAll(id);

        id=GetNextChild(parent,cookie);
    }
}

//------------------------------------------------------------------------------
void CIwUIEdTreeFrame::SetOpened(std::vector<wxString>& list,int& offset,wxTreeItemId parent)
{
    if (!parent) return;

    for (; offset<(int)list.size(); offset++)
    {
        if (list[offset]==L"^")
            return;

        wxTreeItemIdValue cookie;
        wxTreeItemId id=GetFirstChild(parent,cookie);
        while (id.IsOk()) {
            if (GetItemText(id)==list[offset])
            {
                Expand(id);
                break;
            }

            id=GetNextChild(parent,cookie);
        }
    }
}

static const wxChar* iconList[]={
    L"group_file16x16.png;.group",
    L"itx_file16x16.png;.itx",
    L"mtl_file16x16.png;.mtl",
    L"ui_file16x16.png;.ui",
    L"gxf_file16x16.png;.gxfont",
    L"texture_file16x16.png;.tga;.bmp;.png;.gif",
    L"unknown_file16x16.png",
    NULL
};

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::SetupIcons()
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
int CIwUIEdProjectFrame::GetIcon(const wxString& fileName)
{
    int j,k;
    for (k=0; iconList[k]!=NULL; k++)
    {
        std::vector<wxString> args;
        Split(wxString(iconList[k]),args,L";");

        if (args.size()==1)
            return k;

        for (j=1; j<(int)args.size(); j++)
        {
            if (fileName.Lower().EndsWith(args[j]))
                break;
        }
        if (j<(int)args.size())
            return k;
    }
    return -1;
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::Add(wxTreeItemId parent,CUIEdProjectGroup* group)
{
    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (!group->m_Lines[i]->m_Name.empty())
        {
            int icon=GetIcon(group->m_Lines[i]->m_FileName);

            wxTreeItemId id=AppendItem(parent,group->m_Lines[i]->m_Name,icon,icon,new TreeData(group->m_Lines[i]));
            if (group->m_Lines[i]->m_FileName.Lower().EndsWith(L".ui"))
                SetItemBold(id);

            if (group->m_Lines[i]->m_Group!=NULL)
                Add(id,group->m_Lines[i]->m_Group);
        }
    }
}

//------------------------------------------------------------------------------
void CIwUIEdProjectFrame::Reset()
{
    std::vector<wxString> list;
    GetOpened(list,GetRootItem());

    DeleteAllItems();
    wxTreeItemId root=AddRoot(L"");
    if (m_Shared->m_Project.m_GroupData!=NULL)
    {
        wxFileName name(m_Shared->m_Project.m_GroupData->m_FileName);
        SetItemText(root,name.GetFullName());
        SetItemImage(root,GetIcon(m_Shared->m_Project.m_GroupData->m_FileName));
        Add(root,m_Shared->m_Project.m_GroupData);
        Expand(root);
    }

    int offset=0;
    SetOpened(list,offset,GetRootItem());
}

//------------------------------------------------------------------------------
wxSizerItem* CUIEdProjectPane::Create(wxWindow* parent)
{
    m_Tree=new CIwUIEdProjectFrame(parent,m_Shared);

    return new wxSizerItem(m_Tree,1,wxEXPAND|wxALL,0,NULL);
}

//------------------------------------------------------------------------------
void CUIEdProjectPane::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Shared->m_Project.HasChanged())
        dataList.push_back(&m_Shared->m_Project);
}

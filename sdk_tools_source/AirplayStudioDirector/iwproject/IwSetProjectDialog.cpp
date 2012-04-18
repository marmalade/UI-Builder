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
#include "IwSetProjectDialog.h"
#include "IwProject.h"
#include "../Iwattr/IwAttrDesc.h"
#include "../asdirector/Source/IwWinCommand.h"
#include "../IwSourceControl/IwWinSourceControl.h"
#include "../IwStyleSheet/IwStyleSheet.h"


BEGIN_EVENT_TABLE(CIwSetProjectDialog, CIwStyleDialog)
    //	EVT_COMBOBOX(CTRLID_PROJECT,CIwSetProjectDialog::OnProject)
    EVT_BUTTON(CTRLID_BROWSE,CIwSetProjectDialog::OnBrowse)
    EVT_BUTTON(CTRLID_EDIT,CIwSetProjectDialog::OnEdit)
    EVT_BUTTON(CTRLID_REMOVE,CIwSetProjectDialog::OnRemove)
    EVT_BUTTON(CTRLID_ADD,CIwSetProjectDialog::OnAdd)
    EVT_BUTTON(wxID_OK,CIwSetProjectDialog::OnDone)
    EVT_BUTTON(wxID_CANCEL,CIwSetProjectDialog::OnCancel)
    EVT_BUTTON(CTRLID_COPY,CIwSetProjectDialog::OnCopy)
    EVT_LISTBOX(CTRLID_PROJECT,CIwSetProjectDialog::OnProject)
    EVT_CHECKBOX(CTRLID_CHECKSC,CIwSetProjectDialog::OnCheckSC)
END_EVENT_TABLE()


bool CIwSetProjectDialog::DoesProjectNameAlreadyExist(const wxString& AddName)
{
    int projectNum=0;
    wxString projectName;
    // check the project for AddName already
    while ( Project.Get(wxT("project"), projectName, projectNum) )
    {
        if (projectName.IsSameAs(AddName,false))   // project name found!
            return true;

        projectNum++;
    }
    return false;
}

//-----------------------------------------------------------------------------
//project editor dialog
CIwSetProjectDialog::CIwSetProjectDialog(wxWindow* Parent, CIwWinSourceControl& SC) :
    CIwStyleDialog(Parent,wxT("Setup Project..."),wxSize(480,400)),
    m_SourceControl(SC),m_bPromptedForDelete(false),m_CancelPressed(false)
{
    wxArrayString Strings;
    wxString Data;
    int j;

    for (j=0; Project.Get(wxT("project"),Data,j); j++)
    {
        Strings.Add(Data);
    }

    wxSizer* MainV=new wxBoxSizer(wxVERTICAL);
    SetSizer(MainV);

    MainV->Add(new CIwStyleHeader(this,IwGetSDKIdent()+wxT(" Studio"),wxT("Setup "+IwGetSDKIdent()+L" Project")),0,wxEXPAND);

    wxSizer* LBoxH=new wxBoxSizer(wxHORIZONTAL);
    MainV->Add(LBoxH,1,wxEXPAND);

    CIwStyleCtrlGroup* LHGroup=new CIwStyleCtrlGroup(false,false,true);
    LBoxH->Add(LHGroup,0,wxEXPAND);

    CIwStyleCtrlGroup* RHGroup=new CIwStyleCtrlGroup(false,false,true);
    LBoxH->Add(RHGroup,0,wxEXPAND);

    CIwStyleCtrlGroup* BotGroup=new CIwStyleCtrlGroup(false,true);
    MainV->Add(BotGroup,0,wxEXPAND);

    CIwStyleButtonBar* Bar=new CIwStyleButtonBar(this);
    MainV->Add(Bar,0,wxEXPAND);

    //m_Export=new CIwStyleButtonOD(this,CTRLID_EXPORT,"Export!",wxSize(3,1));
    //m_Bar->Add(m_Export);

    // LHGroup - project & scroll list
    m_Project = new wxListBox( this, CTRLID_PROJECT, wxDefaultPosition, wxSize(150,120), Strings, wxLB_SINGLE | wxLB_NEEDED_SB );

    if (Project.Get(wxT("currentproject"),Data))
    {
        int idx = m_Project->FindString(Data);
        m_Project->SetSelection(idx);
        m_LastProject=m_Project->GetStringSelection();
        m_FirstProject=m_LastProject;
    }

    LHGroup->Add(m_Project,wxT("Projects:"));
    m_Project->Refresh();

    // RHGroup - Settings and multiline text control
    m_SettingsList = new wxTextCtrl( this, CTRLID_SETTINGS, wxEmptyString, wxDefaultPosition, wxSize(300,120), wxTE_MULTILINE | wxTE_DONTWRAP | wxTE_READONLY | wxTE_RICH );

    //m_SettingsList->Enable(false);

    RHGroup->Add(m_SettingsList,wxT("Settings:"));

    // read the settings file for the settings list
    if (Project.Read(Data))
    {
        Data.clear();
        Project.GetFile(wxT("data"),Data);
        if (!Data.empty())
            if ((Data[Data.size()-1]==wxT('\\')) || (Data[Data.size()-1]==wxT('/')))
                Data.RemoveLast();

        Project.WriteSettingsToControl( m_SettingsList, PROJECT_LOCAL|PROJECT_PROJECT);
    }

    wxBoxSizer* checkSizer=new wxBoxSizer(wxHORIZONTAL);
    LHGroup->Add(checkSizer,L"",this);

    SCCheck=new CIwStyleCheckOD(this,CTRLID_CHECKSC,wxT("Use Source Control?"));
    checkSizer->Add(SCCheck,1,wxEXPAND);

    checkSizer=new wxBoxSizer(wxHORIZONTAL);
    RHGroup->Add(checkSizer,L"",this);

    BKCheck=new CIwStyleCheckOD(this,wxID_ANY,wxT("Make Backup files?"));
    checkSizer->Add(BKCheck,1,wxEXPAND);
    BKCheck->Enable(!m_SourceControl.m_Active);
    BKCheck->SetValue(m_SourceControl.m_Backup);
    CLCheck=new CIwStyleCheckOD(this,wxID_ANY,wxT("Make Change List?"));
    checkSizer->Add(CLCheck,1,wxEXPAND);
    CLCheck->Enable(m_SourceControl.m_Active);
    CLCheck->SetValue(m_SourceControl.m_ChangeList);

    // browse button and uneditable text box ?
    // data will still contain path or NULL
    m_Dir=new wxTextCtrl(this,wxID_ANY,Data,wxPoint(-1,-1),wxSize(350,-1),wxTE_RICH);
    BotGroup->Add(m_Dir,wxT("Directory:"));
    BotGroup->Add(new CIwStyleButtonOD(this,CTRLID_BROWSE,wxT("Browse...")));

    Strings.Clear();
    Strings.Add(wxT("<None>"));

    m_ResBuildStyleList=new wxComboBox(this,wxID_ANY,wxT("<None>"),wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY);
    BotGroup->Add(m_ResBuildStyleList,wxT("Use ResBuildStyle:"));
    ResetResBuildStyle();

    // Add Delete Cancel Ok
    Bar->Add(new CIwStyleButtonOD(this,CTRLID_ADD,wxT("Add...")));
    Bar->Add(new CIwStyleButtonOD(this,CTRLID_EDIT,wxT("Edit Name...")));
    Bar->Add(new CIwStyleButtonOD(this,CTRLID_COPY,wxT("Move...")));
    Bar->Add(new CIwStyleButtonOD(this,CTRLID_REMOVE,wxT("Remove")),CIwStyleButtonBar::SPACE_PROP);
    Bar->Add(new CIwStyleButtonOD(this,wxID_OK,wxT("OK")));
    Bar->Add(new CIwStyleButtonOD(this,wxID_CANCEL,wxT("Cancel")));

    SetBackgroundColour(SCCheck->GetBackgroundColour());
    m_SettingsList->SetBackgroundColour(SCCheck->GetBackgroundColour());

    MainV->Layout();
    MainV->Fit(this);
}

//-----------------------------------------------------------------------------
void CIwSetProjectDialog::ResetResBuildStyle()
{
    wxString Data;
    wxString Style=wxT("<None>");

    m_ResBuildStyleList->Clear();
    m_ResBuildStyleList->Append(Style);

    Project.Get(wxT("ResBuildStyle"),Style);

    if (Project.GetFile(wxT("data"),Data) && !Data.empty())
    {
        wxString FileName=wxString::Format(wxT("%s/resbuildstyles.itx"),Data.c_str());

        wxTextFile fp(FileName);
        if (fp.Exists())
        {
            fp.Open();
            for (int i=0; i<(int)fp.GetLineCount(); i++)
            {
                std::vector<wxString> argv;
                if (SuperSplit(fp[i],argv,wxT(" \t"))<2) continue;

                if (argv[0].IsSameAs(wxT("name"),false))
                    m_ResBuildStyleList->Append(argv[1]);
            }
        }
    }

    m_ResBuildStyleList->SetValue(Style);

    if (Project.Get(wxT("useSC"),Data))
        m_SourceControl.m_Active=Data.IsSameAs(L"true",false);

    if (m_SourceControl.m_User.empty())
        SCCheck->Enable(false);
    else
    {
        SCCheck->Enable(true);
        SCCheck->SetValue(m_SourceControl.m_Active);
    }

    if (Project.Get(wxT("useBackup"),Data))
        m_SourceControl.m_Backup=Data.IsSameAs(L"true",false);

    if (Project.Get(wxT("useChangeList"),Data))
        m_SourceControl.m_ChangeList=Data.IsSameAs(L"true",false);

    if (m_SourceControl.m_Active)
    {
        BKCheck->Enable(false);
        CLCheck->Enable(true);
    }
    else
    {
        BKCheck->Enable(true);
        CLCheck->Enable(false);
    }
}

//-----------------------------------------------------------------------------
void CIwSetProjectDialog::OnCopy(wxCommandEvent&)
{
    wxDirDialog dlg(this,L"Choose Destination Directory");
    if (dlg.ShowModal()!=wxID_OK)
        return;

    wxString Data=m_Dir->GetValue();
    wxString Data2=dlg.GetPath();

    wxArrayString Out,Err;
    wxExecute(wxString::Format(L"cp -pR \"%s/\" \"%s\"",Data.c_str(),Data2.c_str()),Out,Err);

    char Line[256];
    for (int i=0; i<(int)Err.Count(); i++)
    {
        strcpy(Line,Err[i].mb_str());
    }

    m_Dir->SetValue(Data2);
}

//-----------------------------------------------------------------------------
void CIwSetProjectDialog::OnCheckSC(wxCommandEvent&)
{
    if (SCCheck->GetValue())
    {
        BKCheck->Enable(false);
        CLCheck->Enable(true);
    }
    else
    {
        BKCheck->Enable(true);
        CLCheck->Enable(false);
    }
}

//-----------------------------------------------------------------------------
void CIwSetProjectDialog::OnProject(wxCommandEvent&)
{
    wxString Data;

    if (wxDirExists(m_Dir->GetValue()))
    {
        if ( Project.Has(wxT("data") ) )
            Project.Set(wxT("data"), m_Dir->GetValue());
        else
            Project.Add(wxT("data"),m_Dir->GetValue(), true,false);

        m_SourceControl.m_Active=SCCheck->GetValue();
        Project.SetOrAdd(wxT("useSC"),SCCheck->GetValue() ? wxT("true") : wxT("false"),true,false);


        m_SourceControl.m_ChangeList=m_SourceControl.m_Active ? CLCheck->GetValue() : false;
        m_SourceControl.m_Backup=m_SourceControl.m_Active ? false : BKCheck->GetValue();

        Project.SetOrAdd(wxT("useBackup"),BKCheck->GetValue() ? wxT("true") : wxT("false"),true,false);
        Project.SetOrAdd(wxT("useChangeList"),CLCheck->GetValue() ? wxT("true") : wxT("false"),true,false);

        Project.SetOrAdd(wxT("ResBuildStyle"),m_ResBuildStyleList->GetValue().c_str(),true,false);

        Project.SaveLocal(m_LastProject);
    }

    // reset the other controls
    m_Dir->Clear();
    m_SettingsList->Clear();

    // load that project
    Project.Read(m_Project->GetStringSelection());
    m_LastProject=m_Project->GetStringSelection();

    // check for a data directory
    if (Project.GetFile(wxT("data"),Data))
    {
        if ((Data[Data.size()-1]==wxT('\\')) || (Data[Data.size()-1]==wxT('/')))
            Data.RemoveLast();

        m_Dir->SetValue( Data );
    }
    else
    {
        // bring up the browse for path prompt
        wxCommandEvent blank;
        OnBrowse(blank);
    }

    ResetResBuildStyle();

    Project.WriteSettingsToControl( m_SettingsList, PROJECT_LOCAL|PROJECT_PROJECT);

}
//-----------------------------------------------------------------------------
//on add project
void CIwSetProjectDialog::OnAdd(wxCommandEvent& e)
{
    wxTextEntryDialog Dlg(this,wxT("Please name the new project:"));
    if (Dlg.ShowModal()==wxID_OK)
    {
        if (!Dlg.GetValue().empty())
        {
            if ( DoesProjectNameAlreadyExist(Dlg.GetValue()) )
            {
                wxMessageDialog msgDlg(this, wxT("A project of that name already exists, project will not be added"),wxT("Warning"),wxOK );
                msgDlg.ShowModal();
            }
            else
            {
                m_Project->Append(Dlg.GetValue());
                // update the controls
                m_Dir->Clear();
                m_Project->SetSelection(m_Project->GetCount()-1);
                m_SettingsList->Clear();
                Project.WriteSettingsToControl( m_SettingsList, PROJECT_LOCAL|PROJECT_PROJECT);

                // update the status in the project files
                // check for this file already existing
                Project.Add(wxT("project"),Dlg.GetValue(),false,false);

                if ( !Project.Has(wxT("currentproject") ) )
                    Project.Add(wxT("currentproject"),Dlg.GetValue(),false,false);
                else
                    Project.Set( wxT("currentproject"), Dlg.GetValue());

                // Project.Save(false);	// globallocal
                // Project.Save(true);	// <project>Local
                OnProject(e);           // the selection has changed,
            }
        }
    }
}

//-----------------------------------------------------------------------------
//on edit project name
void CIwSetProjectDialog::OnEdit(wxCommandEvent&)
{
    int Num=m_Project->GetSelection();
    wxString ExistingName;
    Project.Get(wxT("currentproject"),ExistingName);
    if (m_Project->GetSelection()!=wxNOT_FOUND)
    {
        if (m_Project->GetStringSelection().IsSameAs(wxT("Select Project..."),false))
            Num=wxNOT_FOUND;
        else
            ExistingName=m_Project->GetStringSelection();
    }

    wxTextEntryDialog Dlg(this,wxT("Please rename this project:"),wxGetTextFromUserPromptStr,ExistingName);
    if (Dlg.ShowModal()==wxID_OK)
    {
        if (!Dlg.GetValue().empty() && !Dlg.GetValue().IsSameAs(ExistingName,false))
        {
            if ( DoesProjectNameAlreadyExist(Dlg.GetValue()) )
            {
                wxMessageDialog msgDlg(this, wxT("A project of that name already exists, project will not be changed"),wxT("Warning"),wxOK );
                msgDlg.ShowModal();
            }
            else
            {
                Project.ChangeProjectName(Dlg.GetValue(),ExistingName);
                // change the controls
                if (Num==wxNOT_FOUND) // create a new one or do nothing?
                {
                    m_Project->Append(Dlg.GetValue());
                    m_Project->SetSelection(m_Project->GetCount()-1);
                    Project.Add(wxT("project"),Dlg.GetValue(),false,false);
                    Project.Read(Dlg.GetValue());

                    wxString Data;
                    if (Project.GetFile(wxT("data"),Data))
                        m_Dir->SetValue(Data);
                }
                else
                {
                    m_Project->SetString(Num,Dlg.GetValue());
                    m_Project->SetSelection(Num);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//on remove project
void CIwSetProjectDialog::OnRemove(wxCommandEvent& e)
{
    if (m_Project->GetStringSelection().IsSameAs(wxT("Select Project..."),false))
        return;

    //if(!Project.Get("project",Data,1)) return;
    // Project.Get("currentproject",Data);
    // check the current selection in the projects dialog, changing the

    // Project.DeleteProject( m_Project->GetStringSelection().c_str()  );
    // int Num=m_Project->FindString(Data);
    wxString msg = wxT("Are you sure you want to delete ") + m_Project->GetStringSelection() +wxT("?");
    wxMessageDialog Dlg(this, msg,wxT("Warning"),wxYES_NO);
    if ( m_bPromptedForDelete || (Dlg.ShowModal()==wxID_YES) )
    {
        int Num = m_Project->GetSelection();
        m_Project->Delete(Num);

        if ( m_Project->GetCount() )
        {
            m_Project->Select(0);   // trying to select with 0 items causes an exception
            OnProject(e);
        }
        else
        {
            m_Dir->Clear();
            m_SettingsList->Clear();
            Project.DeleteSettingsByName(wxT("currentproject"));
        }
    }
}

//-----------------------------------------------------------------------------
//ask for project data directory
void CIwSetProjectDialog::OnBrowse(wxCommandEvent& e)
{
    wxFileName name(m_Dir->GetValue(),L"");
    name.MakeAbsolute();
    while (name.GetDirCount()>0 && !name.DirExists())
        name.RemoveLastDir();
    wxDirDialog Dlg(this,wxT("Please specify the location of the Project Data Directory"),name.GetFullPath());

    if (Dlg.ShowModal()==wxID_OK)
    {
        if ( Project.Has(wxT("data") ) )
            Project.Set(wxT("data"),Dlg.GetPath());
        else
            Project.Add(wxT("data"),Dlg.GetPath(), true,false);

        // we have not changed project, hence must not call OnProject which refers back to the files
        m_Dir->SetValue(Dlg.GetPath());

        ResetResBuildStyle();
    }
    else
    {
        if ( !Project.Has(wxT("data")) )
        {
            wxString msg = m_Project->GetStringSelection() +wxT(" must have a path associated with it\nOK to delete that project?");
            wxMessageDialog Dlg2(this, msg,wxT("Error"),wxYES_NO);
            m_bPromptedForDelete=true;
            if (Dlg2.ShowModal()==wxID_YES)
                OnRemove(e);
        }
    }

    m_bPromptedForDelete = false;
}

//--------------------------------------------------------------------------------
void CIwSetProjectDialog::OnCancel(wxCommandEvent &event)
{
    if (!m_FirstProject.empty())
    {
        m_Project->SetStringSelection(m_FirstProject);
        OnProject(event);
    }

    m_CancelPressed=true;
    OnDone(event);
    m_CancelPressed=false;
}

//--------------------------------------------------------------------------------
//finished project setup, apply values
void CIwSetProjectDialog::OnDone(wxCommandEvent &event)
{
    int i=0;
    wxString Data;
    bool HasProj=false;

    wxFileName name(m_Dir->GetValue(),L"");
    name.MakeAbsolute();

    if (!name.DirExists() && !m_CancelPressed)
    {
        wxMessageDialog Dlg(this,wxT("Directory does not exist!"),wxT("Project Setup"),wxOK|wxICON_EXCLAMATION);
        Dlg.ShowModal();
        return;
    }

    while (Project.Get(wxT("project"),Data,i++))
    {   //check to see if that name is still in the control
        if ( m_Project->FindString( Data ) == wxNOT_FOUND )
        {
            // remove the project local file
            wxString Line=Project.GetUserPath()+Data+wxT("local.txt");

            if (!wxFileName::FileExists(Line))
                Line=Project.GetPath()+Data+wxT("local.txt");

            ::wxRemoveFile(Line);
        }
        else
            HasProj=true;
    }
    // need to clear out and then re populate GlobalLocal dependent on the current contents of m_Project
    int dbgCount = Project.NumProjectSettings();
    Project.DeleteSettingsByName(wxT("currentproject"));
    Project.DeleteSettingsByName(wxT("project"));
    dbgCount = Project.NumProjectSettings();
    Project.DeleteSettingsByMode(PROJECT_LOCAL|PROJECT_PROJECT);
    dbgCount = Project.NumProjectSettings();

    if (HasProj && !Project.Has(wxT("Setup")))
        Project.Add(wxT("Setup"),wxT("Done"),false,false);

    if ( HasProj )
    {
        Project.Add(wxT("currentproject"),wxT("-temp-"),false,false);
        if (!m_CancelPressed)
        {
            Project.Set(wxT("currentproject"),m_Project->GetStringSelection().c_str()); // the callback forces the update of the tree


        }

        for (i=0; i<(int)m_Project->GetCount(); i++)
        {
            Project.Add(wxT("project"),m_Project->GetString(i).c_str(),false,false);
        }

        //	Project.Save(false);
        //	Project.Save(true);	// DFL
        Project.Read(m_Project->GetStringSelection().c_str());

        Project.SetOrAdd(wxT("data"),name.GetFullPath().c_str(),true,false);

        m_SourceControl.m_Active=SCCheck->GetValue();
        Project.SetOrAdd(wxT("useSC"),SCCheck->GetValue() ? wxT("true") : wxT("false"),true,false);

        m_SourceControl.m_ChangeList=m_SourceControl.m_Active ? CLCheck->GetValue() : false;
        m_SourceControl.m_Backup=m_SourceControl.m_Active ? false : BKCheck->GetValue();

        Project.SetOrAdd(wxT("useBackup"),BKCheck->GetValue() ? wxT("true") : wxT("false"),true,false);
        Project.SetOrAdd(wxT("useChangeList"),CLCheck->GetValue() ? wxT("true") : wxT("false"),true,false);

        Project.SetOrAdd(wxT("ResBuildStyle"),m_ResBuildStyleList->GetValue().c_str(),true,false);
    }

    // moved from top?

    // if all projects removed
    if ( m_Project->GetCount() == 0 ) // redirect to <director_path>\data\to_tempdata
    {
        wxString directorPath=Project.GetPath()+wxT("to_tempdata");

        Project.AddTemp(wxT("currentproject"),wxT("-temp-"),false);
        Project.Set(wxT("currentproject"),wxT("to_tempdata"));
        Project.AddTemp(wxT("project"),wxT("to_tempdata"),false);
        Project.AddTemp(wxT("data"), directorPath, false);
    }

    dbgCount = Project.NumProjectSettings();

    Project.Save(false);
    Project.Save(true);
    EndModal(m_CancelPressed ? wxID_CANCEL : wxID_OK);  //finish window
}

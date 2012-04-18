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
// CIwUIEdSourcePanel
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwUIEdSourcePanel, wxPanel)
    EVT_STC_SAVEPOINTLEFT(CTRLID_SOURCE,CIwUIEdSourcePanel::OnSaveOff)
    EVT_STC_SAVEPOINTREACHED(CTRLID_SOURCE,CIwUIEdSourcePanel::OnSaveOn)
    EVT_BUTTON(CTRLID_SAVE,CIwUIEdSourcePanel::OnSave)
    EVT_BUTTON(CTRLID_RELOAD,CIwUIEdSourcePanel::OnReload)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::SetData(CUIEdProjectGroupLine* line)
{
    m_Line=line;
    m_Data=m_Line->m_Source;
    Reset();
}

//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::OnSaveOn(wxStyledTextEvent& e)
{
    if (m_ReLoaded || m_Data==NULL) return;

    m_Data->SetChanged(false);
    m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::OnSaveOff(wxStyledTextEvent& e)
{
    if (m_Data==NULL) return;

    m_Data->SetChanged(true);
    m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::OnSave(wxCommandEvent&)
{
    if (!GetText()) return;

    m_Data->Save();
    if (m_Line->m_UI!=NULL)
    {
        m_Line->m_UI->Load();
        m_Line->m_UI->SetupElements();
        m_Shared->LoadUI(m_Line, true);
    }

    m_Text->SetSavePoint();
    m_Data->SetChanged(false);
    m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
bool CIwUIEdSourcePanel::GetText()
{
    if (m_Data==NULL) return false;

    if (!m_Data->HasChanged()) return false;  //if(m_Data->GetState(CIwASDData::STATE_READWRITE)==0) return false;

    m_Data->m_Buffer=m_Text->GetText();
    return true;
}

//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::OnReload(wxCommandEvent&)
{
    if (m_Data==NULL) return;

    m_Data->Load(true);

    m_ReLoaded=false;
    m_Text->SetReadOnly(false);
    m_Text->SetText(m_Data->m_Buffer);
    //m_Text->SetReadOnly(m_Data->GetState(CIwASDData::STATE_READWRITE)==0);
    m_Text->EmptyUndoBuffer();

    m_Data->SetChanged(false);
    m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
CIwUIEdSourcePanel::CIwUIEdSourcePanel(wxWindow* parent,CUIEdAttrShared* shared) : wxPanel(parent),m_Shared(shared),m_Data(NULL),m_Line(NULL)
{
    m_Shared->m_UIEdSourcePanel=this;
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    CIwStyleButtonBar* bar=new CIwStyleButtonBar(this,false);
    m_Sizer->Add(bar,0,wxEXPAND);

    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/32x32/");

    wxBitmapButton* button=new wxBitmapButton(this,CTRLID_SAVE,wxBitmap(dir+L"saveui32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Save File");
    bar->Add(button);

    button=new wxBitmapButton(this,CTRLID_RELOAD,wxBitmap(dir+L"restore32x32.png",wxBITMAP_TYPE_PNG),wxPoint(-1,-1),wxSize(36,36));
    button->SetToolTip(L"Reload File");
    bar->Add(button);

    m_Text=new wxStyledTextCtrl(this,CTRLID_SOURCE);
    //m_Text=new wxTextCtrl(this,CTRLID_SOURCETEXT,"",wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH2);
    wxFont Font(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"courier");
    //m_Text->SetFont(Font);
    //m_Text->SetReadOnly(true);
    m_Text->SetWrapMode(wxSTC_WRAP_NONE);
    m_Text->SetTabWidth(4);
    m_Text->StyleSetFont(wxSTC_STYLE_DEFAULT,Font);
    //m_Text->StyleSetBackground(wxSTC_STYLE_DEFAULT,wxColour(100,50,200));

    m_Sizer->Add(m_Text,1,wxALL|wxEXPAND,0);

    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
CIwUIEdSourcePanel::~CIwUIEdSourcePanel()
{
}

//--------------------------------------------------------------------------------
// CIwUIEdSourceLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwUIEdSourceLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwUIEdSourcePanel(parent,m_Shared);

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,0,NULL);
}

//------------------------------------------------------------------------------
void CIwUIEdSourceLayout::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Panel->m_Data==NULL) return;

    if (m_Panel->GetText())
        dataList.push_back(m_Panel->m_Data);
}

//--------------------------------------------------------------------------------
void CIwUIEdSourcePanel::Reset()
{
    if (m_Data==NULL)
        return;

    m_ReLoaded=m_Data->HasChanged();
    m_Text->SetReadOnly(false);
    m_Text->SetText(m_Data->m_Buffer);
    //m_Text->SetReadOnly(m_Data->GetState(CIwASDData::STATE_READWRITE)==0);
    m_Text->EmptyUndoBuffer();

    m_Data->SetChanged(m_ReLoaded);
    m_Data->UpdateTitle();
}

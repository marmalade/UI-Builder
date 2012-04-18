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
BEGIN_EVENT_TABLE(CIwASDSourcePanel, wxPanel)
    EVT_STC_SAVEPOINTLEFT(CTRLID_SOURCE,CIwASDSourcePanel::OnSaveOff)
    EVT_STC_SAVEPOINTREACHED(CTRLID_SOURCE,CIwASDSourcePanel::OnSaveOn)
    EVT_BUTTON(CTRLID_SAVE,CIwASDSourcePanel::OnSave)
    EVT_BUTTON(CTRLID_RELOAD,CIwASDSourcePanel::OnReload)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwASDSourcePanel::OnSaveOn(wxStyledTextEvent& e)
{
    if (m_ReLoaded) return;

    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwASDSourcePanel::OnSaveOff(wxStyledTextEvent& e)
{
    m_Layout->m_Data->SetChanged(true);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
void CIwASDSourcePanel::OnSave(wxCommandEvent&)
{
    if (!GetText()) return;

    m_Layout->m_Data->Save();

    m_Text->SetSavePoint();
    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
bool CIwASDSourcePanel::GetText()
{
    if (!m_Layout->m_Data->HasChanged()) return false;

    if (m_Layout->m_Data==NULL) return false;

    if (m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)==0) return false;

    m_Layout->m_Data->m_Buffer=m_Text->GetText();
    return true;
}

//--------------------------------------------------------------------------------
void CIwASDSourcePanel::OnReload(wxCommandEvent&)
{
    m_Layout->m_Data->Load(true);

    m_ReLoaded=false;
    m_Text->SetReadOnly(false);
    m_Text->SetText(m_Layout->m_Data->m_Buffer);
    m_Text->SetReadOnly(m_Layout->m_Data->GetState(CIwASDData::STATE_READWRITE)==0);
    m_Text->EmptyUndoBuffer();

    m_Layout->m_Data->SetChanged(false);
    m_Layout->m_Data->UpdateTitle();
}

//--------------------------------------------------------------------------------
CIwASDSourcePanel::CIwASDSourcePanel(wxWindow* parent,CIwASDSourceLayout* layout) : wxPanel(parent),m_Layout(layout)
{
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    CIwStyleButtonBar* bar=new CIwStyleButtonBar(this,false);
    m_Sizer->Add(bar,0,wxEXPAND);

    bar->Add(new CIwStyleButton(this,CTRLID_SAVE,L"Save"));
    bar->Add(new CIwStyleButton(this,CTRLID_RELOAD,L"Reload"));

    m_Text=new wxStyledTextCtrl(this,CTRLID_SOURCE);
    //m_Text=new wxTextCtrl(this,CTRLID_SOURCETEXT,"",wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY|wxTE_RICH2);
    wxFont Font(10,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_NORMAL,false,L"courier");
    //m_Text->SetFont(Font);
    m_Text->SetReadOnly(true);
    m_Text->SetWrapMode(wxSTC_WRAP_NONE);
    m_Text->SetTabWidth(4);
    m_Text->StyleSetFont(wxSTC_STYLE_DEFAULT,Font);
    //m_Text->StyleSetBackground(wxSTC_STYLE_DEFAULT,wxColour(100,50,200));

    m_Sizer->Add(m_Text,1,wxALL|wxEXPAND,0);

    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
CIwASDSourcePanel::~CIwASDSourcePanel()
{
}

//--------------------------------------------------------------------------------
void CIwASDFileDataSource::Save()
{
    if (m_File->m_Paths.empty()) return;

    wxTextFile fp(m_File->m_Paths[0]->c_str());

    std::vector<wxString> lines;
    Split(m_Buffer,lines,L"\n\r");
    for (int i=0; i<(int)lines.size(); i++)
    {
        fp.AddLine(lines[i]);
    }
    fp.Write();

    CIwTheFileModule->AddToSaveLog(m_File->m_Paths[0]->c_str());
}

//--------------------------------------------------------------------------------
void CIwASDFileDataSource::Load(bool force)
{
    if (!force && m_Buffer.size()>0)
        return;

    bool readOnly=true,noteditable=false;
    m_Buffer=L"<Uninitialized>";

    if (m_File->m_Paths.size()<1)
        return;

    int BufferLen=0;
#ifdef I3D_OS_WINDOWS
    FILE* fp=_wfopen(m_File->m_Paths[0]->c_str().c_str(),L"rb");
#else
    FILE* fp=fopen(m_File->m_Paths[0]->c_str().mb_str(),"rb");
#endif
    if (fp==NULL) return;

    fseek(fp,0,SEEK_END);

    BufferLen=ftell(fp);
    char* Buffer=new char[BufferLen+1];
    fseek(fp,0,SEEK_SET);
    fread(Buffer,1,BufferLen,fp);
    Buffer[BufferLen]=0;

    fclose(fp);

    int i,j;
    for (i=0; i<BufferLen; i++)
    {
        if (Buffer[i]<32 && (Buffer[i]!='\t' && Buffer[i]!='\n' && Buffer[i]!='\r'))
            break;
    }

    if (i==BufferLen)
    {
        m_File->m_File.SetupFileAttr(m_File->m_Paths[0]->c_str());

        if ((m_File->m_File.isReadOnly)==0)
            readOnly=false;

        m_Buffer=wxString(Buffer,wxConvUTF8);
    }
    else if (BufferLen>0)
    {
        bool Truncate=false;
        if (BufferLen>32*16)
        {
            Truncate=true;
            BufferLen=32*16;
        }

        m_Buffer.clear();
        for (i=0; i<BufferLen+15; i+=16)
        {
            m_Buffer.append(wxString::Format(L"%08x  ",i));

            for (j=0; j<16; j++)
            {
                if (j+i<BufferLen)
                    m_Buffer.append(wxString::Format(L"%02x ",Buffer[j+i]));
                else
                    m_Buffer.append(L"   ");

                if (j==7)
                    m_Buffer.append(L" ");
            }
            m_Buffer.append(L" ");

            for (j=0; j<16 && j+i<BufferLen; j++)
            {
                if (Buffer[i+j]<32 || Buffer[i+j]>126)
                    m_Buffer.append(L".");
                else
                    m_Buffer.append(1,Buffer[i+j]);
            }
            m_Buffer.append(L"\n");
        }
        if (Truncate)
            m_Buffer.append(L"<File too long to view truncated here...>");

        noteditable=true;
    }

    delete[] Buffer;

    if (noteditable)
        SetState(STATE_LOADED|STATE_NOTEDITABLE,STATE_LOADED|STATE_READWRITE|STATE_NOTEDITABLE);
    else if (!readOnly)
        SetState(STATE_LOADED|STATE_READWRITE,STATE_LOADED|STATE_READWRITE);
    else
        SetState(STATE_LOADED,STATE_LOADED|STATE_READWRITE);
}

//--------------------------------------------------------------------------------
// CIwASDSourceLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwASDSourceLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwASDSourcePanel(parent,this);

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);
}

//------------------------------------------------------------------------------
void CIwASDSourceLayout::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (m_Data==NULL) return;

    if (m_Panel->GetText())
        dataList.push_back(m_Data);
}

//--------------------------------------------------------------------------------
void CIwASDSourceLayout::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    if (data==m_Data->m_File && base && Delete)
        m_Data=NULL;

    m_NeedReset=true;
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
void CIwASDSourceLayout::SetData(CIwASDData* data)
{
    if (!data->HasType(FILETYPE_FILE)) return;

    CIwASDFile* root=(CIwASDFile*)data;

    m_Data=(CIwASDFileDataSource*)root->LoadFileData(FILEDATATYPE_SOURCE);
    if (m_Data==NULL)
    {
        root->AddFileData(FILEDATATYPE_SOURCE,new CIwASDFileDataSource);
        m_Data=(CIwASDFileDataSource*)root->LoadFileData(FILEDATATYPE_SOURCE);
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

    Reset();
}

//--------------------------------------------------------------------------------
void CIwASDSourceLayout::Reset()
{
    if (m_Data==NULL)
        return;

    m_Panel->m_ReLoaded=m_Data->HasChanged();
    m_Panel->m_Text->SetReadOnly(false);
    m_Panel->m_Text->SetText(m_Data->m_Buffer);
    m_Panel->m_Text->SetReadOnly(m_Data->GetState(CIwASDData::STATE_READWRITE)==0);
    m_Panel->m_Text->EmptyUndoBuffer();

    m_Data->SetChanged(m_Panel->m_ReLoaded);
    m_Data->UpdateTitle();

    m_NeedReset=false;
}

//--------------------------------------------------------------------------------
CIwASDSourceLayout::~CIwASDSourceLayout()
{
}

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
// CIwASDTexturePanel
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDTexturePanel, wxPanel)
    EVT_BUTTON(CTRLID_NORMAL,CIwASDTexturePanel::OnNormal)
    EVT_BUTTON(CTRLID_ZOOMIN,CIwASDTexturePanel::OnZoomIn)
    EVT_BUTTON(CTRLID_ZOOMOUT,CIwASDTexturePanel::OnZoomOut)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CIwWinSummaryImage, wxPanel)
    EVT_PAINT(CIwWinSummaryImage::OnPaint)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
CIwASDTexturePanel::CIwASDTexturePanel(wxWindow* parent,CIwASDTextureLayout* layout) : wxPanel(parent),m_Layout(layout),m_Zoom(0)
{
    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);

    m_ToolBar=new CIwStyleButtonBar(this,false);
    m_Sizer->Add(m_ToolBar,0,wxEXPAND);

    m_ToolBar->Add(new CIwStyleButton(this,CTRLID_NORMAL,L"1:1"));
    m_ToolBar->Add(new CIwStyleButton(this,CTRLID_ZOOMIN,L"+"));
    m_ToolBar->Add(new CIwStyleButton(this,CTRLID_ZOOMOUT,L"-"));

    m_Panel=new CIwWinSummaryImage(this);
    m_Sizer->Add(m_Panel,1,wxEXPAND,4);

    //m_Text=new wxTextCtrl(this,CTRLID_SUMMARYTEXT,L"",wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY);
    //m_Sizer->Add(m_Text,1,wxEXPAND,0);

    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
CIwASDTexturePanel::~CIwASDTexturePanel()
{
}

//--------------------------------------------------------------------------------
void CIwASDTexturePanel::Update(float adjust)
{
    m_Zoom+=adjust;
    float Zoom=powf(2,m_Zoom);

    wxImage Image=m_Layout->m_Data->m_Image.Scale(m_Layout->m_Data->x*Zoom,m_Layout->m_Data->y*Zoom);
    m_Panel->m_Bitmap=new wxBitmap(Image);
    m_Panel->SetMinSize(wxSize(m_Layout->m_Data->x*Zoom,m_Layout->m_Data->y*Zoom));

    m_Panel->Refresh();
}

//--------------------------------------------------------------------------------
void CIwASDTexturePanel::OnNormal(wxCommandEvent& event)
{
    m_Zoom=0;
    Update(0);
}

//--------------------------------------------------------------------------------
void CIwASDTexturePanel::OnZoomIn(wxCommandEvent& event)
{
    Update(0.25f);
}

//--------------------------------------------------------------------------------
void CIwASDTexturePanel::OnZoomOut(wxCommandEvent& event)
{
    Update(-0.25f);
}


//--------------------------------------------------------------------------------
void CIwASDTextureFile::Load(int Stage,int pathnum)
{
    if ((depth!=0) && m_Image.Ok())
        return;

    if (m_Paths[0]->getExt().IsSameAs(L"bmp",false))
    {
        wxBitmap Texture(m_Paths[0]->c_str(),wxBITMAP_TYPE_BMP);

        if (Texture.Ok())
        {
            x=Texture.GetWidth();
            y=Texture.GetHeight();
            depth=Texture.GetDepth();

            m_Image=Texture.ConvertToImage();
        }
    }
    else
    {
        m_Image.LoadFile(m_Paths[0]->c_str());
        if (!m_Image.Ok())
            return;

        x=m_Image.GetWidth();
        y=m_Image.GetHeight();
        depth=-1;
    }
}

//--------------------------------------------------------------------------------
// CIwASDTextureLayout
//--------------------------------------------------------------------------------
wxSizerItem* CIwASDTextureLayout::Create(wxWindow* parent)
{
    m_Panel=new CIwASDTexturePanel(parent,this);

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);
}

//--------------------------------------------------------------------------------
void CIwASDTextureLayout::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    if (data==m_Data && base && Delete)
        m_Data=NULL;

    m_NeedReset=true;
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
void CIwASDTextureLayout::SetData(CIwASDData* data)
{
    if (!data->HasType(FILETYPE_TEXTURE)) return;

    m_Data=(CIwASDTextureFile*)data;
    m_Data->Load(0);
    Reset();
}

//--------------------------------------------------------------------------------
void CIwASDTextureLayout::Reset()
{
    if (m_Data==NULL)
        return;

    m_Panel->Update(0);
    m_NeedReset=false;
}

//--------------------------------------------------------------------------------
CIwASDTextureLayout::~CIwASDTextureLayout()
{
}

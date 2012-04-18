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
#include "IwASDBase.h"

//------------------------------------------------------------------------------
void CIwLayoutElement::Save(wxTextFile& fp,int indent)
{
    int i;
    wxString ind=wxString(L"\t",indent);

    if (m_Parent!=NULL)
        m_ParentData=m_Parent->GetParentData(this);

    fp.AddLine(ind+wxString::Format(L"%s %s %s",m_Type.c_str(),m_ParentData.c_str(),DoSave().c_str()));

    for (i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->Save(fp,indent+1);
    }
}

//------------------------------------------------------------------------------
CIwLayoutElement::~CIwLayoutElement()
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        delete m_Children[i];
    }
}

//------------------------------------------------------------------------------
void CIwLayoutElement::Layout()
{
    DoLayout();
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->Layout();
    }
}

//------------------------------------------------------------------------------
void CIwLayoutElement::CheckSave(std::vector<CIwASDData*>& dataList)
{
    DoCheckSave(dataList);

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->CheckSave(dataList);
    }
}

//------------------------------------------------------------------------------
void CIwLayoutElement::CheckForReset()
{
    DoCheckForReset();
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->CheckForReset();
    }
}

//------------------------------------------------------------------------------
void CIwModule::Setup(CIwASDApp* App,unsigned char ModuleNum)
{
    m_App=App;
    m_ModuleNum=ModuleNum;
    OnInit();
}

//------------------------------------------------------------------------------
int CIwModule::IsPrefix(const wxString& tag,unsigned int type)
{
    if (tag.Lower().StartsWith(m_Prefix.Lower()))
        return m_Prefix.size();

    return UNKNOWN_TAG;
}

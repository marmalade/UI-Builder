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

BEGIN_EVENT_TABLE(CIwActionMenu, wxMenu)
    EVT_MENU_RANGE(0,10000,CIwActionMenu::OnItem)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
CIwActionControl::~CIwActionControl()
{
    if (m_Action!=NULL)
        m_Action->RemoveControl(this);
}

//------------------------------------------------------------------------------
CIwAction::~CIwAction()
{
    for (int i=0; i<(int)m_Controls.size(); i++)
    {
        m_Controls[i]->m_Action=NULL;
    }
}

//------------------------------------------------------------------------------
void CIwAction::AddControl(CIwActionControl* ctrl)
{
    ctrl->m_Action=this;
    m_Controls.push_back(ctrl);
}

//------------------------------------------------------------------------------
void CIwAction::RemoveControl(CIwActionControl* ctrl)
{
    for (int i=0; i<(int)m_Controls.size(); i++)
    {
        if (m_Controls[i]==ctrl)
        {
            m_Controls.erase(m_Controls.begin()+i);
            break;
        }
    }
    if (m_Controls.size()==0 && m_Flags&FLAG_DELETEONNOCTRLS)
        DeleteThis();
}

//------------------------------------------------------------------------------
void CIwAction::Update()
{
    for (int i=0; i<(int)m_Controls.size(); i++)
    {
        m_Controls[i]->Update();
    }
}

//------------------------------------------------------------------------------
void CIwActionDelegate::Action(int value)
{
    if (!m_Delegate) return;

    m_Delegate();
}

//------------------------------------------------------------------------------
CIwAction* CIwActionDelegateDouble::MakeSubAction(const wxString& tag)
{
    if (m_Value!=1e100)
        return NULL;

    CIwActionDelegateDouble* d=new CIwActionDelegateDouble(m_Module,m_Delegate);
    d->m_Value=atof(tag.mb_str());

    return d;
}

//------------------------------------------------------------------------------
void CIwActionDelegateDouble::Action(int value)
{
    if (m_Value==1e100)
        return;

    if (!m_Delegate) return;

    m_Delegate(m_Value);
}

//------------------------------------------------------------------------------
CIwAction* CIwActionDelegateInt::MakeSubAction(const wxString& tag)
{
    if (m_Value!=-5000)
        return NULL;

    CIwActionDelegateInt* d=new CIwActionDelegateInt(m_Module,m_Delegate);
    d->m_Value=atof(tag.mb_str());

    return d;
}

//------------------------------------------------------------------------------
void CIwActionDelegateInt::Action(int value)
{
    if (m_Value==-5000)
        return;

    if (!m_Delegate) return;

    m_Delegate(m_Value);
}

//------------------------------------------------------------------------------
CIwAction* CIwActionDelegateParam::MakeSubAction(const wxString& tag)
{
    return new CIwActionDelegateParam(m_Module,m_Delegate,tag);
}

//------------------------------------------------------------------------------
void CIwActionDelegateParam::Action(int value)
{
    if (!m_Delegate) return;

    m_Delegate(m_FileName);
}

//------------------------------------------------------------------------------
CIwAction* CIwActionDelegateSub::MakeSubAction(const wxString& tag)
{
    if (m_Values==NULL) return NULL;

    for (int i=0; m_Values[i].m_Name!=NULL; i++)
    {
        if (tag.IsSameAs(m_Values[i].m_Name,false))
        {
            if (m_Overrides.find(m_Values[i].m_ID)!=m_Overrides.end())
                return m_Overrides[m_Values[i].m_ID];

            CIwActionDelegateSub* act=new CIwActionDelegateSub(m_Module,m_Delegate,NULL,m_GetDelegate,m_EnableDelegate);
            act->m_Value=m_Values[i].m_ID;
            return act;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------
void CIwActionDelegateSub::Action(int value)
{
    if (m_Value==0xffffffff)
        return;

    if (!m_Delegate) return;

    m_Delegate(m_Value,value);
}

//------------------------------------------------------------------------------
CIwActionDelegateSub::~CIwActionDelegateSub()
{
    std::map<unsigned int,CIwAction*>::iterator it;

    for (it=m_Overrides.begin(); it!=m_Overrides.end(); ++it)
    {
        delete it->second;
    }
}

//------------------------------------------------------------------------------
bool CIwActionDelegateSub::GetEnable(int value)
{
    if (!m_EnableDelegate) return true;

    return m_EnableDelegate(m_Value);
}

//------------------------------------------------------------------------------
int CIwActionDelegateSub::GetValue()
{
    if (!m_GetDelegate) return 0;

    return (int)m_GetDelegate(m_Value);
}

//------------------------------------------------------------------------------
void CIwActionDelegateBool::Action(int value)
{
    if (!m_Delegate) return;

    m_Delegate(value!=0);
}

//------------------------------------------------------------------------------
bool CIwActionDelegateBool::GetEnable(int value)
{
    if (!m_EnableDelegate) return true;

    return m_EnableDelegate();
}

//------------------------------------------------------------------------------
int CIwActionDelegateBool::GetValue()
{
    if (!m_GetDelegate) return 0;

    return (int)m_GetDelegate();
}

//------------------------------------------------------------------------------
int CIwActionDelegateList::GetValue()
{
    if (!m_UpdateDelegate) return 0;

    return m_UpdateDelegate();
}

//------------------------------------------------------------------------------
void CIwActionDelegateList::Action(int value)
{
    if (!m_Delegate) return;

    m_Delegate(value);
}

//------------------------------------------------------------------------------
bool CIwActionDelegateList::GetEnable(int value)
{
    if (!m_EnableDelegate) return true;

    return m_EnableDelegate(value);
}

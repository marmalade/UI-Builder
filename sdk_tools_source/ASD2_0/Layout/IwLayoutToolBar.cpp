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
void CIwLayoutElementToolbar::Load(std::vector<wxString>& argv)
{
    if (argv.size()==0) return;

    m_FileName=argv[0];

    LoadBar();
}

//------------------------------------------------------------------------------
wxString CIwLayoutElementToolbar::DoSave()
{
    return wxString::Format(L"\"%s\"",m_FileName.c_str());
}

//------------------------------------------------------------------------------
void CIwLayoutElementToolbar::LoadBar()
{
    wxTextFile fp(CIwTheApp->MakeAbsoluteFilename(m_FileName));
    if (!fp.Exists() || !fp.Open()) return;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> argv;
        CIwLayoutMenuDef* def;

        int argc=SuperSplit(fp[i],argv,L" \t\n");
        if (argc<1) continue;

        if (argv[0].IsSameAs(L"section",false))
        {
            if (argc<2) continue;

            m_Title=argv[1];
        }
        else if (argv[0].IsSameAs(L"tool",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(NULL,FTOOLITEM_TOOL);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
        else if (argv[0].IsSameAs(L"icon",false))
        {
            if (argc<4) continue;

            def=new CIwLayoutMenuDef(NULL,FTOOLITEM_ICON);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
        else if (argv[0].IsSameAs(L"check",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(NULL,FMENUITEM_CHECK);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
        else if (argv[0].IsSameAs(L"toggle",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(NULL,FTOOLITEM_TOGGLE);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
        else if (argv[0].IsSameAs(L"list",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(NULL,FMENUITEM_LIST);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
        else if (argv[0].IsSameAs(L"inlist",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(NULL,FMENUITEM_INLIST);
            def->Setup(argv);
            m_MenuDef.push_back(def);
        }
    }
}

//------------------------------------------------------------------------------
class CIwLayoutElementToolbarButton : public CIwStyleButton
{
    enum { CTRLID_BUTTON };
    CIwLayoutMenuDef* m_Def;
    void* m_Context;
public:
    CIwLayoutElementToolbarButton(wxWindow* parent,CIwLayoutMenuDef* Def,void* context) :
        CIwStyleButton(parent,CTRLID_BUTTON,Def->m_String),m_Def(Def),m_Context(context)
    {
        m_Def->m_Ctrl=this;
        m_Def->m_Context=m_Context;
        m_Def->Update();

        if (m_Def->m_Type==FTOOLITEM_ICON)
        {

        }
    }
    void OnButton(wxCommandEvent&)
    {
        if (m_Def->m_Action==NULL) return;

        m_Def->m_Context=m_Context;
        m_Def->m_Action->Action(1);
        m_Def->Update();
    }

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CIwLayoutElementToolbarBitmapButton : public wxBitmapButton
{
    enum { CTRLID_BUTTON };
    CIwLayoutMenuDef* m_Def;
    void* m_Context;
public:
    CIwLayoutElementToolbarBitmapButton(wxWindow* parent,int id,CIwLayoutMenuDef* Def,wxBitmap icon,void* context) :
        wxBitmapButton(parent,id,icon,wxPoint(-1,-1),wxSize(36,36),wxBU_AUTODRAW),m_Def(Def),m_Context(context)
    {
        m_Def->m_Ctrl=this;
        m_Def->m_Context=m_Context;
        m_Def->Update();

        SetToolTip(m_Def->m_String);
    }
    void OnButton(wxCommandEvent&)
    {
        if (m_Def->m_Action==NULL) return;

        m_Def->m_Context=m_Context;
        m_Def->m_Action->Action(1);
        m_Def->Update();
    }

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CIwLayoutElementToolbarToggleButton : public wxCustomButton
{
    enum { CTRLID_BUTTON };
    CIwLayoutMenuDef* m_Def;
    void* m_Context;
public:
    CIwLayoutElementToolbarToggleButton(wxWindow* parent,int id,CIwLayoutMenuDef* Def,wxBitmap icon,void* context) :
        wxCustomButton(parent,id,icon,wxPoint(-1,-1),wxSize(32,32)),m_Def(Def),m_Context(context)
    {
        m_Def->m_Ctrl=this;
        m_Def->m_Context=m_Context;
        m_Def->Update();

        if (m_Def->m_Action!=NULL && m_Def->m_Action->GetValue()!=0)
            SetValue(true);

        SetToolTip(m_Def->m_String);

        if (m_Def->m_Conditions.size()<2)
            return;

        wxString file=CIwTheApp->MakeAbsoluteFilename(L"{viewer}"+m_Def->m_Conditions[1]);
        SetBitmapSelected(wxBitmap(file,wxBITMAP_TYPE_PNG));
    }
    virtual void SetLabel(const wxString &label) { SetToolTip(label); }
    void OnButton(wxCommandEvent&)
    {
        if (m_Def->m_Action==NULL) return;

        m_Def->m_Context=m_Context;
        m_Def->m_Action->Action(GetValue() ? 1 : 0);
        m_Def->Update();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutElementToolbarButton, CIwStyleButton)
    EVT_BUTTON(CTRLID_BUTTON,CIwLayoutElementToolbarButton::OnButton)
END_EVENT_TABLE()

#define EVT_BUTTON_RANGE(id1, id2, func) wx__DECLARE_EVT2(wxEVT_COMMAND_BUTTON_CLICKED, id1, id2, wxCommandEventHandler(func))
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutElementToolbarBitmapButton, wxBitmapButton)
    EVT_BUTTON_RANGE(0,1000,CIwLayoutElementToolbarBitmapButton::OnButton)
END_EVENT_TABLE()

#define EVT_TOGGLEBUTTON_RANGE(id1, id2, func) wx__DECLARE_EVT2(wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, id1, id2, wxCommandEventHandler(func))
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutElementToolbarToggleButton, wxCustomButton)
    EVT_TOGGLEBUTTON_RANGE(0,1000,CIwLayoutElementToolbarToggleButton::OnButton)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
class CIwLayoutElementToolbarCheck : public wxCheckBox
{
    enum { CTRLID_CHECK };
    CIwLayoutMenuDef* m_Def;
    void* m_Context;
public:
    CIwLayoutElementToolbarCheck(wxWindow* parent,CIwLayoutMenuDef* Def,void* context) :
        wxCheckBox(parent,CTRLID_CHECK,Def->m_String),m_Def(Def),m_Context(context)
    {
        m_Def->m_Ctrl=this;
        m_Def->m_Context=m_Context;
        m_Def->Update();
    }
    void OnCheck(wxCommandEvent&)
    {
        if (m_Def->m_Action==NULL) return;

        m_Def->m_Context=m_Context;
        m_Def->m_Action->Action(IsChecked() ? 1 : 0);
        m_Def->Update();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutElementToolbarCheck, wxCheckBox)
    EVT_CHECKBOX(CTRLID_CHECK,CIwLayoutElementToolbarCheck::OnCheck)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
class CIwLayoutElementToolbarCombo : public wxComboBox
{
    enum { CTRLID_COMBO };
    CIwLayoutMenuDef* m_Def;
    void* m_Context;
public:
    CIwLayoutElementToolbarCombo(wxWindow* parent,CIwLayoutMenuDef* Def,void* context) :
        wxComboBox(parent,CTRLID_COMBO,L"",wxPoint(-1,-1),wxSize(150,-1),0,NULL,wxCB_READONLY),m_Def(Def),m_Context(context)
    {
        m_Def->m_Ctrl=this;
        m_Def->m_Context=m_Context;
        for (int i=0; i<(int)m_Def->m_Conditions.size(); i++)
        {
            Append(m_Def->m_Conditions[i]);
        }
        m_Def->Update();
    }
    void OnCombo(wxCommandEvent&)
    {
        if (m_Def->m_Action==NULL) return;

        m_Def->m_Context=m_Context;
        m_Def->m_Action->Action(GetSelection());
        m_Def->Update();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutElementToolbarCombo, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwLayoutElementToolbarCombo::OnCombo)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementToolbar::Create(wxWindow* parent)
{
    return Create(parent,this);
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementToolbar::Create(wxWindow* parent,void* context)
{
    m_Ctrl=parent;
    m_ToolBar=new CIwStyleButtonBar(parent,false);
    m_Tools.clear();

    for (int i=0; i<(int)m_MenuDef.size(); i++)
    {
        if (m_MenuDef[i]->m_Action!=NULL)
            if (!m_MenuDef[i]->m_Action->DoCreate())
                continue;

        if (m_MenuDef[i]->m_Type==FMENUITEM_LIST)
        {
            CIwLayoutElementToolbarCombo* tool=new CIwLayoutElementToolbarCombo(parent,m_MenuDef[i],context);
            m_ToolBar->Add(new wxStaticText(parent,wxID_ANY,m_MenuDef[i]->m_String),CIwStyleButtonBar::SPACE_SMALL);
            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
        else if (m_MenuDef[i]->m_Type==FMENUITEM_INLIST)
        {
            CIwLayoutElementToolbarCombo* tool=new CIwLayoutElementToolbarCombo(parent,m_MenuDef[i],context);
            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
        else if (m_MenuDef[i]->m_Type==FTOOLITEM_TOOL)
        {
            CIwLayoutElementToolbarButton* tool=new CIwLayoutElementToolbarButton(parent,m_MenuDef[i],context);
            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
        else if (m_MenuDef[i]->m_Type==FTOOLITEM_ICON)
        {
            wxButton* tool;
            if (m_MenuDef[i]->m_Conditions.empty())
                tool=new CIwLayoutElementToolbarButton(parent,m_MenuDef[i],context);
            else
            {
                wxString file=CIwTheApp->MakeAbsoluteFilename(L"{viewer}"+m_MenuDef[i]->m_Conditions[m_MenuDef[i]->m_Conditions.size()-1]);
                wxBitmap icon(file,wxBITMAP_TYPE_PNG);
                tool=new CIwLayoutElementToolbarBitmapButton(parent,i,m_MenuDef[i],icon,context);
            }

            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
        else if (m_MenuDef[i]->m_Type==FTOOLITEM_TOGGLE)
        {
            wxWindow* tool;
            if (m_MenuDef[i]->m_Conditions.empty())
                tool=new CIwLayoutElementToolbarButton(parent,m_MenuDef[i],context);
            else
            {
                wxString file=CIwTheApp->MakeAbsoluteFilename(L"{viewer}"+m_MenuDef[i]->m_Conditions[0]);
                wxBitmap icon(file,wxBITMAP_TYPE_PNG);
                tool=new CIwLayoutElementToolbarToggleButton(parent,i,m_MenuDef[i],icon,context);
            }

            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
        else
        {
            CIwLayoutElementToolbarCheck* tool=new CIwLayoutElementToolbarCheck(parent,m_MenuDef[i],context);
            m_ToolBar->Add(tool,CIwStyleButtonBar::SPACE_SMALL);
            m_Tools.push_back(tool);
        }
    }

    return new wxSizerItem(m_ToolBar,1,wxEXPAND|wxTOP,8,NULL);
}

//------------------------------------------------------------------------------
bool CIwLayoutElementToolbar::Query(EIwLayoutElementQuery value)
{
    switch (value)
    {
    case ELEMENT_QUERY_NODRAGNDROP:
    case ELEMENT_QUERY_CANNOTCLOSE:
    case ELEMENT_QUERY_ISTOOL:
        return true;
    default:
        break;
    }
    return false;
}

//------------------------------------------------------------------------------
void CIwLayoutElementToolbar::DoLayout()
{
    for (int i=0; i<(int)m_MenuDef.size(); i++)
    {
        m_MenuDef[i]->Update();
    }
}

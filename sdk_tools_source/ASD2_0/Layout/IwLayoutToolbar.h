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
#ifndef IW_LAYOUT_TOOLBAR_H
#define IW_LAYOUT_TOOLBAR_H

//------------------------------------------------------------------------------
// CIwLayoutElementToolbar
//	toolbar element
//------------------------------------------------------------------------------
class CIwLayoutElementToolbar : public CIwLayoutElement
{
public:
    wxString m_FileName;
    wxString m_Title;
    wxWindow* m_Ctrl;
    std::vector<CIwLayoutMenuDef*> m_MenuDef;
    std::vector<wxWindow*> m_Tools;
    CIwStyleButtonBar* m_ToolBar;
public:
    ~CIwLayoutElementToolbar()
    {
        for (int i=0; i<(int)m_MenuDef.size(); i++)
        {
            delete m_MenuDef[i];
        }
    }
    virtual void Load(std::vector<wxString>& argv);
    virtual wxString DoSave();
    virtual wxSizerItem* Create(wxWindow* parent);
    wxSizerItem* Create(wxWindow* parent,void* context);
    virtual bool Query(EIwLayoutElementQuery value);

    virtual wxWindow* GetControl() { return m_Ctrl; }
    virtual void DoLayout();

    void LoadBar();
};

#endif

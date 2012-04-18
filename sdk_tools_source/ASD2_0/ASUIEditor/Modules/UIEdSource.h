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
#ifndef IW_UIED_SOURCE_H
#define IW_UIED_SOURCE_H

class CIwUIEdSourceLayout;

//--------------------------------------------------------------------------------
// CIwUIEdSourcePanel
//--------------------------------------------------------------------------------
class CIwUIEdSourcePanel : public wxPanel
{
public:
    enum { CTRLID_SOURCE,CTRLID_SAVE,CTRLID_RELOAD };
public:
    wxSizer* m_Sizer;
    wxStyledTextCtrl* m_Text;
    bool m_ReLoaded;
    CUIEdAttrShared* m_Shared;
    CIwUIEdFileDataSource* m_Data;
    CUIEdProjectGroupLine* m_Line;
public:
    CIwUIEdSourcePanel(wxWindow* parent,CUIEdAttrShared* shared);
    ~CIwUIEdSourcePanel();

    void OnSaveOn(wxStyledTextEvent& e);
    void OnSaveOff(wxStyledTextEvent& e);

    void OnSave(wxCommandEvent&);
    void OnReload(wxCommandEvent&);
    bool GetText();

    void Reset();
    void SetData(CUIEdProjectGroupLine* line);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
// CIwUIEdSourceLayout
//--------------------------------------------------------------------------------
class CIwUIEdSourceLayout : public CIwLayoutElement
{
protected:
    CIwUIEdSourcePanel* m_Panel;
    bool m_NeedReset;
    CUIEdAttrShared* m_Shared;
public:

    CIwUIEdSourceLayout(CUIEdAttrShared* shared) : m_Panel(NULL),m_NeedReset(false),m_Shared(shared) {}

    virtual void Load(std::vector<wxString>& argv) {}
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual void DoLayout() { m_Panel->Layout(); }
    virtual wxWindow* GetControl() { return m_Panel; }

    virtual void DoCheckForReset() { if (m_NeedReset) m_Panel->Reset();

                                     m_NeedReset=false; }
    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
};

#endif // !IW_ASD_TREE_H

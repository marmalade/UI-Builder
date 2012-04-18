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
#ifndef UIEDPROJECT_H
#define UIEDPROJECT_H

//------------------------------------------------------------------------------
class CIwUIEdTreeFrame : public wxTreeCtrl
{
public:
    CUIEdAttrShared* m_Shared;
    wxMenu* m_PopupMenu;
public:
    CIwUIEdTreeFrame(wxWindow *parent,int id,CUIEdAttrShared* shared,int flags=wxTR_HAS_BUTTONS|wxTR_HIDE_ROOT|wxTR_SINGLE|wxTR_LINES_AT_ROOT) :
        wxTreeCtrl(parent,id,wxPoint(-1,-1),wxSize(-1,-1),flags),m_Shared(shared),m_PopupMenu(NULL) { }
    ~CIwUIEdTreeFrame() { if (m_PopupMenu!=NULL) delete m_PopupMenu; }

    void GetOpened(std::vector<wxString>& list,wxTreeItemId id);
    void SetOpened(std::vector<wxString>& list,int& offset,wxTreeItemId id);
    void SetOpenedAll(wxTreeItemId id);
};

//------------------------------------------------------------------------------
class CIwUIEdProjectFrame : public CIwUIEdTreeFrame
{
    class TreeData : public wxTreeItemData
    {
    public:
        CUIEdProjectGroupLine* m_Line;
        TreeData(CUIEdProjectGroupLine* line) : m_Line(line) {}
    };

    enum { CTRLID_TREE,CTRLID_ADD,CTRLID_MAKEGROUP,CTRLID_MAKEUI,CTRLID_MAKEITX,CTRLID_MAKEMAT,
           CTRLID_REMOVE,CTRLID_SETACTIVEITX,CTRLID_SETACTIVEMAT,CTRLID_VIEWSOURCE };

    CUIEdProjectGroupLine* currData;
public:
    CIwUIEdProjectFrame(wxWindow *parent,CUIEdAttrShared* shared) :
        CIwUIEdTreeFrame(parent,CTRLID_TREE,shared,wxTR_HAS_BUTTONS|wxTR_SINGLE|wxTR_LINES_AT_ROOT),currData(NULL)
    {
        m_Shared->m_ProjectFrame=this;
        SetupIcons();
    }

    bool MakeProject(bool fromGroup);
    void Reset();
    void SetupIcons();
    void Add(wxTreeItemId id,CUIEdProjectGroup* group);
    int GetIcon(const wxString& fileName);

    void OnDblClick(wxTreeEvent& e);
    void OnMenu(wxMouseEvent& e);
    void OnAdd(wxCommandEvent&);
    void OnMakeGroup(wxCommandEvent&);
    void OnMakeUI(wxCommandEvent&);
    void OnMakeITX(wxCommandEvent&);
    void OnActiveMat(wxCommandEvent&);
    void OnActiveITX(wxCommandEvent&);
    void OnMakeMat(wxCommandEvent&);
    void OnRemove(wxCommandEvent&);
    void OnIdle(wxIdleEvent&);
    void OnViewSource(wxCommandEvent&);
    void OnMakeStyleITX(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CUIEdProjectPane : public CIwLayoutElement
{
    CIwUIEdProjectFrame* m_Tree;
    CUIEdAttrShared* m_Shared;
public:
    CUIEdProjectPane(CUIEdAttrShared* shared) : m_Shared(shared) {}
    // CIwLayoutElement virtual overrides
    void        Load(std::vector<wxString>& argv){}
    void        DoLayout() { m_Tree->Layout(); }
    wxWindow*   GetControl() { return m_Tree; }

    wxSizerItem* Create(wxWindow* parent);
    void DoCheckSave(std::vector<CIwASDData*>& dataList);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
};

#endif

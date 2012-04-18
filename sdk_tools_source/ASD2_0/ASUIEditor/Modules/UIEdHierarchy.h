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
#ifndef UIEDHIERARCHY_H
#define UIEDHIERARCHY_H

class CIwUIEdHierFrame;

//-----------------------------------------------------------------------------
class CIwUIEdHierDropTarget : public wxTextDropTarget
{
    CIwUIEdHierFrame* m_Frame;
    wxTreeItemId lastId;
public:
    CIwUIEdHierDropTarget (CIwUIEdHierFrame* frame) : m_Frame(frame) { }
    virtual bool OnDropText(wxCoord x, wxCoord y, const wxString& data);

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
};

//------------------------------------------------------------------------------
class CIwUIEdHierFrame : public CIwUIEdTreeFrame
{
public:
    bool m_Selecting;
    CIwAttrInstance* currData;
    enum { CTRLID_TREE,CTRLID_ADD,CTRLID_REMOVE };
public:
    CIwUIEdHierFrame(wxWindow *parent,CUIEdAttrShared* shared);

    void Reset();
    void Add(wxTreeItemId id,CIwAttrInstance* inst);

    void OnSelect(wxTreeEvent& e);
    void OnMenu(wxMouseEvent& e);
    void OnRemove(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CUIEdHierPane : public CIwLayoutElement
{
    CIwUIEdHierFrame* m_Tree;
    CUIEdAttrShared* m_Shared;
public:
    CUIEdHierPane(CUIEdAttrShared* shared) : m_Shared(shared) {}
    // CIwLayoutElement virtual overrides
    void        Load(std::vector<wxString>& argv){}
    void        DoLayout() { m_Tree->Layout(); }
    wxWindow*   GetControl() { return m_Tree; }

    wxSizerItem* Create(wxWindow* parent);
    void DoCheckSave(std::vector<CIwASDData*>& dataList);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
};

#endif

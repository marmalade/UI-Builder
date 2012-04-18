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
#ifndef UIEDPALETTE_H
#define UIEDPALETTE_H


//-----------------------------------------------------------------------------
class CIwUIEdPaletteDropTarget : public wxDropTarget
{
    CIwUIEdPaletteFrame* m_Frame;
    wxTreeItemId lastId;
public:
    CIwUIEdPaletteDropTarget(CIwUIEdPaletteFrame* frame) : m_Frame(frame) { SetDataObject(new wxTextDataObject); }
    wxDragResult OnDropText(wxCoord x, wxCoord y, wxString data,wxDragResult def);
    virtual wxDragResult OnData(wxCoord x,wxCoord y,wxDragResult def);

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();
};

//------------------------------------------------------------------------------
class CIwUIEdPaletteFrame : public CIwUIEdTreeFrame
{
    int m_Mode;
    enum { CTRLID_TREE };
public:
    CIwUIEdPaletteFrame(wxWindow *parent,CUIEdAttrShared* shared,int mode) : CIwUIEdTreeFrame(parent,CTRLID_TREE,shared),m_Mode(mode)
    {
        if (mode==2)
            m_Shared->m_PaletteFrame2=this;
        else
            m_Shared->m_PaletteFrame=this;

        SetDropTarget(new CIwUIEdPaletteDropTarget(this));
        SetupIcons();
    }
    void Reset();
    void Add(wxTreeItemId parent,CIwAttrInstance* inst,int level);

    void OnDrag(wxTreeEvent& e);
    void Drop(CIwAttrInstance* inst,CIwAttrInstance* parent);

    void SetupIcons();
    int GetIcon(CIwAttrInstance* inst);

    ~CIwUIEdPaletteFrame(){}
    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CUIEdPalettePane : public CIwLayoutElement
{
    CIwUIEdPaletteFrame* m_Tree;
    CUIEdAttrShared* m_Shared;
    int m_Mode;
public:
    CUIEdPalettePane(CUIEdAttrShared* shared) : m_Shared(shared),m_Mode(0) {}

    void        Load(std::vector<wxString>& argv);
    void        DoLayout() { m_Tree->Layout(); }
    wxWindow*   GetControl() { return m_Tree; }

    wxSizerItem* Create(wxWindow* parent);
    void DoCheckSave(std::vector<CIwASDData*>& dataList);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
};
//------------------------------------------------------------------------------
class CIwUIEdMediaFrame : public wxPanel
{
    enum { CTRLID_SELECT,CTRLID_LIST };
    CUIEdAttrShared* m_Shared;
    wxComboBox* m_Select;
    wxListCtrl* m_List;
    wxImageList* m_Images;
public:
    CIwUIEdMediaFrame(wxWindow *parent,CUIEdAttrShared* shared);
    void Reset();
    void Add(std::vector<wxString>& exts);
    void Add(std::map<wxString,CUIEdAttrPropSet>& dict);

    void OnDrag(wxListEvent& e);
    void OnSelect(wxCommandEvent&);

    ~CIwUIEdMediaFrame(){}
    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------
class CIwUIEdMediaPane : public CIwLayoutElement
{
    CIwUIEdMediaFrame* m_Tree;
    CUIEdAttrShared* m_Shared;
public:
    CIwUIEdMediaPane(CUIEdAttrShared* shared) : m_Shared(shared) {}
    // CIwLayoutElement virtual overrides
    void        Load(std::vector<wxString>& argv){}
    void        DoLayout() { m_Tree->Layout(); }
    wxWindow*   GetControl() { return m_Tree; }

    wxSizerItem* Create(wxWindow* parent);
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
};

#endif

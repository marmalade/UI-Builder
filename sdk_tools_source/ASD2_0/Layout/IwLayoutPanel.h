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
#ifndef IW_LAYOUT_PANEL_H
#define IW_LAYOUT_PANEL_H


//--------------------------------------------------------------------------------
class CIwLayoutPanel : public wxPanel
{
public:
    CIwLayoutElement* m_Elem;
public:
    CIwLayoutPanel(wxWindow* parent,CIwLayoutElement* Elem,int style=wxTAB_TRAVERSAL);

    void OnMouseClick(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwLayoutSash : public wxSashLayoutWindow
{
public:
    CIwLayoutElement* m_Elem;
public:
    CIwLayoutSash(wxWindow* parent,CIwLayoutElement* Elem,int Id);

    void OnMouseClick(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwLayoutNotebook : public wxNotebook
{
    enum { CTRLID_NOTEBOOK };
public:
    CIwLayoutElement* m_Elem;
    int m_Curr;
public:
    CIwLayoutNotebook(wxWindow* parent,CIwLayoutElementNotebook* Elem);
    void Update();
    void StartDrag(CIwLayoutElement* Elem);

    void OnMouse(wxMouseEvent& event);
    void OnChanged(wxNotebookEvent& event);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwLayoutDnDPageTarget : public wxDropTarget
{
public:
    CIwLayoutElement* m_Elem;
    wxWindow* m_Ctrl;
public:
    CIwLayoutDnDPageTarget(CIwLayoutElement* elem,wxWindow* ctrl) : wxDropTarget(new CIwLayoutDnDObject(elem)),m_Elem(elem),m_Ctrl(ctrl) {}
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
};

#endif

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
#ifndef IW_LAYOUT_FRAME_MODULE_H
#define IW_LAYOUT_FRAME_MODULE_H

class CIwLayoutFrame;

//--------------------------------------------------------------------------------
// CIwFrameModule
//	this is the module that does basic frame stuff like exit from the game
//	and layout containers

//--------------------------------------------------------------------------------
class CIwLayoutElementSizer : public CIwLayoutElement
{
    wxWindow* m_Ctrl;
    int m_Orient;
    wxSizer* m_Sizer;
public:
    CIwLayoutElementSizer(int orient) : m_Orient(orient) {}
    virtual void Load(std::vector<wxString>& argv) {}
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual bool AddChild(wxSizerItem* child,CIwLayoutElement* elem);
    virtual void DoLayout() { m_Sizer->Layout(); }
    virtual wxString GetParentData(CIwLayoutElement* Child);
    virtual wxWindow* GetControl() { return m_Ctrl; }
};

//--------------------------------------------------------------------------------
class CIwLayoutElementSash  : public CIwLayoutElement
{
    wxPanel* m_Panel;
    wxPanel* m_MainPanel;
    int m_ID;
public:
    std::vector<wxSashLayoutWindow*> m_Sashes;
public:
    CIwLayoutElementSash();

    virtual void Load(std::vector<wxString>& argv) {}
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual bool AddChild(wxSizerItem* child,CIwLayoutElement* elem);
    virtual void DoLayout();
    virtual wxString GetParentData(CIwLayoutElement* Child);
    virtual wxWindow* GetControl() { return m_Panel; }
    virtual bool Query(EIwLayoutElementQuery value) { return (value==ELEMENT_QUERY_ISSASH); }
    wxSashLayoutWindow* GetSash(CIwLayoutElement* child);
};

//--------------------------------------------------------------------------------
class CIwLayoutElementNotebook : public CIwLayoutElement
{
    wxNotebook* m_Book;
    bool m_NoDnD;
    std::vector<wxString> m_Targets;
public:
    CIwLayoutElementNotebook() : m_NoDnD(false) {}

    virtual void Load(std::vector<wxString>& argv);
    virtual wxString DoSave();
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual bool AddChild(wxSizerItem* child,CIwLayoutElement* elem) { return AddChild(child,elem,m_Book->GetPageCount()); }
    bool AddChild(wxSizerItem* child,CIwLayoutElement* elem,int Pos);
    virtual void DoLayout() { m_Book->Update(); m_Book->Layout(); }
    virtual bool Query(EIwLayoutElementQuery value);

    virtual wxSizerItem* RemoveChild(CIwLayoutElement* elem,bool Delete);
    void RemoveAll(bool EmptyOnly);

    virtual wxDragResult DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data=NULL);
    virtual wxDragResult DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutElement* Child,CIwLayoutDnDObject* Data=NULL);
    void Move(CIwLayoutElement* Item,int To,bool Replace);
    virtual wxString GetParentData(CIwLayoutElement* Child);
    bool CanHandleData(const wxString& containerType);
    virtual wxWindow* GetControl() { return m_Book; }
    void UpdateTitles();
    void UpdateTitle(int i);
};

//--------------------------------------------------------------------------------
class CIwLayoutElementLog : public CIwLayoutElement
{
    wxTextCtrl* m_TextCtrl;
    wxString m_FileName;
public:
    virtual ~CIwLayoutElementLog();
    virtual void Load(std::vector<wxString>& argv);
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual void DoLayout() { m_TextCtrl->Layout(); }
    virtual wxWindow* GetControl() { return m_TextCtrl; }
};

#define FRAME_MODULE_ID 0
#define TAG_MENU_ACTION MODULE_TAG(FRAME_MODULE_ID,0)

//--------------------------------------------------------------------------------
class CIwFrameModule : public CIwModule
{
    CIwLayoutFrame* m_Frame;
public:
    CIwFrameModule(CIwLayoutFrame* Frame);

    virtual CIwLayoutElement* MakeElement(const wxString& type);
protected:
    virtual void OnInit();
};

#endif // !IW_LAYOUT_FRAME_H

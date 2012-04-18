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
#ifndef IW_LAYOUT_FRAME_H
#define IW_LAYOUT_FRAME_H

#define LAYOUTSHARED L"VLayoutShared.svl"

//------------------------------------------------------------------------------
// CIwLayoutFrame
//	main frame of the app
//------------------------------------------------------------------------------
enum EIwLayoutMenuItemType
{
    FMENUITEM_NULL,
    FMENUITEM_MENU,
    FMENUITEM_ITEM,
    FMENUITEM_CHECK,
    FMENUITEM_SEPARATOR,
    FMENUITEM_LIST,
    FMENUITEM_INLIST,
    FTOOLITEM_BAR,
    FTOOLITEM_TOOL,
    FTOOLITEM_ICON,
    FTOOLITEM_TOGGLE,
};

//------------------------------------------------------------------------------
class CIwLayoutMenuDef : public CIwActionControl
{
public:
    CIwLayoutMenuDef* m_Parent;
    std::vector<CIwLayoutMenuDef*> m_Children;

    EIwLayoutMenuItemType m_Type;
    wxString m_String;

    wxMenu* m_Menu;
    int m_ID;
    int m_NumSubIDs;
    wxWindow* m_Ctrl;
    void* m_Context;
public:
    CIwLayoutMenuDef() : m_Parent(NULL),m_Type(FMENUITEM_MENU),m_Menu(NULL),m_ID(-1),m_NumSubIDs(0),m_Ctrl(NULL),m_Context(NULL) {}
    CIwLayoutMenuDef(CIwLayoutMenuDef* parent,EIwLayoutMenuItemType type) :
        m_Parent(parent),m_Type(type),m_Menu(NULL),m_ID(0),m_NumSubIDs(0),m_Ctrl(NULL),m_Context(NULL)
    {
        if (m_Parent!=NULL)
            m_Parent->m_Children.push_back(this);
    }
    ~CIwLayoutMenuDef()
    {
        for (int i=0; i<(int)m_Children.size(); i++)
        {
            delete m_Children[i];
        }
    }

    void Setup(std::vector<wxString>& argv);
    virtual void Update();
    wxMenu* Create(int& id,void* context);
    bool Action(int id);
};

//--------------------------------------------------------------------------------
class CIwLayoutElemCont : public wxObject
{
public:
    CIwLayoutElement* m_Elem;
    int m_Num;
    CIwLayoutElemCont(CIwLayoutElement* Elem,int num) : m_Elem(Elem),m_Num(num) {}
};

//--------------------------------------------------------------------------------
class CIwLayoutSubFrame : public wxFrame
{
public:
    CIwLayoutElement* m_Elements;
    wxMenu* m_PopupMenu;
    CIwLayoutFrame* m_Parent;
    wxString m_Ident;

    wxSizer* m_Sizer;
public:
    CIwLayoutSubFrame(CIwLayoutFrame* parent,wxPoint pt,wxSize sz,const wxString& ident=L"");
    ~CIwLayoutSubFrame();

    void OnFrameClose(wxCloseEvent& e);
    void Close() { wxWindow::Close(false); }
    void Reset();

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
#ifdef ASD_AS_DLL
class CIwLayoutFrame : public wxDialog
{
#else
class CIwLayoutFrame : public wxFrame
{
#endif
public:
    CIwLayoutElement* m_Elements;
    CIwLayoutMenuDef m_MainMenu;
    CIwLayoutMenuDef m_TabMenu;
    CIwLayoutMenuDef m_RightMenu;
    wxMenu* m_PopupMenu;
    CIwLayoutElement* m_CurrDnDElem;
    CIwASDData* m_CurrDnDData;
    std::vector<CIwLayoutSubFrame*> m_SubWindows;

    wxSizer* m_Sizer;
    wxFrame* m_StatusWindow;
#ifdef ASD_AS_DLL
    wxToolBar* m_MenuToolBar;
#endif
public:
    CIwLayoutFrame();
    ~CIwLayoutFrame();

    void LoadMenuToolbar(const wxString& FileName);
    void LoadLayout(const wxString& FileName);
    void LoadLayoutDlg();
    void SaveLayout(const wxString& FileName);
    void SaveLayoutDlg();
    void EditTool();
    void SetMainTitle();
    void Reset();
    void Update();
    void LoadHelp(const wxString& FileName);
    void AboutBox(const wxString& Args);
    wxMenu* MakeMenu(CIwLayoutMenuDef& MenuDef,void* context);
    wxMenu* MakeMenu(CIwLayoutMenuDef& MenuDef,const wxString& name,void* context);
    void MakeMainMenu();
    void ReParent(wxSizerItem* item,wxWindow* parent);
    wxSizerItem* CreateLayout(CIwLayoutElement* Element,wxWindow* parent);

    bool CloseElem(CIwLayoutElement* elem,bool canVeto,bool all);

    void OnSashDrag(wxSashEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMainMenu(wxCommandEvent& event);
    void OnMainMenuTool(wxCommandEvent& event);
    void OnRMouse(CIwLayoutElement* Elem);
    void OnFrameClose(wxCloseEvent& e);
    void OnMainMenuOpen(wxMenuEvent& e);

    CIwLayoutElement* FindNext(EIwLayoutElementQuery Query,CIwLayoutElement* Curr=NULL);
    CIwLayoutElement* CreateNew(CIwLayoutElement* parent,const wxString& newContainerType,const wxString& parentData);
    CIwLayoutData* FindDataContainer(unsigned int dataType,const wxString& newContainerType,const wxString& addName=L"");
    bool ShowControl(wxWindow* win,CIwLayoutElement* Curr=NULL,int num=0);

    void Close() { wxWindow::Close(false); }

    DECLARE_EVENT_TABLE()
};

#endif

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
#ifndef IW_ASD_TREE_H
#define IW_ASD_TREE_H

class CIwASDTreeLayout;

//--------------------------------------------------------------------------------
// CIwASDTreePanel
//--------------------------------------------------------------------------------
class CIwASDTreePanel : public wxPanel
{
public:
    enum { CTRLID_TREE, };
    class CIwASDTreeData : public wxTreeItemData
    {
    public:
        CIwASDData* m_Data;
        CIwASDTreeData(CIwASDData* data) : m_Data(data) {}
    };
public:
    wxSizer* m_Sizer;
    wxTreeCtrl* m_Tree;
    CIwASDTreeLayout* m_Layout;
    std::vector<unsigned int> m_IconTypes;
    std::vector<CIwASDData*> m_ExpandList;
    wxMenu* m_PopupMenu;
public:
    CIwASDTreePanel(wxWindow* parent,CIwASDTreeLayout* layout,int style=wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT);
    ~CIwASDTreePanel() { if (m_PopupMenu!=NULL) delete m_PopupMenu; }

    void OnExpanding(wxTreeEvent& e);
    void OnDrag(wxTreeEvent& e);
    void OnMenu(wxTreeEvent& e);
    void OnSelected(wxTreeEvent& e);
    void OnKeyDown(wxTreeEvent& e);
    int GetIcon(unsigned int type);

    void StoreExpanded();
    void RestoreExpanded();
protected:
    void StoreExpanded(wxTreeItemId id);
    void RestoreExpanded(wxTreeItemId id);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
// CIwASDTreeLayout
//--------------------------------------------------------------------------------
class CIwASDTreeLayout : public CIwLayoutData
{
protected:
    CIwASDTreePanel* m_Panel;
    bool m_NeedReset;
    CIwASDData* m_Root;
    unsigned int m_RootType;
    wxString m_TitleType;
    std::vector<unsigned int> m_ConnectionTypes;
public:
    wxString m_MenuType;
public:
    CIwASDTreeLayout(unsigned int rootType,const wxString& titleType,const wxString& menuType) :
        m_Panel(NULL),m_NeedReset(false),m_Root(NULL),m_RootType(rootType),m_TitleType(titleType),m_MenuType(menuType) {}
    ~CIwASDTreeLayout() { }
    virtual void Load(std::vector<wxString>& argv) {}
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual wxSizerItem* DoCreate() { return NULL; }    //override to add specific stuff to the panel
    virtual void DoLayout()
    {
        m_Panel->Layout();
    }
    virtual wxWindow* GetControl() { return m_Panel; }
    virtual CIwASDData* GetData() { return m_Root; }

    virtual bool CanHandleData(unsigned int type) { return type==m_RootType; }
    virtual void RefreshData(CIwASDData* data,bool base,bool Delete);
    virtual wxDragResult DoDataDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data);
    virtual void Selected(CIwASDData* data,std::vector<CIwASDData*>& sel) {}

    virtual void DoCheckForReset() { if (m_NeedReset) Reset();

                                     m_NeedReset=false; }

    //override these to change data handling
    virtual void Expand(CIwASDData* data,wxTreeItemId id);
    virtual void Delete(CIwASDData* data) { }
protected:
    virtual void DoSetData(CIwASDData* data);
    void Reset();
    //override these to change data handling
    virtual bool HasChildren(CIwASDData* data);
    virtual bool Find(CIwASDData* data,CIwASDData* from);
    virtual CIwASDData* GetRoot() { return m_Root; }
    virtual wxTreeItemId MakeRootNode();
};

//--------------------------------------------------------------------------------
// CIwASDTreeLayoutProject
//--------------------------------------------------------------------------------
class CIwASDTreeLayoutProject : public CIwASDTreeLayout
{
protected:
public:
    CIwASDTreeLayoutProject() : CIwASDTreeLayout(FILETYPE_DIR,L"{file_name}",L"file")
    {
        m_ConnectionTypes.push_back(FILETYPE_DIR);
        m_ConnectionTypes.push_back(FILETYPE_FILE);
    }
};

#endif // !IW_ASD_TREE_H

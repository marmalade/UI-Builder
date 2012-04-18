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
#ifndef UIEDATTRPANE_H
#define UIEDATTRPANE_H

class CUIEdAttrShared;
class CIwAttrGrid;

//--------------------------------------------------------------------------------
class CIwASDUIEdAttrPanel : public wxPanel
{
protected:
    class CTree : public wxTreeCtrl
    {
        enum { CTRLID_TREE };
    public:
        CIwASDUIEdAttrPanel* m_Parent;

        CTree(CIwASDUIEdAttrPanel* parent) : wxTreeCtrl(parent,CTRLID_TREE,wxPoint(-1,-1),wxSize(-1,-1),wxTR_HAS_BUTTONS|wxTR_LINES_AT_ROOT|wxTR_HIDE_ROOT|wxTR_MULTIPLE),
            m_Parent(parent) {}

        void OnItemSelected(wxTreeEvent& event) { m_Parent->OnItemSelected(event); }
        //void OnItemActivated(wxTreeEvent& event) { m_Parent->OnItemActivated(event); }
        void OnDrag(wxTreeEvent& e) { m_Parent->OnDrag(e); }
        void OnKey(wxKeyEvent& e) { m_Parent->OnKey(e); }
        void OnMenu(wxTreeEvent& e) { m_Parent->OnMenu(e); }
        void OnMouseUp(wxMouseEvent& e) { m_Parent->OnMouseUp(e); }

        DECLARE_EVENT_TABLE()
    };
    class CShim : public CIwAttrDialog
    {
    public:
        CIwASDUIEdAttrPanel* m_Frame;
        CShim() : CIwAttrDialog(NULL,L"<>",0) {}
        virtual void Reset()            { m_Frame->m_Shared->Change(m_Frame->m_Sect,CHANGETYPE_BIG); }
        virtual void ScheduleReset()    { m_Frame->m_Shared->Change(m_Frame->m_Sect,CHANGETYPE_BIG|SELSOURCE_SCHEDULE); }
        virtual void SetChanged(bool Changed);
        virtual wxWindow* GetDlgItem(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) { return NULL; }
        virtual wxString GetBaseDir(bool fileBase);
        virtual void GridOverride(wxGrid* grid,int startLine,int endLine,CIwAttrData* data);
        virtual CIwAttrData* OverrideDataForGrid(CIwAttrData* data,bool checkFirst);
        virtual bool GridOverrideActivate(wxGrid* grid,CIwAttrData* data,std::vector<CIwAttrData*>& alts,int row,int col);
        virtual void DealWithChange(CIwAttrData* data);
        virtual bool GridOverrideCheck(CIwAttrData* data);

        void MakeButton(wxGrid* grid,CIwAttrData* data,const wxString& textName,int currRow,int currCol);
    };
    CShim m_Shim;
    CIwAttrDescMgr& m_GameMgrMeta;

    wxString m_OldAttrText;

    wxSizer* m_Sizer;
    wxSizer* m_AreaSizer;
    CIwStyleButtonBar* m_Bar;

    CIwAttrGrid* m_PropPanel;
    CIwStyleButton* m_OpenEdit;
    wxCustomButton* m_CheckBox;
    bool m_Selecting;
public:
    bool m_NeedsReset;
    wxString m_Type;

    wxTreeCtrl* m_Tree;
    wxComboBox* m_Selector;

    CUIEdAttrShared* m_Shared;
    CIwAttrInstance* m_Sect;
    CIwAttrInstance* m_OldSect;
    CIwAttrInstance* m_Base;
    CIwAttrInstance* currData;
    wxMenu* m_PopupMenu;

    enum { CTRLID_SAVE,CTRLID_RELOAD,CTRLID_OPENEDIT,CTRLID_REFRESH,CTRLID_LIVEUPDATE,CTRLID_REMOVE,CTRLID_STYLE,
           CTRLID_SETDEFAULT,CTRLID_ADD,CTRLID_CHECKNAMES,CTRLID_CHECKPROPUSAGE,CTRLID_ADDSTYLESHEET,CTRLID_SELECTOR };
public:
    CIwASDUIEdAttrPanel(wxWindow* parent,CUIEdAttrShared* shared,bool hasCombo);
    ~CIwASDUIEdAttrPanel() { if (m_PopupMenu!=NULL) delete m_PopupMenu; }

    void Reset();
    void ResetProp();

    virtual CIwAttrInstance* BuildTree()=0;
    void SetOpenedAll(wxTreeItemId parent);

    virtual void OnItemSelected(wxTreeEvent& event);
    //virtual void OnItemActivated(wxTreeEvent& event) {}
    virtual void Select(wxTreeItemId parent,CIwAttrInstance* inst,bool single=true);
    virtual void SetCurrent(CIwAttrInstance* inst);
    virtual void Remove(CIwAttrInstance* inst)=0;
    virtual void Drop(CIwAttrInstance* inst,CIwAttrInstance* parent,bool insert,bool move) {}
    virtual void NameChanged(CIwAttrData* data,bool forward);
    virtual void Drop(const wxString& type,const wxString& data,CIwAttrInstance* parent);
    virtual void Selected() {}

    virtual void OnChanged(bool Changed);
    wxString GetAttrText();
    wxString FilterAttrText(const wxString& curr,const wxString& old);

    virtual void OnSave(wxCommandEvent&);
    void OnRefresh(wxCommandEvent&);
    //void OnIdle(wxIdleEvent&);
    void OnLiveUpdate(wxCommandEvent&);
    void OnRemove(wxCommandEvent&);
    void OnRemove2(wxCommandEvent&);
    void OnReset(wxCommandEvent&);
    void OnMenu(wxTreeEvent& e);
    void OnDrag(wxTreeEvent& e);
    void OnCheckNames(wxCommandEvent&);
    void OnKey(wxKeyEvent& e);
    void OnSelector(wxCommandEvent&);
    virtual void OnMouseUp(wxMouseEvent& e);

    wxString GetName(wxTreeItemId id);
    wxTreeItemId FindName(std::vector<wxString>& args,int offset,wxTreeItemId parent);
    int GetIcon(CIwAttrInstance* inst);
    void SetupIcons();
    void UpdateSelList(std::vector<CIwAttrInstance*>& add,std::vector<CIwAttrInstance*>& remove);

    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
class CIwUIEdAttrDropTarget : public wxDropTarget
{
    CIwASDUIEdAttrPanel* m_Frame;
    wxTreeItemId lastId;
public:
    CIwUIEdAttrDropTarget(CIwASDUIEdAttrPanel* frame) : m_Frame(frame) { SetDataObject(new wxTextDataObject); }
    wxDragResult OnDropText(wxCoord x, wxCoord y, wxString data,wxDragResult def);
    virtual wxDragResult OnData(wxCoord x,wxCoord y,wxDragResult def);

    virtual wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    virtual void OnLeave();

    CIwAttrInstance* GetNext(CIwAttrInstance* parent);
    CIwAttrInstance* GetFirstChild(CIwAttrInstance* parent);
};

//--------------------------------------------------------------------------------
class CIwASDUIEdAttrUnityPanel : public CIwASDUIEdAttrPanel
{
public:
    CIwASDUIEdAttrUnityPanel(wxWindow* parent,CUIEdAttrShared* shared);
    virtual CIwAttrInstance* BuildTree();
    virtual void OnItemSelected(wxTreeEvent& event);
    //virtual void OnItemActivated(wxTreeEvent& event);
    virtual void Remove(CIwAttrInstance* inst);
    virtual void Drop(CIwAttrInstance* inst,CIwAttrInstance* parent,bool insert,bool move);
    virtual void NameChanged(CIwAttrData* data,bool forward);
    virtual void Selected();
    virtual void OnMouseUp(wxMouseEvent& e);

    void BuildTree(CUIEdAttrLinkData* link,wxTreeItemId id);
};
//--------------------------------------------------------------------------------
class CIwASDUIEdAttrStylePanel : public CIwASDUIEdAttrPanel
{
    wxComboBox* m_Style;
public:
    CIwASDUIEdAttrStylePanel(wxWindow* parent,CUIEdAttrShared* shared);
    virtual CIwAttrInstance* BuildTree();
    virtual void Remove(CIwAttrInstance* inst);

    void OnStyle(wxCommandEvent&);
    virtual void OnSave(wxCommandEvent&);
    void OnToDefault(wxCommandEvent&);
    void OnAdd(wxCommandEvent&);
    void OnCheckPropUsage(wxCommandEvent&);
    void OnAddStyleSheet(wxCommandEvent&);
    void Selected() { m_Shared->m_OpenEditor=1; }
    DECLARE_EVENT_TABLE()
};
//--------------------------------------------------------------------------------
class CIwASDUIEdAttrMaterialPanel : public CIwASDUIEdAttrPanel
{
public:
    CIwASDUIEdAttrMaterialPanel(wxWindow* parent,CUIEdAttrShared* shared);
    virtual CIwAttrInstance* BuildTree();
    virtual void Remove(CIwAttrInstance* inst);
    virtual void OnSave(wxCommandEvent&);
    void OnAdd(wxCommandEvent&);
    void OnCheckPropUsage(wxCommandEvent&);
    void Selected() { m_Shared->m_OpenEditor=2; }
    DECLARE_EVENT_TABLE()
};

enum EUIEdAttrPaneType
{
    ATTRPANETYPE_UNITY,
    ATTRPANETYPE_STYLE,
    ATTRPANETYPE_MATERIAL,
};

//------------------------------------------------------------------------------
class CUIEdAttrPane : public CIwLayoutElement
{
public:
    EUIEdAttrPaneType m_Type;
    CIwASDUIEdAttrPanel* m_Panel;
    CUIEdAttrShared* m_Shared;
public:
    CUIEdAttrPane(CUIEdAttrShared* shared,EUIEdAttrPaneType type) : m_Type(type),m_Shared(shared) {}
    wxSizerItem* Create(wxWindow* parent);

    virtual void Load(std::vector<wxString>& argv) {}
    virtual void DoLayout() { m_Panel->Layout(); }
    virtual wxWindow* GetControl() { return m_Panel; }
    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_NOICON; }
    virtual void Selected() { m_Panel->Selected(); }
};

#endif

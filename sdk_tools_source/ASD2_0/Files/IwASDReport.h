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
#ifndef IW_ASD_REPORT_H
#define IW_ASD_REPORT_H

class CIwASDReportLayout;

//--------------------------------------------------------------------------------
// CIwASDReportPanel
//--------------------------------------------------------------------------------
class CIwASDReportPanel : public wxPanel
{
public:
    enum { CTRLID_LIST, };
public:
    wxSizer* m_Sizer;
    wxListCtrl* m_List;
    CIwASDReportLayout* m_Layout;
    std::vector<unsigned int> m_IconTypes;
    wxMenu* m_PopupMenu;
public:
    CIwASDReportPanel(wxWindow* parent,CIwASDReportLayout* layout);
    ~CIwASDReportPanel() { if (m_PopupMenu!=NULL) delete m_PopupMenu; }

    int GetIcon(unsigned int type);
    void OnDrag(wxListEvent& e);
    void OnRClick(wxListEvent& e);
    void OnLClick(wxListEvent& e);
    void OnKeyDown(wxListEvent& e);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
// CIwASDTreeLayout
//--------------------------------------------------------------------------------
class CIwASDReportLayout : public CIwLayoutData
{
    struct Column
    {
        wxString m_Title;
        wxString m_Type;
        int      m_Width;
        Column(const wxString& title,const wxString& type,int width=wxLIST_AUTOSIZE_USEHEADER) :
                 m_Title(title),m_Type(type),m_Width(width) {}
    };
protected:
    CIwASDReportPanel* m_Panel;
    unsigned int m_RootType;
    std::vector<Column> m_Columns;
public:
    bool m_NeedReset;
    CIwASDData* m_Root;
    unsigned int m_ConnectionType;
    unsigned int m_InsertType;
    CIwASDDataConnection* m_Connection;
    int m_InsertPoint,m_InsertLen;
    wxString m_MenuType;
    wxString m_FilterShow;
    std::vector<ExpandType> m_ExpandTypes;
public:
    CIwASDReportLayout(unsigned int rootType, unsigned int connectionType,unsigned int insertType,const wxString& menuType,const wxString& filterShow) :
        m_Panel(NULL),m_RootType(rootType),m_NeedReset(false),m_Root(NULL),m_ConnectionType(connectionType),m_InsertType(insertType),m_Connection(NULL),
        m_InsertPoint(-1),m_MenuType(menuType),m_FilterShow(filterShow) { }
    ~CIwASDReportLayout() { }

    void AddColumn(const wxString& titleType,const wxString& title,int titleWidth=wxLIST_AUTOSIZE_USEHEADER)
    {
        m_Columns.push_back(Column(title,titleType,titleWidth));
    }

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

    virtual wxDragResult DoDataDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data=NULL);
    virtual wxDragResult DoDataDrop(wxPoint Pt,wxDragResult def,const wxArrayString& files);

    virtual void DoCheckForReset() { if (m_NeedReset) Reset();

                                     m_NeedReset=false; }
    virtual void DoSetData(CIwASDData* data);
    virtual void Selected(CIwASDData* data,std::vector<CIwASDData*>& sel) {}
    virtual CIwASDData* GetDataFromFilename(const wxString& name) { return NULL; }
protected:
    void Reset();
    bool DoDataDropStart(wxPoint Pt,wxDragResult& def);
    wxDragResult DoDropData(wxDragResult def,std::vector<CIwASDData*>& data,CIwLayoutElement* elem);
};

#endif // !IW_ASD_REPORT_H

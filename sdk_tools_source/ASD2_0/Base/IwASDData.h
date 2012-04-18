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
#ifndef IW_LAYOUT_DATA_H
#define IW_LAYOUT_DATA_H

class CIwLayoutData;

//--------------------------------------------------------------------------------
// CIwLayoutDataTarget
//
//--------------------------------------------------------------------------------
class CIwLayoutDataDnDTarget : public wxDropTarget
{
public:
    CIwLayoutData* m_Elem;
    wxWindow* m_Ctrl;
public:
    CIwLayoutDataDnDTarget(CIwLayoutData* elem,wxWindow* ctrl);
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
};

//--------------------------------------------------------------------------------
// CIwLayoutDataOrFileTarget
//
//--------------------------------------------------------------------------------
class CIwLayoutDataOrFileTarget : public wxDropTarget
{
public:
    CIwLayoutData* m_Elem;
    wxWindow* m_Ctrl;

    CIwLayoutDnDObject* m_Layout;
    wxFileDataObject* m_Files;
public:
    CIwLayoutDataOrFileTarget(CIwLayoutData* elem,wxWindow* ctrl);
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult def);
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult def);
};

//------------------------------------------------------------------------------
// CIwASDDataConnection
//	a list of data items connected to this data item base class
//------------------------------------------------------------------------------
class CIwASDDataConnection
{
public:
    //required item access functions
    virtual int size()=0;
    virtual CIwASDData* operator[](int i)=0;

    //required list modification functions
    virtual void push_back(CIwASDData* data)=0;
    virtual void insert(int i,CIwASDData* data)=0;
    virtual void erase(int i,bool remove)=0;
    virtual void clear()=0;
    virtual bool isRefList()=0;

    bool empty() { return size()==0; }
    virtual ~CIwASDDataConnection() {}
};

//------------------------------------------------------------------------------
// CIwLayoutData
//	base class for elements that have data
//------------------------------------------------------------------------------
class CIwLayoutData : public CIwLayoutElement
{
public:
    struct ExpandType
    {
        int                               m_Type;
        int                               m_ToType;
        bool                              m_Checked;
        bool                              m_PreExpand;
        wxString                          m_Name;

        ExpandType(int type,int toType) : m_Type(type),m_ToType(toType),m_Checked(true),m_PreExpand(true) {}
        ExpandType(int type,int toType,const wxString& name) : m_Type(type),m_ToType(toType),m_Checked(true),m_PreExpand(false),m_Name(name) {}
    };
public:
    std::vector<CIwASDData*> m_Datas;
public:
    virtual ~CIwLayoutData();

    virtual void SetData(CIwASDData* data);
    virtual void SetAuxData(CIwASDData* data) { }

    virtual CIwASDData* GetData() { return NULL; }
    //override to do something specific when the data is set
    virtual void DoSetData(CIwASDData* data) {}
    //override to do something when the data is changed
    virtual void RefreshData(CIwASDData* data,bool base,bool Delete) {}
    //override to signal if this data type is usable by this element
    virtual bool CanHandleData(unsigned int dataType) { return false; }
    //override to accept drop (if Data==NULL it is a test drop for cursor mode), second version is for external file drops
    virtual wxDragResult DoDataDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data=NULL) { return wxDragNone; }
    virtual wxDragResult DoDataDrop(wxPoint Pt,wxDragResult def,const wxArrayString& files) { return wxDragNone; }

    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList);

    virtual bool Query(EIwLayoutElementQuery value) { return value==ELEMENT_QUERY_ISDATACONTAINER; }
protected:
    bool FilterAndExpand(std::vector<CIwASDData*>& dataList,std::vector<ExpandType>& expandTypes,const wxString& show,bool all=false);
    CIwASDData* DropDataItem(wxDragResult& def,CIwASDData* data,CIwLayoutElement* elem,int insertType,CIwASDDataConnection* list,CIwASDData* root);
};

//------------------------------------------------------------------------------
// CIwASDData
//	base class for all data items
//------------------------------------------------------------------------------
class CIwASDData
{
public:
    enum EState
    {
        STATE_CLEAR=0,
        STATE_LOADED=1,
        STATE_CHANGED=2,
        STATE_READWRITE=4,
        STATE_NOTEDITABLE=8,
        STATE_NOTSAVEABLE=16,       //not editable even if the file is readwrite
    };
private:
    int m_State;    //EState
public:
    unsigned int m_Type;
    std::vector<CIwLayoutData*> m_Panels;
    std::map<unsigned int,CIwASDDataConnection*> m_Connections;
public:
    CIwASDData(unsigned int type) : m_State(STATE_CLEAR),m_Type(type) {}
    virtual ~CIwASDData();

    virtual void SetState(int value,int mask);  //EState,EState
    virtual void SetChanged(bool changed) { SetState(changed ? (STATE_LOADED|STATE_CHANGED) : (STATE_LOADED),STATE_LOADED|STATE_CHANGED); }
    bool HasChanged() { return (m_State&STATE_CHANGED)!=0; }
    int GetState(int mask=0xffff) { return m_State&mask; }

    void UpdateTitle();
    virtual wxString GetName() { return L""; }

    virtual void RefreshData(CIwASDData* data,bool base,bool Delete);
    virtual bool HasType(unsigned int type) { return false; }
    //fills the map with information strings about this item and optionally sets a colour value
    virtual void GetOptions(std::map<wxString,wxString>& opts,wxColour& col) {}
    virtual bool ExpandList(std::vector<CIwASDData*>& dataList,int toType) { return false; }

    virtual void CheckSave(bool force) {}
    virtual void Clear();
    virtual void ClearContents() {}
};

//------------------------------------------------------------------------------
//	linked to a list of data items
class CIwASDDataConxToList : public CIwASDDataConnection
{
private:
    std::vector<CIwASDData*>& m_List;
    bool m_RefList;
public:
    CIwASDDataConxToList(std::vector<CIwASDData*>& list,bool refList) : m_List(list),m_RefList(refList) {}
    virtual ~CIwASDDataConxToList() { clear(); }

    virtual int size() { return (int)m_List.size(); }
    virtual CIwASDData* operator[](int i) { return m_List[(unsigned int)i]; }

    virtual void push_back(CIwASDData* data) { m_List.push_back(data); }
    virtual void insert(int i,CIwASDData* data) { m_List.insert(m_List.begin()+i,data); }

    virtual void erase(int i,bool remove);
    virtual void clear();
    virtual bool isRefList() { return m_RefList; }
};

//	linked to a pointer to a data item
class CIwASDDataConxToItem : public CIwASDDataConnection
{
private:
    CIwASDData*& m_Item;
    bool m_RefItem;
public:
    CIwASDDataConxToItem(CIwASDData*& item,bool refItem) : m_Item(item),m_RefItem(refItem) {}
    virtual ~CIwASDDataConxToItem() { clear(); }

    virtual int size() { return (m_Item==NULL) ? 0 : 1; }
    virtual CIwASDData* operator[](int i) { return m_Item; }

    virtual void push_back(CIwASDData* data) { m_Item=data; }
    virtual void insert(int i,CIwASDData* data) { m_Item=data; }

    virtual void erase(int i,bool remove) { clear(); }
    virtual void clear() { if (!m_RefItem) delete m_Item;

                           m_Item=NULL; }
    virtual bool isRefList() { return m_RefItem; }
};

//	has a list of data items connected to this data item
class CIwASDDataConxList : public CIwASDDataConnection
{
private:
    std::vector<CIwASDData*> m_List;
    bool m_RefList;
public:
    CIwASDDataConxList(bool refList) : m_RefList(refList) {}
    virtual ~CIwASDDataConxList() { clear(); }

    virtual int size() { return (int)m_List.size(); }
    virtual CIwASDData* operator[](int i) { return m_List[(unsigned int)i]; }

    virtual void push_back(CIwASDData* data) { m_List.push_back(data); }
    virtual void insert(int i,CIwASDData* data) { m_List.insert(m_List.begin()+i,data); }

    virtual void erase(int i,bool remove);
    virtual void clear();
    virtual bool isRefList() { return m_RefList; }
};

//------------------------------------------------------------------------------
// CIwLayoutDnDObject
//	class that gets passed around on drag and drop
//--------------------------------------------------------------------------------
class CIwLayoutDnDObject : public wxDataObjectSimple
{
public:
    CIwLayoutElement* m_Elem;
    std::vector<CIwASDData*> m_Data;
    wxDataFormat m_Format;
public:
    CIwLayoutDnDObject(CIwLayoutElement* elem) : m_Elem(elem),m_Format(L"CIwLayoutDnDObject") { SetFormat(m_Format); }
    CIwLayoutDnDObject(CIwASDData* data) : m_Elem(NULL),m_Format(L"CIwLayoutDnDObject") { SetFormat(m_Format); m_Data.push_back(data); }
    CIwLayoutDnDObject() : m_Elem(NULL),m_Format(L"CIwLayoutDnDObject") { SetFormat(m_Format); }

    virtual size_t GetDataSize() const;
    virtual bool GetDataHere(void *buf) const;
    virtual bool SetData(size_t len, const void *buf);
};

#endif

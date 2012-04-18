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
#include "IwASDBase.h"

//------------------------------------------------------------------------------
CIwASDData::~CIwASDData()
{
    Clear();

    std::map<unsigned int,CIwASDDataConnection*>::iterator it;
    for (it=m_Connections.begin(); it!=m_Connections.end(); ++it)
    {
        delete it->second;
    }

    m_Connections.clear();
}

//------------------------------------------------------------------------------
void CIwASDData::Clear()
{
    RefreshData(this,true,true);

    for (int i=0; i<(int)m_Panels.size(); i++)
    {
        for (int j=0; j<(int)m_Panels[i]->m_Datas.size(); j++)
        {
            if (m_Panels[i]->m_Datas[j]==this)
            {
                m_Panels[i]->m_Datas.erase(m_Panels[i]->m_Datas.begin()+j);
                break;
            }
        }
    }

    m_Panels.clear();

    SetState(0,STATE_CHANGED|STATE_LOADED|STATE_READWRITE);
}

//------------------------------------------------------------------------------
void CIwASDData::SetState(int value,int mask)
{
    m_State=value|(m_State&~mask);

    if (mask&STATE_LOADED && CIwTheApp!=NULL)
        CIwTheApp->SetOpenState(this,(m_State&STATE_LOADED)==STATE_LOADED);
}

//------------------------------------------------------------------------------
void CIwASDData::UpdateTitle()
{
    for (int i=0; i<(int)m_Panels.size(); i++)
    {
        if (m_Panels[i]->m_Parent==NULL) return;

        if (!m_Panels[i]->m_Parent->Query(ELEMENT_QUERY_ISNOTEBOOK)) return;

        CIwLayoutElementNotebook* book=(CIwLayoutElementNotebook*)m_Panels[i]->m_Parent;
        book->UpdateTitles();
    }
}

//------------------------------------------------------------------------------
void CIwASDData::RefreshData(CIwASDData* data,bool base,bool Delete)
{
    SetChanged(true);
    for (int i=0; i<(int)m_Panels.size(); i++)
    {
        m_Panels[i]->RefreshData(data,base,Delete);
    }
    UpdateTitle();
}

//------------------------------------------------------------------------------
void CIwLayoutData::SetData(CIwASDData* data)
{
    int i;
    for (i=0; i<(int)data->m_Panels.size(); i++)
    {
        if (data->m_Panels[i]==this)
            break;
    }
    if (i==(int)data->m_Panels.size())
    {
        data->m_Panels.push_back(this);
        m_Datas.push_back(data);
    }

    DoSetData(data);
}

//------------------------------------------------------------------------------
void CIwLayoutData::DoCheckSave(std::vector<CIwASDData*>& dataList)
{
    if (GetData()==NULL) return;

    if (GetData()->HasChanged())
        dataList.push_back(GetData());
}

//------------------------------------------------------------------------------
CIwLayoutData::~CIwLayoutData()
{
    for (int i=0; i<(int)m_Datas.size(); i++)
    {
        for (int j=0; j<(int)m_Datas[i]->m_Panels.size(); j++)
        {
            if (m_Datas[i]->m_Panels[j]==this)
            {
                m_Datas[i]->m_Panels.erase(m_Datas[i]->m_Panels.begin()+j);
                break;
            }
        }
    }
    m_Datas.clear();
}

//------------------------------------------------------------------------------
CIwASDData* CIwLayoutData::DropDataItem(wxDragResult& def,CIwASDData* data,CIwLayoutElement* elem,int insertType,CIwASDDataConnection* list,CIwASDData* root)
{
    if (data==NULL) return NULL;

    if (def==wxDragCopy)
    {
        if (data->HasType(insertType))
        {
            if (!list->isRefList())
                return CIwTheApp->CloneDataObject(insertType,data,root,true);
            else
                return data;
        }
        else
        {
            if (!list->isRefList())
                return CIwTheApp->CloneDataObject(insertType,data,root,false);
            else
                def=wxDragNone;
        }
    }
    else
    {
        if (elem==this)
        {
            def=wxDragLink; //move in same list
            return data;
        }
        else if (data->HasType(insertType))
        {
            if (!list->isRefList())
                return CIwTheApp->CloneDataObject(insertType,data,root,true);
            else
            {
                def=wxDragCopy;
                return data;
            }
        }
        else
        {
            if (!list->isRefList())
            {
                def=wxDragCopy;
                return CIwTheApp->CloneDataObject(insertType,data,root,false);
            }
            else
                def=wxDragNone;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------
CIwLayoutDataDnDTarget::CIwLayoutDataDnDTarget(CIwLayoutData* elem,wxWindow* ctrl) :
    wxDropTarget(new CIwLayoutDnDObject(elem)),m_Elem(elem),m_Ctrl(ctrl)
{
}

//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDataDnDTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    if (CIwTheFrame->m_CurrDnDData==NULL)
        return wxDragNone;

    return m_Elem->DoDataDrop(Pt2,def);
}

//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDataDnDTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    if (CIwTheFrame->m_CurrDnDData==NULL)
        return wxDragNone;

    GetData();
    CIwLayoutDnDObject* Data=(CIwLayoutDnDObject*)GetDataObject();
    if (Data==NULL)
        return wxDragNone;

    wxDragResult Result=m_Elem->DoDataDrop(Pt2,def,Data);
    return Result;
}


//------------------------------------------------------------------------------
CIwLayoutDataOrFileTarget::CIwLayoutDataOrFileTarget(CIwLayoutData* elem,wxWindow* ctrl) :
    wxDropTarget(NULL),m_Elem(elem),m_Ctrl(ctrl),m_Layout(new CIwLayoutDnDObject(elem)),m_Files(new wxFileDataObject())
{
    wxDataObjectComposite* comp=new wxDataObjectComposite();
    comp->Add(m_Layout,true);
    comp->Add(m_Files);
    SetDataObject(comp);
}

//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDataOrFileTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult def)
{
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    return m_Elem->DoDataDrop(Pt2,def);
}

//--------------------------------------------------------------------------------
wxDragResult CIwLayoutDataOrFileTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
    wxDragResult Result;
    wxPoint Pt2(x,y);
    Pt2=m_Ctrl->ClientToScreen(Pt2);

    GetData();
    if (m_Files->GetFilenames().size()>0)
        Result=m_Elem->DoDataDrop(Pt2,def,m_Files->GetFilenames());
    else
        Result=m_Elem->DoDataDrop(Pt2,def,m_Layout);

    return Result;
}

//--------------------------------------------------------------------------------
size_t CIwLayoutDnDObject::GetDataSize() const
{
    return sizeof(CIwLayoutElement*)+sizeof(int)+m_Data.size()*sizeof(CIwASDData*);
}

//--------------------------------------------------------------------------------
bool CIwLayoutDnDObject::GetDataHere(void *buf) const
{
    int size=m_Data.size();
    memcpy(buf,&m_Elem,sizeof(CIwLayoutElement*));
    memcpy((char*)buf+sizeof(CIwLayoutElement*),&size,sizeof(int));
    for (int i=0; i<size; i++)
    {
        memcpy((char*)buf+sizeof(CIwLayoutElement*)+sizeof(int)+sizeof(CIwASDData*)*i,&m_Data[i],sizeof(CIwASDData*));
    }
    return true;
}

//--------------------------------------------------------------------------------
bool CIwLayoutDnDObject::SetData(size_t len, const void *buf)
{
    int size;
    memcpy(&m_Elem,buf,sizeof(CIwLayoutElement*));
    memcpy(&size,(char*)buf+sizeof(CIwLayoutElement*),sizeof(int));
    m_Data.clear();
    for (int i=0; i<size; i++)
    {
        CIwASDData* ptr;
        memcpy(&ptr,(char*)buf+sizeof(CIwLayoutElement*)+sizeof(int)+sizeof(CIwASDData*)*i,sizeof(CIwASDData*));
        m_Data.push_back(ptr);
    }
    return true;
}

//--------------------------------------------------------------------------------
void CIwASDDataConxToList::erase(int i,bool remove)
{
    if (!m_RefList && remove)
        delete m_List[i];

    m_List.erase(m_List.begin()+i);
}

//--------------------------------------------------------------------------------
void CIwASDDataConxToList::clear()
{
    if (!m_RefList)
        for (int i=0; i<(int)m_List.size(); i++)
        {
            delete m_List[i];
        }

    m_List.clear();
}

//--------------------------------------------------------------------------------
void CIwASDDataConxList::clear()
{
    if (!m_RefList)
        for (int i=0; i<(int)m_List.size(); i++)
        {
            delete m_List[i];
        }

    m_List.clear();
}

//--------------------------------------------------------------------------------
void CIwASDDataConxList::erase(int i,bool remove)
{
    if (!m_RefList && remove)
        delete m_List[i];

    m_List.erase(m_List.begin()+i);
}

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
#ifndef IWUIEDPROJECTUI_H
#define IWUIEDPROJECTUI_H

enum LayoutItemType
{
    LAYOUTITEM_UNKNOWN,
    LAYOUTITEM_LAYOUTITEM,  //eg a spacer
    LAYOUTITEM_ELEMENT,
    LAYOUTITEM_LAYOUT,
    LAYOUTITEM_CONTAINER,
    LAYOUTITEM_FOCUS,
    LAYOUTITEM_PROPERTYSET,
};

enum ActionModifier
{
    ACTIONMODIFIER_INSERT=1,
    ACTIONMODIFIER_TAKEALL=2,
    ACTIONMODIFIER_KEEPNAME=4,
    ACTIONMODIFIER_REMOVEPOSSIZE=8,
};

//--------------------------------------------------------------------------------
struct CUIEdAttrLinkData
{
    LayoutItemType                  m_Type;
    wxString                        m_Name;
    CIwAttrInstance*                m_Element;
    CIwAttrInstance*                m_LayoutItem;

    CUIEdAttrLinkData*              m_Parent;
    std::vector<CUIEdAttrLinkData*> m_Children;

    CUIEdAttrLinkData(const wxString& name,LayoutItemType type,CUIEdAttrLinkData* parent,CIwAttrInstance* elem,CIwAttrInstance* cont=NULL);
    virtual ~CUIEdAttrLinkData();

    CUIEdAttrLinkData*              GetLayout();
    CUIEdAttrLinkData*              GetFocus();
    CUIEdAttrLinkData*              GetParent(LayoutItemType type=LAYOUTITEM_UNKNOWN,CUIEdAttrLinkData** sibling=NULL);
    bool                            GotName(CUIEdAttrLinkData* base)
    {
        if (base!=this && m_Name.IsSameAs(base->m_Name,false))
            return true;

        for (int i=0; i<(int)m_Children.size(); i++)
        {
            if (m_Children[i]->GotName(base))
                return true;
        }
        return false;
    }
    void UnlinkAndRemove();
};

//--------------------------------------------------------------------------------
class CUIEdProjectUI : public CIwASDData
{
public:
    class Group : public CIwAttrFileGroup
    {
    public:
        CUIEdProjectUI* m_UI;
    public:
        virtual CIwAttrInstance* TryGetPtrItem(const wxString& Name,CIwAttrData* data);
        virtual wxString MakeNew(EIwAttrDialogStringType type,CIwAttrData* data=NULL,int offset=0);
        virtual void GetPtrStrings(wxArrayString& strings,CIwAttrData* data=NULL);
        virtual void SetupInlinePtr(CIwAttrInstance* inst,CIwAttrMember* member);
        virtual bool CheckAllowMultiple(CIwAttrInstance* inst,CIwAttrMember* member,std::vector<wxString>& argv,bool more);
        virtual wxBitmap* GetBitmap(CIwAttrData* data,EIwAttrDialogStringType type,bool* shouldHave=NULL);
        virtual wxBitmap* GetIcon(const wxString& textName);
    };
    wxString m_FileName;
    Group m_Group;

    CUIEdAttrLinkData m_BaseElement;

    CUIEdProjectUI();

    void Load();
    virtual void CheckSave(bool force);
    virtual wxString GetName()
    {
        wxFileName name(m_FileName);
        return name.GetFullName();
    }
    
    CIwAttrInstance* GetItem(const wxString& data);
    CIwAttrInstance* Add(CIwAttrInstance* item,CIwAttrInstance* parent,int actionModifer=0);
    CIwAttrInstance* Move(CIwAttrInstance* item,CIwAttrInstance* parent,int actionModifer=0);
    void Delete(CIwAttrInstance*& inst,bool deleteLinked=true);

    CIwAttrInstance* SizerPolicy(CIwAttrInstance* inst,CIwAttrInstance* parent);
    CIwAttrInstance* SetLayout(CIwAttrInstance* inst,CIwAttrInstance* parent);        
    CIwAttrInstance* FindLayoutItem(CIwAttrInstance* layout,const wxString& name);
    LayoutItemType GetLayoutElemType(CIwAttrInstance* elem);

    CUIEdAttrLinkData* SetupElements(CIwAttrInstance* elem,CUIEdAttrLinkData* parent=NULL);
    void SetupElements();
    bool SaveFiltered(wxString& fileName,int& num);

    CUIEdAttrLinkData* ResolveAddItem(CUIEdAttrLinkData* link);
    CIwAttrInstance* CopyFocus(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent);
    CUIEdAttrLinkData* Copy(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling,int actionModifer=0);
    CUIEdAttrLinkData* GetLink(CIwAttrInstance* inst);

    void Add2(CIwAttrInstance* inst,CIwAttrInstance* parent,CIwAttrInstance* sibling=NULL);
    void AddToLayout(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling=NULL);
    void AddToElement(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling=NULL);

    CIwAttrInstance* MakeContainer(const wxString& name);
    void SetName(CUIEdAttrLinkData* link);
    void CopyLayoutChildren(CUIEdAttrLinkData* parent,CUIEdAttrLinkData* old,int actionModifer=0);
    CUIEdAttrLinkData* CopyLayout(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent);
    
    bool IsParent(CIwAttrInstance* inst,CIwAttrInstance* parent);
    void MoveLink(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling=NULL);
};

#endif

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
#include "IwUIEdHeader.h"

//------------------------------------------------------------------------------
CUIEdAttrLinkData::CUIEdAttrLinkData(const wxString& name,LayoutItemType type,CUIEdAttrLinkData* parent,CIwAttrInstance* elem,CIwAttrInstance* cont) :
    m_Type(type),m_Name(name),m_Element(elem),m_LayoutItem(cont),m_Parent(parent)
{
    if (type!=LAYOUTITEM_PROPERTYSET && elem!=NULL)
        elem->m_TempClass=(CIwAttrClass*)this;

    if (cont!=NULL)
        cont->m_TempClass=(CIwAttrClass*)this;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData::~CUIEdAttrLinkData()
{
    if (m_Element!=NULL)
        m_Element->m_TempClass=NULL;

    if (m_LayoutItem!=NULL)
        m_LayoutItem->m_TempClass=NULL;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        delete m_Children[i];
    }
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdAttrLinkData::GetLayout()
{
    if (m_Type!=LAYOUTITEM_ELEMENT)
        return NULL;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (m_Children[i]->m_Type==LAYOUTITEM_LAYOUT)
            return m_Children[i];
    }
    return NULL;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdAttrLinkData::GetFocus()
{
    if (m_Type!=LAYOUTITEM_ELEMENT)
        return NULL;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (m_Children[i]->m_Type==LAYOUTITEM_FOCUS)
            return m_Children[i];
    }
    return NULL;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdAttrLinkData::GetParent(LayoutItemType type,CUIEdAttrLinkData** sibling)
{
    if (m_Parent==NULL)
        return NULL;

    if (sibling!=NULL)
        (*sibling)=this;

    if (type==LAYOUTITEM_UNKNOWN)
        return m_Parent;
    else if (m_Parent->m_Type==type)
        return m_Parent;
    else
        return m_Parent->GetParent(type,sibling);
}

//------------------------------------------------------------------------------
void CUIEdAttrLinkData::UnlinkAndRemove()
{
    int i;

    if (m_Element!=NULL)
        m_Element->m_TempClass=NULL;

    if (m_LayoutItem!=NULL)
        m_LayoutItem->m_TempClass=NULL;

    if (m_Parent!=NULL)
    {
        for (i=0; i<(int)m_Parent->m_Children.size(); i++)
        {
            if (m_Parent->m_Children[i]==this)
            {
                m_Parent->m_Children.erase(m_Parent->m_Children.begin()+i);
                break;
            }
        }
    }

    while (!m_Children.empty())
        m_Children[0]->UnlinkAndRemove();
    delete this;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::SetupElements()
{
    //for(int i=0;i<(int)m_BaseElement.m_Children.size();i++)
    //	delete m_BaseElement.m_Children[i];

    m_BaseElement.m_Children.clear();

    m_BaseElement.m_Element=m_Group.m_Inst;
    m_Group.m_Inst->m_TempClass=(CIwAttrClass*)&m_BaseElement;
    SetupElements(m_Group.m_Inst,&m_BaseElement);
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdProjectUI::SetupElements(CIwAttrInstance* elem,CUIEdAttrLinkData* parent)
{
    int i,j;
    CIwAttrInstance* cont=NULL;
    wxString name=L"";
    CIwAttrData* data=elem->FindData(L"name",0);
    if (data!=NULL)
        name=data->m_Items[0].m_String;

    LayoutItemType type=GetLayoutElemType(elem);
    bool add=(type!=LAYOUTITEM_CONTAINER) && (type!=LAYOUTITEM_UNKNOWN);

    if (type==LAYOUTITEM_ELEMENT && parent!=NULL)
    {
        CIwAttrData* layout=parent->m_Element->FindData(L"CIwUILayout",0);
        if (layout!=NULL && FindLayoutItem(layout->m_Items[0].m_Inst,name)!=NULL)
            return NULL;
    }

    if (type==LAYOUTITEM_CONTAINER)
    {
        CUIEdAttrLinkData* parent2;

        for (parent2=parent; parent2!=NULL; parent2=parent2->m_Parent)
        {
            if (parent2->m_Type==LAYOUTITEM_ELEMENT)
                break;
        }
        if (parent2!=NULL)
        {
            for (i=0; (i<(int)parent2->m_Element->m_Data.size()) && cont==NULL; i++)
            {
                for (j=0; j<(int)parent2->m_Element->m_Data[i]->m_Items.size(); j++)
                {
                    CIwAttrInstance* inst2=parent2->m_Element->m_Data[i]->m_Items[0].m_Inst;
                    if (inst2==NULL)
                        continue;

                    LayoutItemType type2=GetLayoutElemType(inst2);

                    wxString name2=L"";
                    data=inst2->FindData(L"name",0);
                    if (data!=NULL)
                        name2=data->m_Items[0].m_String;

                    if (name2.IsSameAs(name,false) && type2!=LAYOUTITEM_LAYOUT)
                    {
                        cont=elem;
                        elem=inst2;
                        type=LAYOUTITEM_ELEMENT;
                        add=true;
                        break;
                    }
                }
            }
        }
    }

    if (type==LAYOUTITEM_PROPERTYSET && parent==&m_BaseElement)
        return NULL;

    CUIEdAttrLinkData* link=NULL;
    if (add)
    {
        if (type!=LAYOUTITEM_PROPERTYSET)
            link=(CUIEdAttrLinkData*)elem->m_TempClass;
        else if (parent!=NULL)
        {
            for (i=0; i<(int)parent->m_Children.size(); i++)
            {
                if (parent->m_Children[i]->m_Element==elem)
                {
                    link=parent->m_Children[i];
                    break;
                }
            }
        }

        if (link==NULL && parent!=NULL)
        {
            link=new CUIEdAttrLinkData(name,type,parent,elem,cont);
            parent->m_Children.push_back(link);
        }
    }
    else
        link=parent;

    for (i=0; i<(int)elem->m_Data.size(); i++)
    {
        for (j=0; j<(int)elem->m_Data[i]->m_Items.size(); j++)
        {
            CIwAttrInstance* inst2=elem->m_Data[i]->m_Items[j].m_Inst;
            if (inst2==NULL)
                continue;

            if ((elem->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
                if ((elem->m_Data[i]->m_Items[j].m_Flags&ATTRITEM_ALLOCED_F)!=ATTRITEM_ALLOCED_F)
                    continue;

            CUIEdAttrLinkData* link2=SetupElements(inst2,link);
            if (link2==NULL)
                continue;

            if (type==LAYOUTITEM_CONTAINER)
            {
                link2->m_LayoutItem=elem;
                elem->m_TempClass=(CIwAttrClass*)link2;
            }

            if (link2->m_Type==LAYOUTITEM_PROPERTYSET)
            {
                link2->m_Name=elem->m_Data[i]->m_Member->m_Name;
                if (elem->m_Data[i]->m_Member->m_Type&ATTRMEMBER_LIST)
                    link2->m_Name+=wxString::Format(L"[%d]",j);
            }
        }
    }
    return link;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::FindLayoutItem(CIwAttrInstance* layout,const wxString& name2)
{
    for (int i=0; i<(int)layout->m_Data.size(); i++)
    {
        if (layout->m_Data[i]->m_Member->m_Name.IsSameAs(L"element",false))
        {
            CIwAttrInstance* inst2=layout->m_Data[i]->m_Items[0].m_Inst;
            wxString name=L"";
            CIwAttrData* data=inst2->FindData(L"name",0);
            if (data!=NULL)
                name=data->m_Items[0].m_String;

            if (name==name2)
                return inst2;

            inst2=FindLayoutItem(inst2,name2);
            if (inst2!=NULL)
                return inst2;
        }

        if (layout->m_Data[i]->m_Member->m_Name.IsSameAs(L"CIwUILayoutItem",false))
        {
            CIwAttrInstance* inst2=layout->m_Data[i]->m_Items[0].m_Inst;
            inst2=FindLayoutItem(inst2,name2);
            if (inst2!=NULL)
                return inst2;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
bool CUIEdProjectUI::SaveFiltered(wxString& fileName,int& num)
{
    bool doFilter=false;
    std::vector<CIwAttrClass*> classes;
    wxString notes1=m_Group.m_Inst->WriteNotes(0,NULL,true);
    wxString notes2=m_Group.m_Inst->WriteNotes(0,NULL,true,false,true);
    doFilter=notes1.IsSameAs(notes2);

    if (!doFilter) return false;

    int indent=0;
    wxString outstr;
    wxFileName name(m_FileName);
    fileName=wxString::Format(L"/_viewertemp/temp%d.%s",num++,name.GetExt().c_str());

    outstr+=notes2;
    outstr+=m_Group.m_Inst->WriteExtraNotes(0);

    wxTextFile fp(CIwTheHost.m_Shared.m_Project.m_RootDir+fileName);

    std::vector<wxString> lines;
    Split(outstr,lines,L"\n\r");
    for (int i=0; i<(int)lines.size(); i++)
    {
        fp.AddLine(lines[i]);
    }

    fp.Write();

    return true;
}
//------------------------------------------------------------------------------
void CUIEdProjectUI::CheckSave(bool force)
{
    if (!HasChanged() && !force) return;

    wxTextFile fp(m_FileName);
    m_Group.m_Inst->SaveExtra(fp);

    SetChanged(false);
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::Group::GetPtrStrings(wxArrayString& strings,CIwAttrData* data)
{
    CIwTheHost.m_Shared.GetStrings(ATTRSTRING_PTR,strings,data);
}
//------------------------------------------------------------------------------
wxString CUIEdProjectUI::Group::MakeNew(EIwAttrDialogStringType type,CIwAttrData* data,int offset)
{
    wxString klass;
    wxString klass2;
    wxString parent;

    if (data!=NULL)
    {
        if (data->m_Member->m_Items[0].m_Class!=NULL)
            klass=data->m_Member->m_Items[0].m_Class->m_Name;

        if (data->m_Member->m_Items.size()>1 && data->m_Member->m_Items[1].m_Class!=NULL)
            klass2=data->m_Member->m_Items[1].m_Class->m_Name;
        else
            klass2=data->m_Instance->m_Class->m_Name;

        parent=data->m_Items[offset].m_String;
    }

    return CIwTheHost.m_Shared.MakeNew(type,klass,klass2,parent);
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::Group::TryGetPtrItem(const wxString& Name,CIwAttrData* data)
{
    CUIEdAttrShared& shared=CIwTheHost.m_Shared;

    CIwAttrClass* klass=data->m_Member->m_Items[0].m_Class;
    if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwPropertySet",false))
    {
        std::map<wxString,CUIEdAttrPropSet>::iterator it=shared.m_PropSetDict.find(Name);
        if (it==shared.m_PropSetDict.end())
            return NULL;
        else
            return it->second.m_Inst;
    }
    else if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwMaterial",false))
    {
        std::map<wxString,CUIEdAttrPropSet>::iterator it=shared.m_MaterialDict.find(Name);
        if (it==shared.m_MaterialDict.end())
            return NULL;
        else
            return it->second.m_Inst;
    }
    else if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwUIStylesheet",false))
    {
        for (int i=0; i<(int)shared.m_StyleList.size(); i++)
        {
            if (shared.m_StyleList[i].m_StyleSheet==Name)
                return shared.m_StyleList[i].m_Inst;
        }
    }

    return NULL;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::Group::SetupInlinePtr(CIwAttrInstance* inst,CIwAttrMember* member)
{
    CIwAttrClass* klass=member->m_Items[0].m_Class;
    if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwPropertySet",false))
    {
        if (member->m_Items.size()>1)
            klass=member->m_Items[1].m_Class;
        else
            klass=inst->m_Parent->m_Instance->m_Class;

        if (klass!=NULL)
        {
            CIwAttrNote Note;
            std::vector<wxString> argv2;

            Note.m_Name=L"extraclass";
            Note.m_Data=klass->m_Name;
            argv2.push_back(klass->m_Name);
            inst->AddFromNote(L"extraclass",Note,argv2,inst);
        }
    }

    CUIEdAttrLinkData* link=(CUIEdAttrLinkData*)inst->m_TempClass;
    CUIEdAttrLinkData* parent=(CUIEdAttrLinkData*)inst->m_Parent->m_Instance->m_TempClass;
    if (link==NULL && parent!=NULL)
    {
        wxString name=wxString::Format(L"%s[%d]",inst->m_Parent->m_Member->m_Name.c_str(),inst->m_Parent->m_Items.size());
        CIwAttrData* data=inst->FindData(L"name",0);
        if (data!=NULL)
            data->m_Items[0].m_String=name;

        link=new CUIEdAttrLinkData(name,LAYOUTITEM_PROPERTYSET,parent,inst,NULL);
        parent->m_Children.push_back(link);
    }
}

//------------------------------------------------------------------------------
wxBitmap* CUIEdProjectUI::Group::GetBitmap(CIwAttrData* data,EIwAttrDialogStringType type,bool* shouldHave)
{
    CUIEdAttrShared& shared=CIwTheHost.m_Shared;
    if (shouldHave!=NULL)
        (*shouldHave)=false;

    if (type==ATTRSTRING_FILETEXTURE || type==ATTRSTRING_FILEFONT)
    {
        if (shouldHave!=NULL)
            (*shouldHave)=true;

        return shared.GetFileBitmap(data->m_Items[0].m_String,shared.m_Project.m_GroupData);
    }

    if (type==ATTRSTRING_PTR)
    {
        CIwAttrClass* klass=data->m_Member->m_Items[0].m_Class;
        if (klass==NULL || !klass->m_Name.IsSameAs(L"CIwMaterial",false))
            return NULL;

        if (shouldHave!=NULL)
            (*shouldHave)=true;

        if (data->m_Items[0].m_Inst==NULL)
            return NULL;

        CIwAttrData* name=data->m_Items[0].m_Inst->FindData(L"name");
        if (name==NULL || shared.m_MaterialDict.find(name->m_Items[0].m_String)==shared.m_MaterialDict.end()) return NULL;

        return &shared.m_MaterialDict[name->m_Items[0].m_String].m_Bmp;
    }

    return NULL;
}

//------------------------------------------------------------------------------
bool CUIEdProjectUI::Group::CheckAllowMultiple(CIwAttrInstance* inst,CIwAttrMember* member,std::vector<wxString>& argv,bool more)
{
    int j;

    if (member->m_Name==L"extraclass")
    {
        for (j=0; j<(int)inst->m_Data.size(); j++)
        {
            if (inst->m_Data[j]->m_Member==member && inst->m_Data[j]->m_Items[0].m_Class->m_Name.IsSameAs(argv[0],false))
                return false;
        }
        return true;
    }

    return more;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::Load()
{
    m_Group.m_Inst=CIwTheFileModule->GetAttrInstance(m_FileName,&m_Group);
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::GetItem(const wxString& data)
{
    std::vector<wxString> args;
    SuperSplit(data,args,L" ");

    CIwAttrInstance* inst=m_Group.m_Inst;
    for (int i=0; i<(int)args.size(); i++)
    {
        inst=inst->FindChild(args[i]);
        if (inst==NULL) return NULL;
    }
    return inst;
}

//------------------------------------------------------------------------------
CUIEdProjectUI::CUIEdProjectUI() : CIwASDData(UIEDTYPE_UI),m_BaseElement(L"base",LAYOUTITEM_UNKNOWN,NULL,NULL)
{
    m_Group.m_UI=this;
    m_Group.m_Override=&CIwTheHost.m_Shared.m_WriteOverride;
}

//------------------------------------------------------------------------------
LayoutItemType CUIEdProjectUI::GetLayoutElemType(CIwAttrInstance* elem)
{
    CIwAttrClass* sKlass=CIwTheFileMetaMgr.GetClass(L"CIwUILayoutItem");
    CIwAttrClass* lKlass=CIwTheFileMetaMgr.GetClass(L"CIwUILayout");
    CIwAttrClass* cKlass=CIwTheFileMetaMgr.GetClass(L"CIwUILayoutItemContainer");
    CIwAttrClass* eKlass=CIwTheFileMetaMgr.GetClass(L"CIwUIElement");
    CIwAttrClass* fKlass=CIwTheFileMetaMgr.GetClass(L"CIwUIFocusHandler");
    CIwAttrClass* pKlass=CIwTheFileMetaMgr.GetClass(L"CIwPropertySet");

    for (CIwAttrClass* klass2=elem->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
    {
        if (klass2==sKlass)
            return LAYOUTITEM_LAYOUTITEM;
        else if (klass2==lKlass)
            return LAYOUTITEM_LAYOUT;
        else if (klass2==eKlass)
            return LAYOUTITEM_ELEMENT;
        else if (klass2==fKlass)
            return LAYOUTITEM_FOCUS;
        else if (klass2==pKlass)
            return LAYOUTITEM_PROPERTYSET;
        else if (klass2==cKlass)
            return LAYOUTITEM_CONTAINER;
    }
    return LAYOUTITEM_UNKNOWN;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdProjectUI::ResolveAddItem(CUIEdAttrLinkData* link)
{
    if (link->m_Parent==NULL)
        return link;

    wxString mode=link->m_Parent->m_Name.BeforeLast('/').AfterLast(':');

    if (mode=='l')
        return link->GetLayout();
    else if (mode=='s')
    {
        link=link->GetLayout();
        if (link==NULL || link->m_Children.empty())
            return link;

        return link->m_Children[0];
    }
    else if (mode==L"f")
        return link->GetFocus();

    return link;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::Add2(CIwAttrInstance* inst,CIwAttrInstance* parent,CIwAttrInstance* sibling)
{
    if (inst==NULL || parent==NULL) return;

    if (parent->m_Class->GetClassMember(inst->m_Class)==NULL) return;

    CIwAttrData* data=new CIwAttrData;
    data->m_Mgr=parent->m_Mgr;
    data->m_Instance=parent;
    data->m_Member=parent->m_Class->GetClassMember(inst->m_Class);
    data->m_Info=-1;
    data->m_Group=NULL;
    data->SetDefault();

    if (sibling!=NULL)
    {
        for (int i=0; i<(int)parent->m_Data.size(); i++)
        {
            if (parent->m_Data[i]==sibling->m_Parent)
            {
                parent->m_Data.insert(parent->m_Data.begin()+i,data);
                break;
            }
        }
    }
    else
        parent->m_Data.push_back(data);

    inst->m_Parent=data;
    inst->SetupDlg(parent->m_Dialog,parent->m_File);

    data->m_Items.resize(1);
    data->m_Items[0].m_Flags=ATTRITEM_ALLOCED_F;
    data->m_Items[0].m_Inst=inst;
    data->m_FromDefault=false;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::CopyFocus(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent)
{
    int i;
    CIwAttrInstance* inst2=NULL;
    std::vector<CIwAttrNote> notes1;

    link->m_Element->FillNotes(notes1,true,0,true);

    if (parent->GetFocus()!=NULL)
        inst2=parent->GetFocus()->m_Element;

    for (i=0; i<(int)notes1.size(); i++)
    {
        if (notes1[i].m_Name.IsSameAs(L"class",false))
        {
            if (parent->GetFocus()==NULL)
            {
                std::vector<wxString> argv2;
                argv2.push_back(L"{");
                inst2=parent->m_Element->AddFromNote(L"C"+notes1[i].m_Data,notes1[i],argv2,parent->m_Element);

                CUIEdAttrLinkData* link2=new CUIEdAttrLinkData(link->m_Name,link->m_Type,parent,inst2);
                parent->m_Children.push_back(link2);
            }

            notes1.erase(notes1.begin()+i);
            break;
        }
    }

    if (inst2==NULL) return NULL;

    parent->GetFocus()->m_Name=link->m_Name;

    for (i=0; i<(int)inst2->m_Data.size(); i++)
    {
        delete inst2->m_Data[i];
    }
    inst2->m_Data.clear();
    inst2->m_ExtraData.clear();

    inst2->m_Class=link->m_Element->m_Class;
    inst2->AddDefaults(link->m_Element->m_Class);
    inst2->GetFromNotes(notes1);

    SetChanged(true);

    return inst2;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::SizerPolicy(CIwAttrInstance* inst,CIwAttrInstance* parent)
{
    std::vector<CIwAttrNote> notes;

    inst->FillNotes(notes,false,0,true);
    for (int i=0; i<(int)notes.size(); i++)
    {
        if (notes[i].m_Name==L"name")
        {
            notes.erase(notes.begin()+i);
            break;
        }
    }

    parent->GetFromNotes(notes);

    SetChanged(true);
    return parent;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::SetLayout(CIwAttrInstance* inst,CIwAttrInstance* parent)
{
    if (inst==NULL || parent==NULL) return NULL;

    int i;
    CIwAttrInstance* inst2=NULL;
    std::vector<CIwAttrNote> notes;
    std::vector<CIwAttrNote> notes1;
    std::vector<CIwAttrNote> notes2;
    CUIEdAttrLinkData* link2=NULL;
    CUIEdAttrLinkData* link=GetLink(inst);
    if (link==NULL) return NULL;

    link=ResolveAddItem(link);

    CUIEdAttrLinkData* parentLink=GetLink(parent);
    if (parentLink==NULL) return NULL;

    if (parentLink->m_Type!=LAYOUTITEM_ELEMENT)
        parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT);

    if (link==NULL)
    {
        link2=parentLink->GetLayout();

        if (link2!=NULL)
        {
            inst2=link2->m_Element;
            link2->UnlinkAndRemove();
            CIwTheHost.m_Shared.Remove(inst2);
            delete inst2;
        }

        SetupElements(parentLink->m_Element,parentLink->m_Parent);
        SetChanged(true);
        return parent;
    }

    link->m_Element->FillNotes(notes1,true,0,true);

    if (parentLink->GetLayout()!=NULL)
    {
        link2=parentLink->GetLayout();
        inst2=link2->m_Element;

        for (i=0; i<(int)link2->m_Children.size(); i++)
        {
            delete link2->m_Children[i];
        }
        link2->m_Children.clear();
    }

    for (i=0; i<(int)notes1.size(); i++)
    {
        if (notes1[i].m_Name.IsSameAs(L"class",false))
        {
            if (parentLink->GetLayout()==NULL)
            {
                std::vector<wxString> argv2;
                argv2.push_back(L"{");
                inst2=parentLink->m_Element->AddFromNote(L"C"+notes1[i].m_Data,notes1[i],argv2,parentLink->m_Element);

                link2=new CUIEdAttrLinkData(link->m_Name,link->m_Type,parentLink,inst2);
                parentLink->m_Children.push_back(link2);
            }

            notes1.erase(notes1.begin()+i);
            break;
        }
    }

    if (inst2==NULL) return NULL;

    inst2->FillNotes(notes,false,0,true);

    parentLink->GetLayout()->m_Name=link->m_Name;

    for (i=0; i<(int)inst2->m_Data.size(); i++)
    {
        delete inst2->m_Data[i];
    }
    inst2->m_Data.clear();
    inst2->m_ExtraData.clear();

    inst2->m_Class=link->m_Element->m_Class;
    inst2->AddDefaults(link->m_Element->m_Class);

    int mode=0;
    int info=0;

    for (i=0; i<(int)notes1.size(); i++)
    {
        switch (mode)
        {
        case 0:
            if (notes1[i].m_Name.IsSameAs(L"element"))
            {
                info=notes1[i].m_Info;
                mode=1;
            }
            else
                notes2.push_back(notes1[i]);

            break;
        case 1:
            if (notes1[i].m_Name.IsSameAs(L"}") && notes1[i].m_Info==info)
                mode=0;

            break;
        }
    }
    for (i=0; i<(int)notes.size(); i++)
    {
        switch (mode)
        {
        case 0:
            if (notes[i].m_Name.IsSameAs(L"element"))
            {
                info=notes[i].m_Info;
                notes2.push_back(notes[i]);
                mode=1;
            }

            break;
        case 1:
            notes2.push_back(notes[i]);
            if (notes[i].m_Name.IsSameAs(L"}") && notes[i].m_Info==info)
                mode=0;

            break;
        }
    }
    inst2->GetFromNotes(notes2);
    SetName(link2);
    SetupElements(link2->m_Element,parentLink);

    for (i=0; i<(int)parentLink->m_Children.size(); i++)
    {
        link=parentLink->m_Children[i];
        if (link->m_Type==LAYOUTITEM_ELEMENT)
        {
            AddToLayout(link,link2);
            if (link->m_LayoutItem!=NULL)
            {
                parentLink->m_Children.erase(parentLink->m_Children.begin()+i);
                i--;
                link2->m_Children.push_back(link);
                link->m_Parent=link2;
            }
        }
    }

    SetChanged(true);

    return link2->m_Element;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdProjectUI::CopyLayout(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent)
{
    int i;
    CIwAttrInstance* inst2=NULL;
    std::vector<CIwAttrNote> notes1;
    CUIEdAttrLinkData* link2=NULL;

    link->m_Element->FillNotes(notes1,true,0,true);

    if (parent->GetLayout()!=NULL)
    {
        link2=parent->GetLayout();
        inst2=link2->m_Element;
    }

    for (i=0; i<(int)notes1.size(); i++)
    {
        if (notes1[i].m_Name.IsSameAs(L"class",false))
        {
            if (parent->GetLayout()==NULL)
            {
                std::vector<wxString> argv2;
                argv2.push_back(L"{");
                inst2=parent->m_Element->AddFromNote(L"C"+notes1[i].m_Data,notes1[i],argv2,parent->m_Element);

                link2=new CUIEdAttrLinkData(link->m_Name,link->m_Type,parent,inst2);
                parent->m_Children.push_back(link2);
            }

            notes1.erase(notes1.begin()+i);
            break;
        }
    }

    if (inst2==NULL) return NULL;

    parent->GetLayout()->m_Name=link->m_Name;

    for (i=0; i<(int)inst2->m_Data.size(); i++)
    {
        delete inst2->m_Data[i];
    }
    inst2->m_Data.clear();
    inst2->m_ExtraData.clear();

    inst2->m_Class=link->m_Element->m_Class;
    inst2->AddDefaults(link->m_Element->m_Class);
    inst2->GetFromNotes(notes1);

    SetChanged(true);

    return link2;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::SetName(CUIEdAttrLinkData* link)
{
    int num=0;
    if (link->m_Type==LAYOUTITEM_PROPERTYSET)
        return;

    if (link->m_Parent->m_Element->m_Class->m_Name.IsSameAs(L"data",true) && link->m_Name.Find(L"/")==-1)
    {
        wxFileName fname(m_FileName);
        link->m_Name=fname.GetName()+L"/"+link->m_Name;
    }

    while (true)
    {
        if (link->m_Name.Find(L"_")!=-1)
            link->m_Name=link->m_Name.BeforeLast('_');

        link->m_Name+=wxString::Format(L"_%d",num++);

        if (!m_BaseElement.GotName(link))
            break;
    }

    if (link->m_Element!=NULL)
    {
        CIwAttrData* data=link->m_Element->FindData(L"name",0);
        if (data!=NULL)
            data->m_Items[0].m_String=link->m_Name;
    }

    if (link->m_LayoutItem!=NULL)
    {
        CIwAttrData* data=link->m_LayoutItem->FindData(L"name",0);
        if (data!=NULL)
            data->m_Items[0].m_String=link->m_Name;
    }
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdProjectUI::Copy(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling,int actionModifer)
{
    int i;
    CIwAttrInstance* inst2=NULL;
    std::vector<CIwAttrNote> notes;

    link->m_Element->FillNotes(notes,true,0,true);

    std::vector<CIwAttrClass*> Classes;
    Classes.push_back(CIwTheFileMetaMgr.GetClass(L"CIwUILayoutItem"));
    Classes.push_back(CIwTheFileMetaMgr.GetClass(L"CIwUIElement"));
    Classes.push_back(CIwTheFileMetaMgr.GetClass(L"CIwPropertySet"));
    Classes.push_back(CIwTheFileMetaMgr.GetClass(L"CIwUIFocusHandler"));
    CIwTheFileMetaMgr.GetDerivedClasses(Classes);

    if ((actionModifer&ACTIONMODIFIER_REMOVEPOSSIZE)==ACTIONMODIFIER_REMOVEPOSSIZE)
    {
        for (i=0; i<(int)notes.size(); )
        {
            if (notes[i].m_Name==L"pos"|| notes[i].m_Name==L"size")
                notes.erase(notes.begin()+i);
            else
                i++;
        }
    }

    int found=-1;
    for (i=0; i<(int)notes.size(); )
    {
        if (found==-1)
        {
            if (notes[i].m_Name==L"CIwUILayout")
            {
                found=atoi(notes[i].m_Data.mb_str()+1);
                notes.erase(notes.begin()+i);
            }
            else i++;
        }
        else
        {
            if (notes[i].m_Name==L"}" && notes[i].m_ID==found)
                found=-1;

            notes.erase(notes.begin()+i);
        }
    }

    inst2=CIwTheFileMetaMgr.GetFromNotes(notes,Classes,true,&m_Group);

    CUIEdAttrLinkData* link2=new CUIEdAttrLinkData(link->m_Name,link->m_Type,parent,inst2);

    if (sibling!=NULL)
    {
        for (i=0; i<(int)parent->m_Children.size(); i++)
        {
            if (parent->m_Children[i]==sibling)
            {
                parent->m_Children.insert(parent->m_Children.begin()+i,link2);
                break;
            }
        }
        if (i==(int)parent->m_Children.size())
            parent->m_Children.push_back(link2);
    }
    else
        parent->m_Children.push_back(link2);

    SetChanged(true);

    return link2;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::AddToElement(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling)
{
    int i;
    CIwAttrInstance* inst2=link->m_Element;

    if (link->m_Type==LAYOUTITEM_PROPERTYSET)
    {
        CIwAttrData* data=parent->m_Element->FindData(link->m_Element->m_Parent->m_Member->m_Name);
        if (data==NULL) return;

        if (sibling==NULL)
        {
            i=data->m_Items.size();
            data->m_Items.resize(i+1);
        }
        else
        {
            for (i=0; i<(int)data->m_Items.size(); i++)
            {
                if (data->m_Items[i].m_Inst==sibling->m_Element)
                {
                    data->m_Items.insert(data->m_Items.begin()+i,CIwAttrMemberItem());
                    break;
                }
            }
            if (i==(int)data->m_Items.size())
                data->m_Items.resize(i+1);
        }

        data->m_Items[i].m_Inst=inst2;
        data->m_Items[i].m_Flags=ATTRITEM_ALLOCED_F;
        inst2->m_Parent=data;

        if (data->m_Member->m_Type&ATTRMEMBER_LIST)
            link->m_Name=data->m_Member->m_Name+wxString::Format(L"[%d]",i);
        else
            link->m_Name=data->m_Member->m_Name;
    }
    else if (sibling==NULL)
        Add2(inst2,parent->m_Element);
    else
        Add2(inst2,parent->m_Element,sibling->m_Element);

    SetupElements(inst2,parent);
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::MakeContainer(const wxString& name)
{
    CIwAttrNote note;
    std::vector<CIwAttrNote> notes;
    std::vector<CIwAttrClass*> Classes;

    note.m_Name=L"class";
    note.m_Data=L"IwUILayoutItemContainer";
    notes.push_back(note);
    note.m_Name=L"name";
    note.m_Data=name;
    notes.push_back(note);

    Classes.push_back(CIwTheFileMetaMgr.GetClass(L"CIwUILayoutItemContainer"));

    return CIwTheFileMetaMgr.GetFromNotes(notes,Classes,true,&m_Group);
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::AddToLayout(CUIEdAttrLinkData* link,CUIEdAttrLinkData* parent,CUIEdAttrLinkData* sibling)
{
    link->m_LayoutItem=parent->m_Element->FindChild(link->m_Name);
    if (link->m_LayoutItem!=NULL)
    {
        link->m_LayoutItem->m_TempClass=(CIwAttrClass*)link;
        return;
    }

    link->m_LayoutItem=MakeContainer(link->m_Name);

    if (link->m_LayoutItem==NULL)
        return;

    link->m_LayoutItem->m_TempClass=(CIwAttrClass*)link;

    int i;
    int row=-1,col=-1;
    CIwAttrData* data;

    for (CIwAttrClass* klass=parent->m_Element->m_Class; klass!=NULL; klass=klass->m_Parent)
    {
        if (klass->m_Name.IsSameAs(L"CIwUILayoutGrid",false))
        {
            int numcols=0,numrows=0;
            std::vector<CIwAttrData*> elems;
            for (i=0; i<(int)parent->m_Element->m_Data.size(); i++)
            {
                if (parent->m_Element->m_Data[i]->m_Member->m_Name.IsSameAs(L"element",false))
                    elems.push_back(parent->m_Element->m_Data[i]);

                if (parent->m_Element->m_Data[i]->m_Member->m_Name.IsSameAs(L"row",false))
                    numrows++;

                if (parent->m_Element->m_Data[i]->m_Member->m_Name.IsSameAs(L"column",false))
                    numcols++;
            }
            CIwAttrData* colNum=parent->m_Element->FindData(L"numColumns");
            CIwAttrData* rowNum=parent->m_Element->FindData(L"numRows");

            if (numcols==0 && colNum!=NULL)
                numcols=colNum->m_Items[0].m_Int;

            if (numcols==0) numcols=1;

            if (numrows==0 && rowNum!=NULL)
                numrows=rowNum->m_Items[0].m_Int;

            if (numrows==0) numrows=1;

            for (int num=0; num<numrows*numcols; num++)
            {
                row=num/numcols;
                col=num%numcols;
                for (i=0; i<(int)elems.size(); i++)
                {
                    CIwAttrData* rowV=elems[i]->m_Items[0].m_Inst->FindData(L"row");
                    CIwAttrData* colV=elems[i]->m_Items[0].m_Inst->FindData(L"column");
                    if (rowV!=NULL && colV!=NULL)
                        if (rowV->m_Items[0].m_Int==row && colV->m_Items[0].m_Int==col)
                            break;

                }
                if (i==(int)elems.size())
                    break;

                row=col=-1;
            }

            break;
        }
    }

    if (sibling==NULL)
        Add2(link->m_LayoutItem,parent->m_Element);
    else
        Add2(link->m_LayoutItem,parent->m_Element,sibling->m_Element);

    if (row!=-1)
    {
        data=link->m_LayoutItem->FindData(L"row");
        if (data!=NULL)
            data->m_Items[0].m_Int=row;
    }

    if (col!=-1)
    {
        data=link->m_LayoutItem->FindData(L"column");
        if (data!=NULL)
            data->m_Items[0].m_Int=col;
    }
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::CopyLayoutChildren(CUIEdAttrLinkData* parent,CUIEdAttrLinkData* old,int actionModifer)
{
    for (int i=0; i<(int)old->m_Children.size(); i++)
    {
        CIwAttrInstance* inst=Add(old->m_Children[i]->m_Element,parent->m_Element,ACTIONMODIFIER_KEEPNAME);
        CUIEdAttrLinkData* link2=GetLink(inst);

        if (link2!=NULL)
            SetName(link2);
    }
}

//------------------------------------------------------------------------------
bool CUIEdProjectUI::IsParent(CIwAttrInstance* inst,CIwAttrInstance* parent)
{
    CUIEdAttrLinkData* link=GetLink(inst);
    CUIEdAttrLinkData* parentLink=GetLink(parent);

    if (link==NULL || parentLink==NULL) return false;

    for (; parentLink!=NULL; parentLink=parentLink->m_Parent)
    {
        if (link==parentLink)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::Add(CIwAttrInstance* inst,CIwAttrInstance* parent,int actionModifer)
{
    if (inst==NULL || parent==NULL) return NULL;

    CUIEdAttrLinkData* link=GetLink(inst);
    if (link==NULL) return NULL;

    if ((actionModifer&ACTIONMODIFIER_TAKEALL)==0)
        link=ResolveAddItem(link);

    CUIEdAttrLinkData* sibling=NULL;
    CUIEdAttrLinkData* parentLink=GetLink(parent);
    CUIEdAttrLinkData* layout=NULL;
    CUIEdAttrLinkData* layoutSibling=NULL;
    if (parentLink==NULL) return NULL;

    if ((actionModifer&ACTIONMODIFIER_INSERT) && parentLink->m_Parent!=NULL)
    {
        sibling=parentLink;
        parentLink=parentLink->m_Parent;
    }

    if (link->m_Type==LAYOUTITEM_FOCUS)
    {
        if (parentLink->m_Type!=LAYOUTITEM_ELEMENT)
            parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT);

        if (parentLink==NULL)
            return NULL;

        return CopyFocus(link,parentLink);
    }

    if (link->m_Type==LAYOUTITEM_PROPERTYSET)
    {
        if (parentLink->m_Type!=LAYOUTITEM_ELEMENT)
            parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT,&sibling);

        if (parentLink==NULL)
            return NULL;

        if (parentLink->m_Element->m_Class->GetMember(link->m_Element->m_Parent->m_Member->m_Name)==NULL)
            return NULL;

        link=Copy(link,parentLink,sibling,actionModifer);
        if (link==NULL) return NULL;

        AddToElement(link,parentLink,sibling);
        return link->m_Element;
    }

    if (parentLink->m_Type==LAYOUTITEM_FOCUS || parentLink->m_Type==LAYOUTITEM_PROPERTYSET)
        parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT,&sibling);

    if (parentLink->m_Type==LAYOUTITEM_LAYOUTITEM)
        parentLink=parentLink->GetParent(LAYOUTITEM_LAYOUT,&sibling);

    if (link->m_Type==LAYOUTITEM_ELEMENT)
    {
        if (parentLink->m_Type==LAYOUTITEM_LAYOUT)
        {
            layout=parentLink;
            layoutSibling=sibling;
            parentLink=layout->GetParent(LAYOUTITEM_ELEMENT,&sibling);
        }
        else if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
            layout=parentLink->GetLayout();

        if (layout!=NULL)
            link=Copy(link,layout,layoutSibling,actionModifer);
        else
            link=Copy(link,parentLink,sibling,actionModifer);

        if (link==NULL) return NULL;

        if ((actionModifer&ACTIONMODIFIER_KEEPNAME)==0)
            SetName(link);

        AddToElement(link,parentLink,sibling);

        if (layout!=NULL)
            AddToLayout(link,layout,layoutSibling);

        return link->m_Element;
    }

    if (link->m_Type==LAYOUTITEM_LAYOUT)
    {
        bool replaceLayout=false;

        if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
        {
            if (parentLink->GetLayout()==NULL)
                replaceLayout=true;
            else
                parentLink=parentLink->GetLayout();

            sibling=NULL;
        }

        layout=link;

        if (replaceLayout)
            link=CopyLayout(link,parentLink);
        else
            link=Copy(link,parentLink,sibling,actionModifer);

        if (link==NULL) return NULL;

        if ((actionModifer&ACTIONMODIFIER_KEEPNAME)==0)
            SetName(link);

        if (!replaceLayout)
        {
            AddToLayout(link,parentLink,sibling);
            Add2(link->m_Element,link->m_LayoutItem);
        }

        CopyLayoutChildren(link,layout,actionModifer);
        return link->m_Element;
    }

    if (link->m_Type==LAYOUTITEM_LAYOUTITEM)
    {
        if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
        {
            if (parentLink->GetLayout()==NULL)
                parentLink=parentLink->GetParent(LAYOUTITEM_LAYOUT,&sibling);
            else
            {
                parentLink=parentLink->GetLayout();
                sibling=NULL;
            }
        }

        if (parentLink==NULL)
            return NULL;

        link=Copy(link,parentLink,sibling,actionModifer);
        if (link==NULL) return NULL;

        if ((actionModifer&ACTIONMODIFIER_KEEPNAME)==0)
            SetName(link);

        AddToLayout(link,parentLink,sibling);
        Add2(link->m_Element,link->m_LayoutItem);

        return link->m_Element;
    }

    return NULL;
}
//------------------------------------------------------------------------------
void CUIEdProjectUI::MoveLink(CUIEdAttrLinkData* link,CUIEdAttrLinkData* layout,CUIEdAttrLinkData* sibling)
{
    int i;

    if (layout==NULL)
        return;

    if (sibling!=NULL)
    {
        for (i=0; i<(int)layout->m_Children.size(); i++)
        {
            if (layout->m_Children[i]==sibling)
            {
                layout->m_Children.insert(layout->m_Children.begin()+i,link);
                break;
            }
        }
        if (i==(int)layout->m_Children.size())
            layout->m_Children.push_back(link);
    }
    else
        layout->m_Children.push_back(link);

    link->m_Parent=layout;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdProjectUI::Move(CIwAttrInstance* inst,CIwAttrInstance* parent,int actionModifer)
{
    int i;
    CUIEdAttrShared& shared=CIwTheHost.m_Shared;
    CUIEdAttrLinkData* link=GetLink(inst);
    CUIEdAttrLinkData* parentLink=GetLink(parent);
    CUIEdAttrLinkData* sibling=NULL;
    CUIEdAttrLinkData* layout=NULL;
    CUIEdAttrLinkData* layoutSibling=NULL;
    if (link==NULL)
        return NULL;

    if (link->m_Type==LAYOUTITEM_PROPERTYSET)
        if (parentLink==NULL || link->m_Element->m_Parent==NULL || parentLink->m_Element->m_Class->GetMember(link->m_Element->m_Parent->m_Member->m_Name)==NULL)
            return NULL;

    if (link->m_Parent!=NULL)
    {
        for (i=0; i<(int)link->m_Parent->m_Children.size(); i++)
        {
            if (link->m_Parent->m_Children[i]==link)
            {
                link->m_Parent->m_Children.erase(link->m_Parent->m_Children.begin()+i);
                break;
            }
        }
        if (link->m_LayoutItem!=NULL && link->m_Element->m_Parent!=NULL && link->m_Element->m_Parent->m_Instance==link->m_LayoutItem)
            shared.Remove(link->m_LayoutItem);
        else
        {
            if (link->m_LayoutItem!=NULL)
                shared.Remove(link->m_LayoutItem);

            shared.Remove(link->m_Element);
        }
    }

    if (actionModifer&ACTIONMODIFIER_INSERT)
    {
        sibling=parentLink;
        parentLink=parentLink->m_Parent;
    }

    if (link->m_Type==LAYOUTITEM_FOCUS)
    {
        if (parentLink->m_Type!=LAYOUTITEM_ELEMENT)
            parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT);

        if (parentLink==NULL)
            return NULL;

        layout=parentLink->GetFocus();
        if (layout!=NULL)
            Delete(layout->m_Element);

        MoveLink(link,parentLink,sibling);
        AddToElement(link,parentLink,sibling);
        return link->m_Element;
    }

    if (link->m_Type==LAYOUTITEM_PROPERTYSET)
    {
        if (parentLink->m_Type!=LAYOUTITEM_ELEMENT)
            parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT,&sibling);

        if (parentLink==NULL)
            return NULL;

        MoveLink(link,parentLink,sibling);
        AddToElement(link,parentLink,sibling);
        return link->m_Element;
    }

    if (parentLink->m_Type==LAYOUTITEM_FOCUS || parentLink->m_Type==LAYOUTITEM_PROPERTYSET)
        parentLink=parentLink->GetParent(LAYOUTITEM_ELEMENT,&sibling);

    if (parentLink->m_Type==LAYOUTITEM_LAYOUTITEM)
        parentLink=parentLink->GetParent(LAYOUTITEM_LAYOUT,&sibling);

    if (link->m_Type==LAYOUTITEM_ELEMENT)
    {
        if (parentLink->m_Type==LAYOUTITEM_LAYOUT)
        {
            layout=parentLink;
            layoutSibling=sibling;
            parentLink=layout->GetParent(LAYOUTITEM_ELEMENT,&sibling);
        }
        else if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
            layout=parentLink->GetLayout();

        if (layout!=NULL)
            MoveLink(link,layout,layoutSibling);
        else
            MoveLink(link,parentLink,sibling);

        AddToElement(link,parentLink,sibling);

        if (layout!=NULL)
        {
            if (link->m_LayoutItem==NULL)
                AddToLayout(link,layout,layoutSibling);
            else
                Add2(link->m_LayoutItem,layout->m_Element,layoutSibling!=NULL ? layoutSibling->m_Element : NULL);
        }

        return link->m_Element;
    }

    if (link->m_Type==LAYOUTITEM_LAYOUT)
    {
        if (parentLink->m_Type==LAYOUTITEM_LAYOUT)
        {
            layout=parentLink;
            layoutSibling=sibling;
            parentLink=layout->GetParent(LAYOUTITEM_ELEMENT,&sibling);
        }
        else if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
        {
            layout=parentLink->GetLayout();
            if (layout==NULL)
            {
                layout=parentLink;
                layoutSibling=sibling;
            }
        }

        if (layout==NULL)
            return NULL;

        MoveLink(link,layout,layoutSibling);

        if (layout->m_Type==LAYOUTITEM_LAYOUT)
        {
            if (link->m_LayoutItem==NULL)
            {
                AddToLayout(link,layout,layoutSibling);
                Add2(link->m_Element,link->m_LayoutItem);
            }
            else
                Add2(link->m_LayoutItem,layout->m_Element,layoutSibling!=NULL ? layoutSibling->m_Element : NULL);
        }
        else
            AddToElement(link,parentLink,sibling);

        for (i=0; i<(int)link->m_Children.size(); i++)
        {
            if (link->m_Children[i]->m_Type==LAYOUTITEM_ELEMENT)
            {
                shared.Remove(link->m_Children[i]->m_Element);
                AddToElement(link->m_Children[i],parentLink,sibling);
            }
        }

        return link->m_Element;
    }

    if (link->m_Type==LAYOUTITEM_LAYOUTITEM)
    {
        if (parentLink->m_Type==LAYOUTITEM_ELEMENT)
        {
            if (parentLink->GetLayout()==NULL)
                parentLink=parentLink->GetParent(LAYOUTITEM_LAYOUT,&sibling);
            else
            {
                parentLink=parentLink->GetLayout();
                sibling=NULL;
            }
        }

        if (parentLink==NULL)
            return NULL;

        MoveLink(link,parentLink,sibling);

        AddToLayout(link,parentLink,sibling);
        Add2(link->m_Element,link->m_LayoutItem);

        return link->m_Element;
    }

    return inst;
}

//------------------------------------------------------------------------------
CUIEdAttrLinkData* CUIEdProjectUI::GetLink(CIwAttrInstance* inst)
{
    CUIEdAttrLinkData* link=(CUIEdAttrLinkData*)inst->m_TempClass;
    CUIEdAttrLinkData* link2=NULL;

    if (link==NULL && inst->m_Parent!=NULL)
    {
        link2=(CUIEdAttrLinkData*)inst->m_Parent->m_Instance->m_TempClass;
        if (link2!=NULL)
        {
            for (int i=0; i<(int)link2->m_Children.size(); i++)
            {
                if (link2->m_Children[i]->m_Element==inst)
                {
                    link=link2->m_Children[i];
                    break;
                }
            }
        }
    }

    return link;
}

//------------------------------------------------------------------------------
void CUIEdProjectUI::Delete(CIwAttrInstance*& inst,bool deleteLinked)
{
    if (!inst)
        return;

    int i;
    CUIEdAttrShared& shared=CIwTheHost.m_Shared;
    CUIEdAttrLinkData* link=GetLink(inst);
    CUIEdAttrLinkData* link2=NULL;

    CIwAttrInstance* parent=shared.Remove(inst);

    CIwAttrInstance* deadInst = inst;

    if (link!=NULL)
    {
        if (link->m_Element==inst)
        {
            link->m_Element=NULL;
            if (link->m_LayoutItem!=NULL)
            {
                if (deleteLinked)
                    Delete(link->m_LayoutItem,false);
                else
                {
                    link->m_LayoutItem->m_TempClass=NULL;
                    link->m_LayoutItem=NULL;
                }
            }
        }

        if (link->m_LayoutItem==inst)
        {
            link->m_LayoutItem=NULL;
            if (link->m_Element!=NULL)
            {
                if (deleteLinked)
                    Delete(link->m_Element,false);
                else
                {
                    link->m_Element->m_TempClass=NULL;
                    link->m_Element=NULL;
                }
            }
        }

        if (link->m_Element==NULL && link->m_LayoutItem==NULL && deleteLinked)
        {
            if (link->m_Parent)
            {
                for (i=0; i<(int)link->m_Parent->m_Children.size(); i++)
                {
                    if (link->m_Parent->m_Children[i]==link)
                    {
                        link->m_Parent->m_Children.erase(link->m_Parent->m_Children.begin()+i);
                        break;
                    }
                }
            }

            link->m_Children.clear();
            delete link;
        }
    }

    if (inst)
    {
        for (i=0; i<(int)inst->m_Data.size(); i++)
        {
            for (int j=0; i<(int)inst->m_Data.size() && j<(int)inst->m_Data[i]->m_Items.size(); )
            {
                if (inst->m_Data[i]->m_Items[j].m_Inst!=NULL && inst->m_Data[i]->m_Items[j].m_Flags&ATTRITEM_ALLOCED_F)
                    Delete(inst->m_Data[i]->m_Items[j].m_Inst);
                else
                    j++;
            }
        }
    }

    if (shared.m_SelElem==deadInst)
        shared.m_SelElem=parent;

    for (i=0; i<(int)shared.m_SelList.size(); i++)
    {
        if (shared.m_SelList[i]==deadInst)
        {
            shared.m_SelList.erase(shared.m_SelList.begin()+i);
            shared.m_SelList.push_back(parent);
            break;
        }
    }

    if (shared.m_UIEdAttrUIPanel!=NULL && shared.m_UIEdAttrUIPanel->m_Sect==deadInst)
        shared.m_UIEdAttrUIPanel->m_Sect=parent;

    if (shared.m_UIEdAttrStylePanel!=NULL && shared.m_UIEdAttrStylePanel->m_Sect==deadInst)
        shared.m_UIEdAttrStylePanel->m_Sect=parent;

    if (shared.m_UIEdAttrMaterialPanel!=NULL && shared.m_UIEdAttrMaterialPanel->m_Sect==deadInst)
        shared.m_UIEdAttrMaterialPanel->m_Sect=parent;

    if (inst)
    {
        CIwAttrData* name=inst->FindData(L"name",0);
        if (name!=NULL)
        {
            std::map<wxString,CUIEdAttrPropSet>::iterator it=shared.m_PropSetDict.find(name->m_Items[0].m_String);
            if (it!=shared.m_PropSetDict.end())
                shared.m_PropSetDict.erase(it);

            it=shared.m_MaterialDict.find(name->m_Items[0].m_String);
            if (it!=shared.m_MaterialDict.end())
                shared.m_MaterialDict.erase(it);
        }
    }

    SetChanged(true);

    delete inst;

    // Ensure we're not holding onto bad pointers
    inst = NULL;
}

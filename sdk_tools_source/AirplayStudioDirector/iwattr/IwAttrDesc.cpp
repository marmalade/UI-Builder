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
#include "IwAttrDesc.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK,__FILE__, __LINE__)
#define new DEBUG_NEW
#else
#define DEBUG_NEW new
#endif

//-----------------------------------------------------------------------------
IIwAttrDataEditor::IIwAttrDataEditor(CIwAttrData* pData) :
    m_Data(pData), m_NextEditor(NULL)
{
    if (pData)
    {
        m_NextEditor = pData->m_Editor;
        pData->m_Editor = this;
    }
}

IIwAttrDataEditor::~IIwAttrDataEditor()
{
    if (m_Data)
    {
        IIwAttrDataEditor* pPrevEditor = NULL;
        IIwAttrDataEditor* pEditor = m_Data->m_Editor;
        while (pEditor)
        {
            if (pEditor == this)
            {
                if (pPrevEditor)
                    pPrevEditor->m_NextEditor = m_NextEditor;
                else
                    m_Data->m_Editor = m_NextEditor;

                break;
            }
            else
                pEditor = pEditor->m_NextEditor;
        }
    }
}

void IIwAttrDataEditor::ClearData(CIwAttrData* pData)
{
    IIwAttrDataEditor* pEditor = this;
    while (pEditor)
    {
        IIwAttrDataEditor* pNextEditor = pEditor->m_NextEditor;

        wxASSERT(pEditor->m_Data == pData);
        pEditor->m_Data = NULL;
        pEditor->m_NextEditor = NULL;

        pEditor = pNextEditor;
    }
}

//-----------------------------------------------------------------------------
wxString CleanFloat(float num,int precis=-1)
{
    wxString format=wxT("%f");
    if (precis!=-1)
        format=wxString::Format(L"%%.%df",precis);

    wxString str_num=wxString::Format(format,num);

    int l = str_num.Length();
    int max_non_0 = 0;
    bool found_decimal = false;
    for (int c=0; c<l; ++c)
    {
        if (str_num[c]=='.')
            found_decimal = true;
        else
        if ((!found_decimal) || str_num[c]!='0')
            max_non_0 = c+1;
    }
    wxString short_str = ( found_decimal ? str_num.Left(max_non_0) : str_num );
    return (short_str == L"-0") ? L"0" : short_str;
}


//-----------------------------------------------------------------------------
//some members do not have default data
bool HasNoMemberData(int Type)
{
    if (Type&ATTRMEMBER_LIST)
        return false;

    switch (Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_ENUM:
    case ATTRMEMBER_CLASS:
    case ATTRMEMBER_PTR:
    case ATTRMEMBER_GROUP:
    case ATTRMEMBER_TEMPLATE:
    case ATTRMEMBER_CHILD:
    case ATTRMEMBER_RESOURCE:
        return true;
    }
    return false;
}

bool CIwAttrDescMgr::s_LiveEditing=false;

//-----------------------------------------------------------------------------
CIwAttrClass::~CIwAttrClass()
{
    for (int i=0; i<(int)m_Members.size(); i++)
    {
        delete m_Members[i];
    }
}

//-----------------------------------------------------------------------------
CIwAttrDesc::~CIwAttrDesc()
{
    Clear();
}
//-----------------------------------------------------------------------------
//version of GetMember for members of groups
bool CIwAttrClass::IsTemplateMember(CIwAttrMember* member)
{
    if ((member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE && member->m_Items[0].m_Class!=NULL)
        return true;

    if ((member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_DYNTEMPLATE && member->m_Items[0].m_Class!=this)
        return true;

    if ((member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_EXTRACLASS && member->m_Items[0].m_Class!=this)
        return true;  //if((member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && member->m_Items[0].m_Class!=NULL)

    return false;
}

//-----------------------------------------------------------------------------
//version of GetMember for members of groups
CIwAttrMember* CIwAttrClass::GetGroupMember(const wxString& Name,CIwAttrMember* Group,CIwAttrMember** RetGroup)
{
    if ((Group->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_GROUP)
        return NULL;

    for (int i=0; i<(int)Group->m_Items.size(); i++)
    {
        if ((Group->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            continue;

        if (Group->m_Items[i].m_Member->m_Name.IsSameAs(Name,false))
            return Group->m_Items[i].m_Member;

        for (int j=0; j<(int)Group->m_Items[i].m_Member->m_AltNames.size(); j++)
        {
            if (Group->m_Items[i].m_Member->m_AltNames[j].IsSameAs(Name,false))
                return Group->m_Items[i].m_Member;
        }

        if (IsTemplateMember(Group->m_Items[i].m_Member))
        {
            CIwAttrMember* Member=Group->m_Items[i].m_Member->m_Items[0].m_Class->GetMember(Name,RetGroup);
            if (Member!=NULL)
                return Member;
        }

        if ((Group->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupMember(Name,Group->m_Items[i].m_Member,RetGroup);
            if (Member!=NULL)
                return Member;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//recursively finds a member called name in the class hierarchy, retgroup is the group this member is in (if it is)
CIwAttrMember* CIwAttrClass::GetMember(const wxString& Name,CIwAttrMember** RetGroup)
{
    for (int i=0; i<(int)m_Members.size(); i++)
    {
        if ((m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            continue;

        if (m_Members[i]->m_Name.IsSameAs(Name,false))
            return m_Members[i];

        for (int j=0; j<(int)m_Members[i]->m_AltNames.size(); j++)
        {
            if (m_Members[i]->m_AltNames[j].IsSameAs(Name,false))
                return m_Members[i];
        }

        if (IsTemplateMember(m_Members[i]))
        {
            CIwAttrMember* Member=m_Members[i]->m_Items[0].m_Class->GetMember(Name,RetGroup);
            if (Member!=NULL)
                return Member;
        }

        if ((m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupMember(Name,m_Members[i],RetGroup);
            if (Member!=NULL)
            {
                if (RetGroup!=NULL) (*RetGroup)=m_Members[i];

                return Member;
            }
        }
    }
    if (m_Parent!=NULL)
        return m_Parent->GetMember(Name,RetGroup);
    else
        return NULL;
}

//-----------------------------------------------------------------------------
//version of GetMember for members of groups
CIwAttrMember* CIwAttrClass::GetGroupMember(unsigned int Mask,CIwAttrMember* Group,CIwAttrMember** RetGroup)
{
    if ((Group->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_GROUP)
        return NULL;

    for (int i=0; i<(int)Group->m_Items.size(); i++)
    {
        if ((Group->m_Items[i].m_Member->m_Type&Mask)==Mask)
            return Group->m_Items[i].m_Member;

        if (IsTemplateMember(Group->m_Items[i].m_Member))
        {
            CIwAttrMember* Member=Group->m_Items[i].m_Member->m_Items[0].m_Class->GetMember(Mask,RetGroup);
            if (Member!=NULL)
                return Member;
        }

        if ((Group->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupMember(Mask,Group->m_Items[i].m_Member,RetGroup);
            if (Member!=NULL)
                return Member;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//recursively finds the first member in the class hierarchy with a type that is in mask
// retgroup is the group this member is in (if it is)
CIwAttrMember* CIwAttrClass::GetMember(unsigned int Mask,CIwAttrMember** RetGroup)
{
    for (int i=0; i<(int)m_Members.size(); i++)
    {
        if ((m_Members[i]->m_Type&Mask)==Mask)
            return m_Members[i];

        if (IsTemplateMember(m_Members[i]))
        {
            CIwAttrMember* Member=m_Members[i]->m_Items[0].m_Class->GetMember(Mask);
            if (Member!=NULL)
                return Member;
        }

        if ((m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupMember(Mask,m_Members[i],RetGroup);
            if (Member!=NULL)
            {
                if (RetGroup!=NULL) (*RetGroup)=m_Members[i];

                return Member;
            }
        }
    }
    if (m_Parent!=NULL)
        return m_Parent->GetMember(Mask,RetGroup);
    else
        return NULL;
}

//-----------------------------------------------------------------------------
//version of GetClassMember for members of groups
CIwAttrMember* CIwAttrClass::GetGroupClassMember(CIwAttrClass* Class,CIwAttrMember* Group,CIwAttrMember** RetGroup)
{
    if (Group->m_Type!=ATTRMEMBER_GROUP)
        return NULL;

    for (int i=0; i<(int)Group->m_Items.size(); i++)
    {
        if (Group->m_Items[i].m_Member->m_Type==ATTRMEMBER_CHILD)
        {
            for (CIwAttrClass* Class2=Class; Class2!=NULL; Class2=Class2->m_Parent)
            {
                for (int j=0; j<(int)Group->m_Items[i].m_Member->m_Items.size(); j++)
                {
                    if (Group->m_Items[i].m_Member->m_Items[j].m_Class==Class2)
                        return Group->m_Items[i].m_Member;
                }
            }
        }

        if (IsTemplateMember(Group->m_Items[i].m_Member))
        {
            CIwAttrMember* Member=Group->m_Items[i].m_Member->m_Items[0].m_Class->GetClassMember(Class,RetGroup);
            if (Member!=NULL)
                return Member;
        }

        if ((Group->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupClassMember(Class,Group->m_Items[i].m_Member,RetGroup);
            if (Member!=NULL)
            {
                if (RetGroup!=NULL) (*RetGroup)=m_Members[i];

                return Member;
            }
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//recursively finds the first member in the class hierarchy that is a (or is a derivitive) of Class
//retgroup is the group this member is in (if it is)
CIwAttrMember* CIwAttrClass::GetClassMember(CIwAttrClass* Class,CIwAttrMember** RetGroup)
{
    for (int i=0; i<(int)m_Members.size(); i++)
    {
        if ((m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD || (m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
        {
            for (CIwAttrClass* Class2=Class; Class2!=NULL; Class2=Class2->m_Parent)
            {
                for (int j=0; j<(int)m_Members[i]->m_Items.size(); j++)
                {
                    if (m_Members[i]->m_Items[j].m_Class==Class2)
                        return m_Members[i];
                }
            }
        }

        if (IsTemplateMember(m_Members[i]))
        {
            CIwAttrMember* Member=m_Members[i]->m_Items[0].m_Class->GetClassMember(Class,RetGroup);
            if (Member!=NULL)
                return Member;
        }

        if ((m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            CIwAttrMember* Member=GetGroupClassMember(Class,m_Members[i],RetGroup);
            if (Member!=NULL)
            {
                if (RetGroup!=NULL) (*RetGroup)=m_Members[i];

                return Member;
            }
        }
    }
    if (m_Parent!=NULL)
        return m_Parent->GetClassMember(Class);
    else
        return NULL;
}

//-----------------------------------------------------------------------------
bool CIwAttrInstance::SaveExtra(wxTextFile& fp,bool addClassRoot,bool valid)
{
    int indent=0;
    wxString outstr;

    if (addClassRoot)
    {
        outstr=m_Class->m_Name+L"\n{\n";
        indent++;
    }

    outstr+=WriteNotes(indent,NULL,false,false,valid);
    outstr+=WriteExtraNotes(indent);

    if (addClassRoot)
        outstr+=L"\n}\n";

    std::vector<wxString> lines;
    Split(outstr,lines,L"\n\r");
    for (int i=0; i<(int)lines.size(); i++)
    {
        fp.AddLine(lines[i]);
    }

    fp.Write();
    return true;
}

//-----------------------------------------------------------------------------
//set the member to defualts
void CIwAttrMember::SetupDefault()
{
    m_ExportType=EXPORTTYPE_NUM;
    m_UseClassInAdd=true;

    if (HasNoMemberData(m_Type))
        return;

    m_Items.resize(m_ArraySize);

    for (int i=0; i<m_ArraySize; i++)
    {
        switch (m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_BOOL:
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
        case ATTRMEMBER_INT124:
        case ATTRMEMBER_STRINGID:
        case ATTRMEMBER_ALIGNMENT:
        case ATTRMEMBER_COLOUR:
        case ATTRMEMBER_COLOUR3:
            m_Items[i].m_Int=0;
            m_Items[i].m_UInt=0;
            break;

        case ATTRMEMBER_FLOAT:
            m_Items[i].m_Float=0;
            break;
        case ATTRMEMBER_STRING:
        case ATTRMEMBER_USETEMPLATE:
        case ATTRMEMBER_DATA:
        case ATTRMEMBER_FILE:
        case ATTRMEMBER_FILETEXTURE:
        case ATTRMEMBER_FILEFONT:
            m_Items[i].m_String.clear();
            break;
        case ATTRMEMBER_DYNTEMPLATE:
        case ATTRMEMBER_EXTRACLASS:
            m_Items[i].m_Class=m_Parent;
            break;

        case ATTRMEMBER_COORD:
            m_Items[i].m_Coord.m_UseInt=true;
            m_Items[i].m_Coord.m_Int=0;
            break;
        }
    }
}

//-----------------------------------------------------------------------------
CIwAttrMember::~CIwAttrMember()
{
    int i;
    for (i=0; i<(int)m_Items.size(); i++)
    {
        if (m_Items[i].m_Inst!=NULL)
            delete m_Items[i].m_Inst;
    }

    if ((m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_GROUP)
        return;

    for (i=0; i<(int)m_Items.size(); i++)
    {
        delete m_Items[i].m_Member;
    }
}

static wxString exportTypeMap[]={   //MUST CHANGE CIwAttrExportType if you change this
    wxT("world"),
    wxT("material"),
    wxT("selset"),
    wxT("mesh"),
    wxT("camera"),
    wxT("joint"),
    wxT("curve"),
    wxT("gui"),
    wxT("lodgroup"),
    wxT("gfx"),
    wxT("group"),
    wxT("template"),
    wxT("transform"),
    wxT("timeline"),
    wxT("worldImport"),
    wxT(""),
};

//-----------------------------------------------------------------------------
CIwAttrExportType GetExportType(const wxString& type)
{
    for (int i=0; i<(int)EXPORTTYPE_NUM; i++)
    {
        if (exportTypeMap[i].IsSameAs(type,false))
            return (CIwAttrExportType)i;
    }

    return EXPORTTYPE_NUM;
}
//-----------------------------------------------------------------------------
//read a line of an attribute description file
void CIwAttrTimeline::ReadTimelineLine(const std::vector<wxString>& argv)
{
    if (argv[1].IsSameAs(wxT("create"),false)) //timeline create start|keys|visible [first|hierroot|anim|camera|<class>]*
    {
        m_Flags&=~TIMELINEFLAGS_CREATEMASK;
        if (argv[2].IsSameAs(wxT("start"),false))
            m_Flags|=TIMELINEFLAGS_CREATESTART;
        else if (argv[2].IsSameAs(wxT("keys"),false))
            m_Flags|=TIMELINEFLAGS_CREATEKEYS;
        else if (argv[2].IsSameAs(wxT("visible"),false))
            m_Flags|=TIMELINEFLAGS_CREATEVISIBLE;

        for (int i=3; i<(int)argv.size(); i++)
        {
            if (argv[i].IsSameAs(wxT("first"),false))
                m_Flags|=TIMELINEFLAGS_CREATEFIRSTONLY;
            else if (argv[i].IsSameAs(wxT("hierroot"),false))
                m_Flags|=TIMELINEFLAGS_CREATEIFHIERROOT;
            else if (argv[i].IsSameAs(wxT("anim"),false))
                m_Flags|=TIMELINEFLAGS_CREATEIFANIM;
            else if (argv[i].IsSameAs(wxT("camera"),false))
                m_Flags|=TIMELINEFLAGS_CREATEIFCAMERA;
            else if (argv[i].IsSameAs(wxT("model"),false))
                m_Flags|=TIMELINEFLAGS_CREATEIFMODEL;
            else if (argv[i].size()<2)
            {
                m_Flags|=TIMELINEFLAGS_CREATEIFKEY;
                m_CreateClass=argv[i];
            }
            else
                break;
        }
    }
    else if (argv[1].IsSameAs(wxT("duration"),false))
    {
        m_DurItem=argv[2];
        if (argv.size()>=4)
            m_DurStart=argv[3];

        if (argv.size()>=5)
            m_DurEnd=argv[4];
    }
    else if (argv[1].IsSameAs(wxT("affects"),false))
    {
        for (int i=1; i<(int)argv.size(); i++)
        {
            if (argv[i].IsSameAs(wxT("anim"),false))
                m_Flags|=TIMELINEFLAGS_AFFECTSANIM;
            else if (argv[i].IsSameAs(wxT("world"),false))
                m_Flags|=TIMELINEFLAGS_AFFECTSWORLD;
        }
    }
    else if (argv[1].IsSameAs(wxT("priority"),false) && argv.size()>2)
    {
        if (m_Flags==0) // to help detection code:
            m_Flags=TIMELINEFLAGS_CREATEIFMODEL;

        m_Priority=atoi(argv[2].mb_str());
    }
}

//-----------------------------------------------------------------------------
bool CIwAttrTimeline::IsSameObject(CIwAttrInstance* a,CIwAttrInstance* b)
{
    CIwAttrData* aName=a->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
    CIwAttrData* bName=b->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);

    if (aName==NULL && bName==NULL)
        return true;

    if (aName!=NULL && bName!=NULL)
        return aName->m_Items[0].m_String.IsSameAs(bName->m_Items[0].m_String,false);

    aName=a->FindData(L"useGeo",CIwAttrInstance::FINDMODE_EXPORTERTAG);
    bName=b->FindData(L"useGeo",CIwAttrInstance::FINDMODE_EXPORTERTAG);

    if (aName!=NULL && bName!=NULL)
        return aName->m_Items[0].m_String.IsSameAs(bName->m_Items[0].m_String,false);

    return false;
}

//-----------------------------------------------------------------------------
CIwAttrData* CIwAttrTimeline::GetDurationDataItem(CIwAttrInstance* inst,float* rate)
{
    CIwAttrData* data=inst->FindData(m_DurItem,0);
    if (data==NULL) return NULL;

    float a=atoi(m_DurStart.mb_str());
    if (a==0) a=1;

    if (rate!=NULL) (*rate)=a;

    int start=-1,end=-1;
    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_BOOL)
        start=1;
    else if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_BOOL)
        start=1;
    else if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM)
    {
        for (int k=0; k<(int)data->m_Member->m_Items.size(); k++)
        {
            if (data->m_Member->m_Items[k].m_String.IsSameAs(m_DurStart,false))
                start=k;

            if (data->m_Member->m_Items[k].m_String.IsSameAs(m_DurEnd,false))
                end=k;
        }
    }

    if (start==-1 || end==-1)
        return data;

    return NULL;
}

//-----------------------------------------------------------------------------
int CIwAttrTimeline::GetDurationWidth(std::vector<CIwAttrInstance*>& list,int first,float from,float* width)
{
    int k;
    int next=-2;

    CIwAttrData* data=list[first]->FindData(m_DurItem,0);
    if (data!=NULL)
    {
        int start=-1,end=-1;
        if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_STRING)
        {
            for (k=first+1; k<(int)list.size(); k++)
            {
                if (IsSameObject(list[first],list[k]))
                {
                    CIwAttrData* data2=list[k]->FindData(m_DurItem,0);
                    CIwAttrData* frame2=list[k]->FindData(L"firstFrame",CIwAttrInstance::FINDMODE_EXPORTERTAG);

                    if (data2!=NULL && data->m_Items[0].m_String.empty()!=data2->m_Items[0].m_String.empty())
                    {
                        (*width)=frame2->m_Items[0].m_Float-from;
                        next=k;
                        break;
                    }
                }
            }
        }
        else if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_BOOL)
        {
            start=1;
            end=0;
        }
        else if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM)
        {
            for (k=0; k<(int)data->m_Member->m_Items.size(); k++)
            {
                if (data->m_Member->m_Items[k].m_String.IsSameAs(m_DurStart,false))
                    start=k;

                if (data->m_Member->m_Items[k].m_String.IsSameAs(m_DurEnd,false))
                    end=k;
            }
        }

        if (start==-1 || end==-1)
        {
            float rate=atoi(m_DurStart.mb_str());
            if (rate==0) rate=1;

            if (data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
                (*width)=(float)data->m_Items[0].m_UInt/rate;
            else if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_FLOAT)
                (*width)=(float)data->m_Items[0].m_Float/rate;
            else
                (*width)=(float)data->m_Items[0].m_Int/rate;

            next=-1;
        }
        else
        {
            if (data->m_Items[0].m_Int==start)
            {
                for (k=first+1; k<(int)list.size(); k++)
                {
                    if (IsSameObject(list[first],list[k]))
                    {
                        CIwAttrData* data=list[k]->FindData(m_DurItem,0);
                        CIwAttrData* frame2=list[k]->FindData(L"firstFrame",CIwAttrInstance::FINDMODE_EXPORTERTAG);

                        if (data!=NULL && data->m_Items[0].m_Int==end)
                        {
                            (*width)=frame2->m_Items[0].m_Float-from;
                            next=k;
                            break;
                        }
                    }
                }
            }
            else if (data->m_Items[0].m_Int==end)
            {
                for (k=first+1; k<(int)list.size(); k++)
                {
                    if (IsSameObject(list[first],list[k]))
                    {
                        CIwAttrData* data=list[k]->FindData(m_DurItem,0);
                        CIwAttrData* frame2=list[k]->FindData(L"firstFrame",CIwAttrInstance::FINDMODE_EXPORTERTAG);

                        if (data!=NULL && data->m_Items[0].m_Int==start)
                        {
                            (*width)=frame2->m_Items[0].m_Float-from;
                            next=k;
                            break;
                        }
                    }
                }
            }
        }
    }
    else
    {
        for (k=first+1; k<(int)list.size(); k++)
        {
            if (list[k]->m_TempClass->m_Name.IsSameAs(m_DurItem,false))
            {
                if (IsSameObject(list[first],list[k]))
                {
                    CIwAttrData* frame2=list[k]->FindData(L"firstFrame",CIwAttrInstance::FINDMODE_EXPORTERTAG);
                    if (frame2!=NULL)
                    {
                        (*width)=frame2->m_Items[0].m_Float-from;
                        next=k;
                        break;
                    }
                }
            }
        }
    }

    return next;
}

//-----------------------------------------------------------------------------
//read a line of an attribute description file
void CIwAttrDesc::ReadLine(const std::vector<wxString>& argv,const wxString& Dir)
{
    int i,Offset=1;

    if (argv[0]==wxT("{"))
        m_Indent++;
    else if (argv[0]==wxT("}"))
        m_Indent--;
    else if (argv[0].IsSameAs(wxT("exporttype"),false)) //exporttype <exporter_type> [<class>]*
    {
        if (argv.size()<2) return;

        CIwAttrExportType Type=GetExportType(argv[1]);
        if (Type==EXPORTTYPE_NUM)
            return;

        for (i=2; i<(int)argv.size(); i++)
        {
            if (argv[i][0]==wxT(';'))
                break;

            CIwAttrClass* Class=m_Mgr->GetClass(argv[i]);
            if (Class!=NULL)
                m_ExportTypes[Type].push_back(Class);
        }
        Offset=i;
    }
    else if (argv[0].IsSameAs(wxT("exportgroup"),false)) //exportgroup <group> [<exporter_type>]*
    {
        if (argv.size()<2) return;

        for (i=2; i<(int)argv.size(); i++)
        {
            CIwAttrExportType Type=GetExportType(argv[i]);
            if (Type==EXPORTTYPE_NUM)
                continue;

            if (m_FileExtMap.find(argv[1].Lower())==m_FileExtMap.end())
                m_FileExtMap[argv[1].Lower()]=CIwAttrDescFileExt();

            m_FileExtMap[argv[1].Lower()].m_Types.push_back(Type);
        }
        Offset=i;
    }
    else if (argv[0].IsSameAs(wxT("restrictoutput"),false)) //restrictoutput
        m_Restrict=true;
    else if (argv[0].IsSameAs(wxT("version"),false)) //restrictoutput
    {
        if (argv.size()<2) return;

        m_Version=argv[1];
    }
    else if (argv[0].IsSameAs(wxT("ignore"),false)) //ignore <note>
    {
        if (argv.size()<2) return;

        m_Mgr->m_InvalidExtraNotes.push_back(argv[1]);
    }
    else if (argv[0].IsSameAs(wxT("class"),false)) //class <name> [<parent>]
    {
        if (argv.size()<2) return;

        m_CurrClass=new CIwAttrClass;
        m_CurrClass->m_Name=argv[1];
        m_CurrGroup=NULL;
        m_CurrMember=NULL;

        if (argv.size()>2 && argv[2][0]!=wxT(';'))
        {
            m_CurrClass->m_Parent=m_Mgr->GetClass(argv[2]);
            Offset=3;
        }
        else
            Offset=2;

        m_Classes.push_back(m_CurrClass);
    }
    else if (argv[0].IsSameAs(wxT("alias"),false)) //alias <name> <parent>
    {
        if (argv.size()<3) return;

        CIwAttrClass* Class=new CIwAttrClass;
        Class->m_Name=argv[1];
        Class->m_Parent=m_Mgr->GetClass(argv[2]);

        m_Classes.push_back(Class);
    }
    else if (argv[0].IsSameAs(wxT("baseclass"),false)) //baseclass <name> [<parent>]
    {
        if (argv.size()<2) return;

        m_CurrClass=new CIwAttrClass;
        m_CurrClass->m_Name=argv[1];
        m_CurrClass->m_Flags|=ATTRCLASS_BASE_F;
        m_CurrGroup=NULL;
        m_CurrMember=NULL;

        if (argv.size()>2 && argv[2][0]!=wxT(';'))
        {
            m_CurrClass->m_Parent=m_Mgr->GetClass(argv[2]);
            Offset=3;
        }
        else
            Offset=2;

        m_Classes.push_back(m_CurrClass);
    }
    else if (argv[0].IsSameAs(wxT("template"),false)) //template <name>
    {
        if (argv.size()<2 || m_CurrClass==NULL) return;

        m_CurrMember=new CIwAttrMember;
        m_CurrMember->m_Type=ATTRMEMBER_TEMPLATE;
        m_CurrMember->m_Name=argv[1];
        m_CurrMember->m_Indent=m_Indent;
        m_CurrMember->m_Items.resize(1);
        m_CurrMember->m_Items[0].m_Class=m_Mgr->GetClass(argv[1]);
        m_CurrMember->m_ArraySize=1;
        m_CurrMember->m_Sticky=false;
        m_CurrMember->m_Parent=m_CurrClass;

        if (m_CurrGroup==NULL || m_CurrGroup->m_Indent>=m_Indent)
            m_CurrClass->m_Members.push_back(m_CurrMember);
        else
        {
            m_CurrGroup->m_Items.resize(m_CurrGroup->m_Items.size()+1);
            m_CurrGroup->m_Items.back().m_Member=m_CurrMember;
        }

        Offset=2;
    }
    else if (argv[0].IsSameAs(wxT("override"),false)) //override <class>|<member>
    {
        if (argv.size()<2) return;

        CIwAttrClass* Class=m_Mgr->GetClass(argv[1]);
        if (Class!=NULL)
        {
            m_CurrClass=Class;
            m_CurrMember=NULL;
        }
        else if (m_CurrClass!=NULL)
            m_CurrMember=m_CurrClass->GetMember(argv[1]);

        Offset=2;
    }
    else if (argv[0].IsSameAs(wxT("group"),false)) //group <name> 1|+|*
    {
        if (argv.size()<3 || m_CurrClass==NULL) return;

        m_CurrMember=m_CurrGroup=new CIwAttrMember;
        m_CurrMember->m_Name=argv[1];
        m_CurrMember->m_Type=ATTRMEMBER_GROUP;
        m_CurrMember->m_Indent=m_Indent;
        m_CurrClass->m_Members.push_back(m_CurrMember);
        m_CurrMember->m_ArraySize=0;
        m_CurrMember->m_Sticky=false;
        m_CurrMember->m_Parent=m_CurrClass;

        switch (argv[2][0])
        {
        case wxT('+'):
            m_CurrMember->m_Type|=ATTRMEMBER_0OR1;
            break;
        case wxT('*'):
            m_CurrMember->m_Type|=ATTRMEMBER_0ORMORE;
            break;
        }
        Offset=3;

        if ((int)argv.size()>Offset && argv[Offset][0]!=wxT(';'))
        {
            if (argv[Offset].IsSameAs(wxT("hidden"),false))
            {
                m_CurrMember->m_Type|=ATTRMEMBER_HIDDEN;
                Offset++;
            }
        }
    }
    else if (argv[0].IsSameAs(wxT("member"),false)) //member <name> <type_definition> [<default_value> [<min>[..<max>[,<step>]]]]
    {
        if (argv.size()<3 || m_CurrClass==NULL) return;

        m_CurrMember=new CIwAttrMember;
        m_CurrMember->m_Name=argv[1];
        m_CurrMember->m_Indent=m_Indent;
        m_CurrMember->m_ArraySize=1;
        m_CurrMember->m_Sticky=false;
        m_CurrMember->m_Parent=m_CurrClass;

        if (m_CurrGroup==NULL || m_CurrGroup->m_Indent>=m_Indent)
            m_CurrClass->m_Members.push_back(m_CurrMember);
        else
        {
            m_CurrGroup->m_Items.resize(m_CurrGroup->m_Items.size()+1);
            m_CurrGroup->m_Items.back().m_Member=m_CurrMember;
        }

        std::vector<wxString> argv2;
        for (i=2; i<(int)argv.size(); i++)
        {
            argv2.push_back(argv[i]);
        }

        Offset=2+GetType(argv2,m_CurrMember);

        argv2.clear();
        for (i=Offset; i<(int)argv.size(); i++)
        {
            argv2.push_back(argv[i]);
        }

        if ((int)argv.size()>Offset && argv[Offset][0]!=wxT(';'))
            for (int j=0; j<m_CurrMember->m_ArraySize; j++)
            {
                Offset+=GetDefault(argv2,m_CurrMember);
            }
        else
            m_CurrMember->SetupDefault();

        m_CurrMember->m_Limits[2]=0;
        if ((int)argv.size()>Offset && argv[Offset][0]!=wxT(';'))
        {
            char* Ptr;
            const wxCharBuffer cb = argv[Offset].mb_str();
            m_CurrMember->m_Limits[0]=strtol(cb,&Ptr,10);
            Offset++;

            if ((int)argv.size()>Offset && argv[Offset][0]!=wxT(';'))
            {
                char* Ptr;
                const wxCharBuffer cb = argv[Offset].mb_str();
                m_CurrMember->m_Limits[1]=strtol(cb,&Ptr,10);
                Offset++;

                if ((int)argv.size()>Offset && argv[Offset][0]!=wxT(';'))
                {
                    char* Ptr;
                    const wxCharBuffer cb = argv[Offset].mb_str();
                    m_CurrMember->m_Limits[2]=strtol(cb,&Ptr,10);
                    Offset++;
                }
                else
                    m_CurrMember->m_Limits[2]=1;
            }
            else
            {
                switch (m_CurrMember->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
                {
                case ATTRMEMBER_BYTE:
                    m_CurrMember->m_Limits[1]=0x7f; break;
                case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
                    m_CurrMember->m_Limits[1]=0xff; break;
                case ATTRMEMBER_SHORT:
                    m_CurrMember->m_Limits[1]=0x7fff; break;
                case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
                    m_CurrMember->m_Limits[1]=0xffff; break;
                case ATTRMEMBER_INT:
                    m_CurrMember->m_Limits[1]=0x7fffffff; break;
                case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
                    m_CurrMember->m_Limits[1]=0xffffffff; break;
                default:
                    m_CurrMember->m_Limits[1]=0x7fffffff; break;
                }
                m_CurrMember->m_Limits[2]=1;
            }
        }
    }
    else if (argv[0].IsSameAs(wxT("child"),false)) //child 1|+|* [<class>]*
    {
        if (argv.size()<3 || m_CurrClass==NULL) return;

        m_CurrMember=new CIwAttrMember;
        m_CurrMember->m_Name=argv[2];
        m_CurrMember->m_Type=ATTRMEMBER_CHILD;
        m_CurrMember->m_Indent=m_Indent;
        m_CurrMember->m_ArraySize=1;
        m_CurrMember->m_Sticky=false;
        m_CurrMember->m_Parent=m_CurrClass;

        if (m_CurrGroup==NULL || m_CurrGroup->m_Indent>=m_Indent)
            m_CurrClass->m_Members.push_back(m_CurrMember);
        else
        {
            m_CurrGroup->m_Items.resize(m_CurrGroup->m_Items.size()+1);
            m_CurrGroup->m_Items.back().m_Member=m_CurrMember;
        }

        switch (argv[1][0])
        {
        case wxT('+'):
            m_CurrMember->m_Type|=ATTRMEMBER_0OR1;
            break;
        case wxT('*'):
            m_CurrMember->m_Type|=ATTRMEMBER_0ORMORE;
            break;
        }
        for (Offset=2; Offset<(int)argv.size(); Offset++)
        {
            if (argv[Offset][0]==wxT(';'))
                break;
        }

        m_CurrMember->m_Items.resize(Offset-2);

        for (i=2; i<Offset; i++)
        {
            m_CurrMember->m_Items[i-2].m_Class=m_Mgr->GetClass(argv[i]);
        }
    }
    else if (argv[0].IsSameAs(wxT("option"),false)) //option <option>
    {
        if (argv.size()<2 || m_CurrClass==NULL) return;

        if (argv[1].IsSameAs(wxT("viewerready"),false))
            m_CurrClass->m_Flags|=ATTRCLASS_VIEWERREADY_F;

        if (argv[1].IsSameAs(wxT("viewergroup"),false))
            m_CurrClass->m_Flags|=ATTRCLASS_VIEWERGROUP_F;

        if (argv[1].IsSameAs(wxT("ordered"),false))
            m_CurrClass->m_Flags|=ATTRCLASS_ORDERED_F;

        if (argv[1].IsSameAs(wxT("nomesh"),false))
            m_CurrClass->m_Flags|=ATTRCLASS_NOMESH_F;
    }
    else if (argv[0].IsSameAs(wxT("info"),false)) //info <name> <path>
    {
        if (argv.size()<3 || m_CurrClass==NULL) return;

        m_CurrClass->m_Paths[argv[1]]=argv[2];
    }
    else if (argv[0].IsSameAs(wxT("timeline"),false)) //timeline ...
    {
        if (argv.size()<3 || m_CurrClass==NULL) return;

        m_CurrClass->m_TimeLine.ReadTimelineLine(argv);
    }
    else if (argv[0].IsSameAs(wxT("exporter"),false)) //exporter <export_type_def>
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        Offset=2;
        m_CurrMember->m_ExportSection=argv[1];
        if (argv.size()>2 && argv[2][0]!=wxT(';'))
        {
            Offset++;
            m_CurrMember->m_ExportOffset=atoi(argv[2].mb_str());
        }
        else
            m_CurrMember->m_ExportOffset=0;

        if (m_Mgr->m_IsExporter)
            m_CurrMember->m_Type|=ATTRMEMBER_READONLY;

        m_CurrMember->m_Type|=ATTRMEMBER_NODEFAULT;
    }
    else if (argv[0].IsSameAs(wxT("readonly"),false)) //readonly
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_READONLY;
    }
    else if (argv[0].IsSameAs(wxT("notinextraclass"),false)) //notinextraclass
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_NOTINEXTRACLASS;
    }
    else if (argv[0].IsSameAs(wxT("commented"),false)) //commented
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_COMMENTED;
    }
    else if (argv[0].IsSameAs(wxT("usedefault"),false)) //usedefault
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type&=~ATTRMEMBER_NODEFAULT;
    }
    else if (argv[0].IsSameAs(wxT("nodefault"),false)) //nodefault
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_NODEFAULT;
    }
    else if (argv[0].IsSameAs(wxT("readwrite"),false)) //readwrite
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type&=~ATTRMEMBER_READONLY;
        m_CurrMember->m_Type|=ATTRMEMBER_NULLABLE;
    }
    else if (argv[0].IsSameAs(wxT("readwritegroup"),false)) //readwritegroup
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type&=~ATTRMEMBER_READONLY;
        m_CurrMember->m_Type|=ATTRMEMBER_CHANGEABLE;
    }
    else if (argv[0].IsSameAs(wxT("extra"),false)) //extra <type>
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        Offset=2;
        m_CurrMember->m_ExtraSection=argv[1];
        m_CurrMember->m_Type|=ATTRMEMBER_SKIP|ATTRMEMBER_READONLY;
    }
    else if (argv[0].IsSameAs(wxT("fromChild"),false)) //fromChild
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_READONLY|ATTRMEMBER_FROMCHILD;
    }
    else if (argv[0].IsSameAs(wxT("skip"),false)) //skip
    {
        if (m_CurrMember==NULL) return;

        Offset=1;
        m_CurrMember->m_Type|=ATTRMEMBER_SKIP|ATTRMEMBER_READONLY;
        m_CurrMember->m_UseClassInAdd=false;
    }
    else if (argv[0].IsSameAs(wxT("ifx"),false)) //ifx <filename>
    {
        if (argv.size()<2) return;

        LoadIFX(wxString::Format(wxT("%s/%s"),Dir.c_str(),argv[1].c_str()),argv[1]);
    }
    else if (argv[0].IsSameAs(wxT("viewer"),false)) //viewer <type>
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        Offset=2;
        m_CurrMember->m_ViewerType=argv[1];
    }
    else if (argv[0].IsSameAs(wxT("premember"),false)) //premember <member>
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        Offset=2;
        m_CurrMember->m_PreMember=argv[1];
    }
    else if (argv[0].IsSameAs(wxT("attrEd"),false)) //attrEd <type>
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        Offset=2;
        m_CurrMember->m_AttrEd=argv[1];
    }
    else if (argv[0].IsSameAs(wxT("altname"),false)) //altname [<name>*]
    {
        if (argv.size()<2) return;

        if (m_CurrMember!=NULL)
        {
            for (Offset=1; Offset<(int)argv.size(); Offset++)
            {
                if (argv[Offset][0]==wxT(';'))
                    break;

                if ((int)m_CurrMember->m_Text.size()>Offset-1)
                    m_CurrMember->m_Text[Offset-1]=argv[Offset];
                else
                    m_CurrMember->m_AltNames.push_back(argv[Offset]);
            }
        }
        else if (m_CurrClass!=NULL)
        {
            for (Offset=1; Offset<(int)argv.size(); Offset++)
            {
                if (argv[Offset][0]==wxT(';'))
                    break;

                m_CurrClass->m_AltNames.push_back(argv[Offset]);
            }
        }
    }
    else if (argv[0].IsSameAs(wxT("wizard"),false)) //wizard [<routine> [<params>]*]
    {
        if (argv.size()<1 || m_CurrMember==NULL) return;

        for (Offset=1; Offset<(int)argv.size(); Offset++)
        {
            if (argv[Offset][0]==wxT(';'))
                break;

            m_CurrMember->m_Wizard.push_back(argv[Offset]);
        }
        if (m_CurrMember->m_Wizard.empty())
            m_CurrMember->m_Wizard.push_back(wxT("set"));
    }
    else if (argv[0].IsSameAs(wxT("text"),false)) //text <member_text> [<enum_text>]*
    {
        if (argv.size()<2 || m_CurrMember==NULL) return;

        for (Offset=1; Offset<(int)argv.size(); Offset++)
        {
            if (argv[Offset][0]==wxT(';'))
                break;

            if ((int)m_CurrMember->m_Text.size()>Offset-1)
                m_CurrMember->m_Text[Offset-1]=argv[Offset];
            else
                m_CurrMember->m_Text.push_back(argv[Offset]);
        }
    }
    else if (argv[0].IsSameAs(wxT("sticky"),false)) //sticky
    {
        if (m_CurrMember==NULL) return;

        m_CurrMember->m_Sticky=true;
    }
    else if (argv[0].IsSameAs(wxT("liveskip"),false)) //sticky
    {
        if (m_CurrMember==NULL) return;

        m_CurrMember->m_ViewerDead=true;
    }
    else if (argv[0].IsSameAs(wxT("priority"),false)) //priority <num>
    {
        if (m_CurrMember==NULL || argv.size()<2) return;

        m_CurrMember->m_Priority=atoi(argv[1].mb_str());
    }
    else if (argv[0].IsSameAs(wxT("include"),false)) //include <file_name>
    {
        if (argv.size()<2) return;

        Load(wxString::Format(wxT("%s/%s"),Dir.c_str(),argv[1].c_str()));
    }
    else if (argv[0].IsSameAs(wxT("restemplate"),false)) //restemplate <class> [<extension>]+
    {
        if (argv.size()<3) return;

        int j;
        for (j=0; j<(int)m_Classes.size(); j++)
        {
            if (m_Classes[j]->m_Name.IsSameAs(argv[1],false))
                break;
        }
        if (j==(int)m_Classes.size()) return;

        for (int i=2; i<(int)argv.size(); i++)
        {
            if (m_FileExtMap.find(argv[i].Lower())==m_FileExtMap.end())
                m_FileExtMap[argv[i].Lower()]=CIwAttrDescFileExt();

            m_FileExtMap[argv[i].Lower()].m_TemplateClass=m_Classes[j];
        }
    }
    else if (argv[0].IsSameAs(wxT("viewerfield"),false)) //viewerfield <primary> [<alternative>]*
    {
        if (argv.size()<2) return;

        CIwAttrViewerField* field=new CIwAttrViewerField;

        field->m_Primary=argv[1];
        for (int i=2; i<(int)argv.size(); i++)
        {
            if (argv[i].IsSameAs(wxT("!required"),false))
                field->m_Required=true;
            else
                field->m_Alternatives.push_back(argv[i]);
        }

        m_ViewerFields.push_back(field);

        FindFieldInClasses(field);
    }
    else if (argv[0].IsSameAs(wxT("filetype"),false)) //filetype <primary> <ext> [<alternative>]*
    {
        if (argv.size()<3) return;

        CIwAttrViewerField* field=new CIwAttrViewerField;

        field->m_Ext=argv[1];
        field->m_Primary=argv[2];
        for (int i=3; i<(int)argv.size(); i++)
        {
            if (argv[i].IsSameAs(wxT("!required"),false))
                field->m_Required=true;
            else
                field->m_Alternatives.push_back(argv[i]);
        }

        m_ViewerFields.push_back(field);

        FindFieldInClasses(field);
    }
    else if (argv[0].IsSameAs(wxT("classselector"),false))
    {
        if (m_CurrMember==NULL) return;

        for (i=1; i<(int)argv.size() && i-1<(int)m_CurrMember->m_Items.size(); i++)
        {
            m_CurrMember->m_Items[i-1].m_Class=m_Mgr->GetClass(argv[i]);
        }
    }

    //read the rest of the line as a sub line
    if ((int)argv.size()>Offset+1 && argv[Offset][0]==wxT(';'))
    {
        m_Indent++;

        std::vector<wxString> argv2;
        for (i=Offset+1; i<(int)argv.size(); i++)
        {
            argv2.push_back(argv[i]);
        }

        ReadLine(argv2,Dir);

        m_Indent--;
    }
}

//-----------------------------------------------------------------------------
void CIwAttrDesc::LoadIFX(const wxString& fileName,const wxString& shortName)
{
    wxTextFile fp(fileName);
    if (!fp.Exists() || !fp.Open())
        return;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> args;

        if (SuperSplit(fp[i],args,L" \t\n\r")<2)
            continue;

        if (args[0].IsSameAs(L"name",true))
        {
            m_CurrClass=new CIwAttrClass;
            m_CurrClass->m_Name=args[1];
            m_CurrGroup=NULL;
            m_CurrMember=NULL;
            m_CurrClass->m_Parent=m_Mgr->GetClass(L"CIwFxParameters");
            m_CurrClass->m_Paths[L"ifx"]=shortName;

            m_Classes.push_back(m_CurrClass);
        }
        else if (args[0].IsSameAs(L"param",true))
        {
            if (args.size()<3) continue;

            m_CurrMember=new CIwAttrMember;
            m_CurrMember->m_Name=args[1];
            m_CurrMember->m_Indent=m_Indent;
            m_CurrMember->m_Sticky=false;
            m_CurrMember->m_Parent=m_CurrClass;

            if (args[2].IsSameAs(L"float"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_FLOAT;
                m_CurrMember->m_ArraySize=1;
            }
            else if (args[2].IsSameAs(L"float2"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_FLOAT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=2;
            }
            else if (args[2].IsSameAs(L"float3"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_FLOAT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=3;
            }
            else if (args[2].IsSameAs(L"float4"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_FLOAT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=4;
            }
            else if (args[2].IsSameAs(L"int"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_INT;
                m_CurrMember->m_ArraySize=1;
            }
            else if (args[2].IsSameAs(L"int2"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_INT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=2;
            }
            else if (args[2].IsSameAs(L"int3"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_INT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=3;
            }
            else if (args[2].IsSameAs(L"int4"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_INT|ATTRMEMBER_ARRAY;
                m_CurrMember->m_ArraySize=4;
            }
            else if (args[2].IsSameAs(L"sampler"))
            {
                m_CurrMember->m_Type=ATTRMEMBER_FILETEXTURE;
                m_CurrMember->m_ArraySize=1;
            }

            m_CurrMember->SetupDefault();

            m_CurrClass->m_Members.push_back(m_CurrMember);
        }
        else if (args[0].IsSameAs(L"//:",true) && args[1].IsSameAs(L"path",true))
        {
            if (args.size()<4) continue;

            m_CurrClass->m_Paths[args[2]]=args[3];
        }
    }
}

//-----------------------------------------------------------------------------
CIwAttrClass* CIwAttrDescMgr::GetTemplateClassForExt(const wxString& Name)
{
    wxString ext=Name.AfterLast('.').Lower();

    for (int i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]->m_FileExtMap.find(ext)!=m_Descs[i]->m_FileExtMap.end())
            return m_Descs[i]->m_FileExtMap[ext].m_TemplateClass;
    }

    return NULL;
}

//-----------------------------------------------------------------------------
//find all classes that are ATTRCLASS_VIEWERREADY_F and have this field (the primary) in them
void CIwAttrDesc::FindFieldInClasses(CIwAttrViewerField* field)
{
    for (int i=0; i<(int)m_Classes.size(); i++)
    {
        if (m_Classes[i]->m_Flags&ATTRCLASS_VIEWERREADY_F)
        {
            for (CIwAttrClass* c=m_Classes[i]; c!=NULL; c=c->m_Parent)
            {
                for (int j=0; j<(int)c->m_Members.size(); j++)
                {
                    if (c->m_Members[j]->m_Name.IsSameAs(field->m_Primary,false))
                    {
                        field->m_InClasses.push_back(m_Classes[i]);
                        if (field->m_Required)
                            m_Classes[i]->m_NumRequired++;

                        break;
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//setup the type of the member, returning the advancement in the line
int CIwAttrDesc::GetType(const std::vector<wxString>& argv,CIwAttrMember* Member)
{
    int i,Offset=1;
    if (argv.size()<1) return 1;

    if (argv[0].IsSameAs(wxT("bool"),false))
        Member->m_Type=ATTRMEMBER_BOOL;
    else if (argv[0].IsSameAs(wxT("byte"),false))
    {
        Member->m_Type=ATTRMEMBER_BYTE;

        if (argv.size()>1 && argv[1].IsSameAs(wxT("unsigned"),false))
        {
            Member->m_Type|=ATTRMEMBER_UNSIGNED;
            Offset++;
        }
    }
    else if (argv[0].IsSameAs(wxT("short"),false))
    {
        Member->m_Type=ATTRMEMBER_SHORT;

        if (argv.size()>1 && argv[1].IsSameAs(wxT("unsigned"),false))
        {
            Member->m_Type|=ATTRMEMBER_UNSIGNED;
            Offset++;
        }
    }
    else if (argv[0].IsSameAs(wxT("int"),false))
    {
        Member->m_Type=ATTRMEMBER_INT;

        if (argv.size()>1 && argv[1].IsSameAs(wxT("unsigned"),false))
        {
            Member->m_Type|=ATTRMEMBER_UNSIGNED;
            Offset++;
        }
    }
    else if (argv[0].IsSameAs(wxT("int124"),false))
        Member->m_Type=ATTRMEMBER_INT124|ATTRMEMBER_LIST;
    else if (argv[0].IsSameAs(wxT("float"),false))
        Member->m_Type=ATTRMEMBER_FLOAT;
    else if (argv[0].IsSameAs(wxT("string"),false))
        Member->m_Type=ATTRMEMBER_STRING;
    else if (argv[0].IsSameAs(wxT("useTemplate"),false))
    {
        Member->m_Type=ATTRMEMBER_USETEMPLATE;
        Member->m_ArraySize=2;
    }
    else if (argv[0].IsSameAs(wxT("data"),false))
        Member->m_Type=ATTRMEMBER_DATA;
    else if (argv[0].IsSameAs(wxT("stringID"),false))
        Member->m_Type=ATTRMEMBER_STRINGID;
    else if (argv[0].IsSameAs(wxT("file"),false))
    {
        if (argv.size()<2) return 2;

        Offset++;

        if (argv[1].IsSameAs(wxT("texture"),false))
            Member->m_Type=ATTRMEMBER_FILETEXTURE;
        else if (argv[1].IsSameAs(wxT("font"),false))
            Member->m_Type=ATTRMEMBER_FILEFONT;
        else
            Member->m_Type=ATTRMEMBER_FILE;
    }
    else if (argv[0].IsSameAs(wxT("alignment"),false))
    {
        Member->m_Type=ATTRMEMBER_ALIGNMENT;
        Member->m_ArraySize=1;
    }
    else if (argv[0].IsSameAs(wxT("colour"),false))
    {
        Member->m_Type=ATTRMEMBER_COLOUR;
        Member->m_ArraySize=4;
    }
    else if (argv[0].IsSameAs(wxT("colour3"),false))
    {
        Member->m_Type=ATTRMEMBER_COLOUR3;
        Member->m_ArraySize=3;
    }
    else if (argv[0].IsSameAs(wxT("coord"),false))
        Member->m_Type=ATTRMEMBER_COORD;
    else if (argv[0].IsSameAs(wxT("resource"),false))
    {
        Member->m_Type=ATTRMEMBER_RESOURCE;
        Member->m_ArraySize=2;

    }
    else if (argv[0].IsSameAs(wxT("template"),false))
        Member->m_Type=ATTRMEMBER_DYNTEMPLATE;
    else if (argv[0].IsSameAs(wxT("extraclass"),false))
        Member->m_Type=ATTRMEMBER_EXTRACLASS;
    else if (argv[0].IsSameAs(wxT("enum"),false))
    {
        Member->m_Type=ATTRMEMBER_ENUM;

        for (Offset=1; Offset<(int)argv.size(); Offset++)
        {
            if (argv[Offset][0]==wxT(';'))
                break;
        }

        Member->m_Items.resize(Offset-1);
        Member->m_Text.resize(Offset);
        Member->m_Text[0]=Member->m_Name;

        for (i=1; i<Offset; i++)
        {
            Member->m_Items[i-1].m_String=argv[i];
            Member->m_Text[i]=argv[i];
        }
    }
    else if (argv[0].IsSameAs(wxT("ptr"),false))
    {
        Member->m_Type=ATTRMEMBER_PTR;

        for (Offset=1; Offset<(int)argv.size(); Offset++)
        {
            if (argv[Offset][0]==wxT(';') || argv[Offset][0]==wxT('*'))
                break;
        }

        Member->m_Items.resize(Offset-1);

        for (i=1; i<Offset; i++)
        {
            Member->m_Items[i-1].m_Class=m_Mgr->GetClass(argv[i]);
        }
    }
    else if (argv[0].IsSameAs(wxT("class"),false))
    {
        if (argv.size()<3) return 3;

        Offset+=2;
        Member->m_Type=ATTRMEMBER_CLASS;

        switch (argv[1][0])
        {
        case wxT('+'):
            Member->m_Type|=ATTRMEMBER_0OR1;
            break;
        case wxT('*'):
            Member->m_Type|=ATTRMEMBER_0ORMORE;
            break;
        }

        Member->m_Items.resize(1);
        Member->m_Items[0].m_Class=m_Mgr->GetClass(argv[2]);
    }
    else if (argv[0].IsSameAs(wxT("classname"),false))
    {
        if (argv.size()<3) return 3;

        Offset+=2;
        Member->m_Type=ATTRMEMBER_CLASS;

        switch (argv[1][0])
        {
        case wxT('+'):
            Member->m_Type|=ATTRMEMBER_0OR1;
            break;
        case wxT('*'):
            Member->m_Type|=ATTRMEMBER_0ORMORE;
            break;
        }
        Member->m_PreMember=L"name";

        Member->m_Items.resize(1);
        Member->m_Items[0].m_Class=m_Mgr->GetClass(argv[2]);
    }
    else if (argv[0].IsSameAs(wxT("{"),false))
    {
        std::vector<wxString> argv2;
        for (i=1; i<(int)argv.size(); i++)
        {
            argv2.push_back(argv[i]);
        }

        GetType(argv2,Member);

        if (argv2.size()>1 && argv2[argv2.size()-2]==L"*")
        {
            Member->m_ArraySize=0;
            Member->m_Type|=ATTRMEMBER_LIST;
        }
        else
        {
            Member->m_Type|=ATTRMEMBER_ARRAY;

            Offset=0;
            Member->m_ArraySize=0;
            while (Offset<(int)argv.size() && argv[Offset].IsSameAs(wxT("{")))
            {
                Offset++;
                int start=Offset;
                for (; Offset<(int)argv.size(); Offset++)
                {
                    if (argv[Offset][0]==wxT('}'))
                        break;
                }

                Member->m_ArraySize+=Offset-start;
                Member->m_Boundary.push_back(Offset-start);
                Offset++;
            }
        }
    }

    return Offset;
}

//-----------------------------------------------------------------------------
//get the default value of a member from the line
int CIwAttrDesc::GetDefault(const std::vector<wxString>& argv,CIwAttrMember* Member)
{
    char* Ptr;
    if (argv.size()<=0) return 0;

    if (HasNoMemberData(Member->m_Type))
        return 0;

    Member->m_Items.resize(Member->m_Items.size()+1);

    switch (Member->m_Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_BOOL:
        if (argv[0].IsSameAs(wxT("false"),false))
            Member->m_Items.back().m_Int=0;
        else
            Member->m_Items.back().m_Int=1;

        break;
    case ATTRMEMBER_BYTE:
    case ATTRMEMBER_SHORT:
    case ATTRMEMBER_INT:
    case ATTRMEMBER_INT124:
    case ATTRMEMBER_STRINGID:
    case ATTRMEMBER_ALIGNMENT:
    case ATTRMEMBER_COLOUR:
    case ATTRMEMBER_COLOUR3:
        if (Member->m_Type&ATTRMEMBER_UNSIGNED)
            Member->m_Items.back().m_UInt=strtoul(argv[0].mb_str(),&Ptr,10);
        else
            Member->m_Items.back().m_Int=atoi(argv[0].mb_str());

        break;
    case ATTRMEMBER_FLOAT:
        Member->m_Items.back().m_Float=(float)atof(argv[0].mb_str());
        break;

    case ATTRMEMBER_FILE:
    case ATTRMEMBER_FILETEXTURE:
    case ATTRMEMBER_FILEFONT:
    case ATTRMEMBER_STRING:
    case ATTRMEMBER_USETEMPLATE:
    case ATTRMEMBER_DATA:
        Member->m_Items.back().m_String=argv[0];
        break;
    case ATTRMEMBER_DYNTEMPLATE:
    case ATTRMEMBER_EXTRACLASS:
        Member->m_Items.back().m_Class=m_Mgr->GetClass(argv[0]);
        if (Member->m_Items.back().m_Class==NULL)
            Member->m_Items.back().m_Class=Member->m_Parent;

        break;

    case ATTRMEMBER_COORD:
        if (argv[0].Find(wxT('.'))!=wxNOT_FOUND)
        {
            Member->m_Items.back().m_Coord.m_UseInt=false;
            Member->m_Items.back().m_Coord.m_Float=(float)atof(argv[0].mb_str());
        }
        else
        {
            Member->m_Items.back().m_Coord.m_UseInt=true;
            Member->m_Items.back().m_Coord.m_Int=atoi(argv[0].mb_str());
        }

        break;
    }
    return 1;
}

//-----------------------------------------------------------------------------
//load a template description
bool CIwAttrDesc::Load(const wxString& FileName)
{
    CIwTextFile fp(FileName);
    if (!fp.Exists())
        return false;

    fp.Open();

    m_FileName=FileName;
    wxString Dir=FileName;
    Dir.Replace(wxT("\\"),wxT("/"));
    if (Dir.Find(wxT('/'))!=wxNOT_FOUND)
        Dir=Dir.BeforeLast(wxT('/'));
    else
        Dir.clear();

    int mode=0;
    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        wxString Line2;

        if (mode!=10) mode=0;

        for (int k=0; k<(int)fp[i].size(); k++)
        {
            switch (mode)
            {
            case 0:
                if (fp[i][k]=='/')
                    mode=1;
                else
                    Line2+=fp[i][k];

                break;
            case 1:    //: /
                if (fp[i][k]=='/')
                    mode=2;
                else if (fp[i][k]=='*')
                    mode=10;
                else
                {
                    Line2+='/';
                    Line2+=fp[i][k];
                    mode=0;
                }

                break;
            case 2:    //: //
                break;
            case 10:    //: /* xxx
                if (fp[i][k]=='*')
                    mode=11;

                break;
            case 11:    //: /* xxx *
                if (fp[i][k]=='/')
                    mode=0;
                else
                    mode=10;

                break;
            }
        }

        std::vector<wxString> argv;
        if (SuperSplit(Line2,argv,wxT(" \n\t|"))<1)
            continue;

        ReadLine(argv,Dir);
    }

    return true;
}

//-----------------------------------------------------------------------------
//get a class from name
CIwAttrClass* CIwAttrDesc::GetClass(const wxString& Name)
{
    for (int i=0; i<(int)m_Classes.size(); i++)
    {
        if (m_Classes[i]->m_Name.IsSameAs(Name,false))
            return m_Classes[i];

        for (int j=0; j<(int)m_Classes[i]->m_AltNames.size(); j++)
        {
            if (m_Classes[i]->m_AltNames[j].IsSameAs(Name,false))
                return m_Classes[i];
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
//reset description
void CIwAttrDesc::Clear()
{
    int i;

    m_CurrClass=NULL;
    m_CurrMember=NULL;
    m_CurrGroup=NULL;
    m_Indent=0;
    m_Restrict=false;

    for (i=0; i<EXPORTTYPE_NUM; i++)
    {
        m_ExportTypes[i].clear();
    }

    for (i=0; i<(int)m_Classes.size(); i++)
    {
        delete m_Classes[i];
    }

    m_Classes.clear();

    for (i=0; i<(int)m_ViewerFields.size(); i++)
    {
        delete m_ViewerFields[i];
    }

    m_ViewerFields.clear();
}

//-----------------------------------------------------------------------------
void CIwAttrData::SetChanged(bool Changed)
{
    m_FromDefault=!Changed;
    if (Changed && m_Instance->m_Dialog!=NULL)
        m_Instance->m_Dialog->DealWithChange(this);

    m_Instance->SetChanged(Changed);
}

//-----------------------------------------------------------------------------
CIwAttrData::~CIwAttrData()
{
    if (m_Editor)
        m_Editor->ClearData(this);

    for (int i=0; i<(int)m_Items.size(); i++)
    {
        if (m_Items[i].m_Inst!=NULL && m_Items[i].m_Flags&ATTRITEM_ALLOCED_F)
            delete m_Items[i].m_Inst;
    }
}

//-----------------------------------------------------------------------------
//copy the default value from the description
void CIwAttrData::SetDefault(bool skipInstances)
{
    CIwAttrInstance* Inst;
    m_Items.resize(m_Member->m_ArraySize);

    for (int i=0; i<(int)m_Items.size(); i++)
    {
        switch (m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_BOOL:
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
        case ATTRMEMBER_INT124:
        case ATTRMEMBER_STRINGID:
        case ATTRMEMBER_ALIGNMENT:
        case ATTRMEMBER_COLOUR:
        case ATTRMEMBER_COLOUR3:
            m_Items[i].m_Int=m_Member->m_Items[i].m_Int;
            m_Items[i].m_UInt=m_Member->m_Items[i].m_UInt;
            break;
        case ATTRMEMBER_ENUM:
            m_Items[i].m_Int=0;
            break;
        case ATTRMEMBER_FLOAT:
            m_Items[i].m_Float=m_Member->m_Items[i].m_Float;
            break;

        case ATTRMEMBER_FILE:
        case ATTRMEMBER_FILETEXTURE:
        case ATTRMEMBER_FILEFONT:
        case ATTRMEMBER_STRING:
        case ATTRMEMBER_USETEMPLATE:
        case ATTRMEMBER_DATA:
            m_Items[i].m_String=m_Member->m_Items[i].m_String;
            break;

        case ATTRMEMBER_COORD:
            if (m_Member->m_Items[i].m_Coord.m_UseInt)
                m_Items[i].m_Coord.m_Int=m_Member->m_Items[i].m_Coord.m_Int;
            else
                m_Items[i].m_Coord.m_Float=m_Member->m_Items[i].m_Coord.m_Float;

            m_Items[i].m_Coord.m_UseInt=m_Member->m_Items[i].m_Coord.m_UseInt;
            break;
        case ATTRMEMBER_PTR:
            m_Items[i].m_Inst=NULL;
            m_Items[i].m_Flags=0;
            m_Items[i].m_String=L"";
            break;
        case ATTRMEMBER_DYNTEMPLATE:
        case ATTRMEMBER_EXTRACLASS:
            m_Items[i].m_Class=m_Member->m_Items[i].m_Class;
            break;
        case ATTRMEMBER_CLASS:
        case ATTRMEMBER_CHILD:
            if (!skipInstances)
            {
                Inst=new CIwAttrInstance;
                Inst->m_Mgr=m_Mgr;
                Inst->m_Class=m_Member->m_Items[0].m_Class;
                Inst->AddDefaults(Inst->m_Class);
                Inst->m_File=m_Instance->m_File;
                Inst->m_Dialog=m_Instance->m_Dialog;

                Inst->m_Parent=this;
                m_Items[i].m_Inst=Inst;
                m_Items[i].m_Flags=ATTRITEM_ALLOCED_F;
            }

            break;
        }
    }
    //is there a wizard to fill this data?
    if (!m_Member->m_Wizard.empty() && m_Mgr->m_Extra!=NULL)
        m_Mgr->m_Extra->DoWizard(this);
}

static float lastFristFrame=0;

//-----------------------------------------------------------------------------
//set the data from an input
void CIwAttrData::Set(std::vector<wxString>& argv,CIwAttrInstance* Base)
{
    int i,j,size=m_Member->m_ArraySize;
    char* Ptr;
    if (m_Member->m_Type&ATTRMEMBER_LIST)
    {
        size=(int)argv.size();
        m_Items.resize(size);
    }
    else if ((int)argv.size()<m_Member->m_ArraySize)
        size=(int)argv.size();

    m_FromDefault=false;

    for (i=0; i<size; i++)
    {
        if (argv[i][0]=='!')
        {
            argv[i]=argv[i].Mid(1);
            m_Items[i].m_Flags|=ATTRITEM_OVERRIDE_F;
        }

        switch (m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_BOOL:
            if (argv[i].IsSameAs(wxT("false"),false) || argv[i].IsSameAs(wxT("0"),false))
                m_Items[i].m_Int=0;
            else
                m_Items[i].m_Int=1;

            break;
        case ATTRMEMBER_INT124:
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
        case ATTRMEMBER_STRINGID:       //todo link to external data
        case ATTRMEMBER_COLOUR:
        case ATTRMEMBER_COLOUR3:
            if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
                m_Items[i].m_UInt=strtoul(argv[i].mb_str(),&Ptr,10);
            else
                m_Items[i].m_Int=atoi(argv[i].mb_str());

            break;
        case ATTRMEMBER_FLOAT:
            if (argv[i][0]=='+')
            {
                m_Items[i].m_Float=(float)atof(argv[i].mb_str()+1);
                m_Items[i].m_Flags=ATTRITEM_FIRSTFRAME_F;

                if (m_Member->m_ExportSection.IsSameAs(L"firstframe",false))
                    m_Items[i].m_Float+=lastFristFrame;
            }
            else
            {
                m_Items[i].m_Float=(float)atof(argv[i].mb_str());
                m_Items[i].m_Flags=0;
            }

            if (m_Member->m_ExportSection.IsSameAs(L"firstframe",false))
                lastFristFrame=m_Items[i].m_Float;

            break;
        case ATTRMEMBER_ALIGNMENT:    //left|centre|right top|centre|bottom
            if (argv[i].IsSameAs(wxT("centre"),false))
                m_Items[i].m_Int=1;
            else if (argv[i].IsSameAs(wxT("right"),false))
                m_Items[i].m_Int=2;
            else if (argv[i].IsSameAs(wxT("left"),false))
                m_Items[i].m_Int=0;
            else
            {
                m_Items[i].m_Int=atoi(argv[i].mb_str());
                break;
            }

            if (argv[i+1].IsSameAs(wxT("centre"),false))
                m_Items[i].m_Int=1<<4;
            else if (argv[i+1].IsSameAs(wxT("bottom"),false))
                m_Items[i].m_Int=2<<4;
            else
                m_Items[i].m_Int=0;

            i++;
            break;

        case ATTRMEMBER_RESOURCE:
            m_Items[0].m_String=argv[i];
            if ((int)argv.size()>1)
            {
                CIwAttrInstance* Inst=NULL;
                CIwAttrClass* klass=m_Mgr->GetTemplateClassForExt(argv[i]);
                if (klass!=NULL)
                {
                    Inst=new CIwAttrInstance;
                    Inst->m_Mgr=m_Mgr;
                    Inst->m_Class=klass;
                    Inst->AddDefaults(Inst->m_Class,m_Group);
                    Inst->m_File=m_Instance->m_File;
                    Inst->m_Dialog=m_Instance->m_Dialog;

                    Inst->m_Parent=this;
                    m_Items[1].m_Inst=Inst;
                    m_Items[1].m_Flags=ATTRITEM_ALLOCED_F;
                }

                for (i=2; i<(int)argv.size(); i+=2)
                {
                    if (Inst!=NULL)
                    {
                        CIwAttrNote Note;
                        Note.m_Name=argv[i-1];
                        Note.m_Data=argv[i];
                        Note.m_Info=-2;
                        std::vector<wxString> argv2;
                        argv2.push_back(argv[i]);

                        Inst->AddFromNote(argv[i-1],Note,argv2,Base);
                    }
                }
            }

            break;
        case ATTRMEMBER_DYNTEMPLATE:
        case ATTRMEMBER_EXTRACLASS:
            m_Items[i].m_Class=m_Mgr->GetClass(argv[i]);
            if (m_Items[i].m_Class==NULL)
                m_Items[i].m_Class=m_Member->m_Parent;
            else
                m_Instance->AddDefaults(m_Items[i].m_Class,m_Group,true);

            break;
        case ATTRMEMBER_FILE:
        case ATTRMEMBER_FILETEXTURE:
        case ATTRMEMBER_FILEFONT:
        case ATTRMEMBER_STRING:
        case ATTRMEMBER_USETEMPLATE:
            m_Items[i].m_String=argv[i];
            break;
        case ATTRMEMBER_DATA:
            m_Items[i].m_String=argv[i];
            for (j=i+1; j<(int)argv.size(); j++)
            {
                m_Items[i].m_String+=L" "+argv[j];
            }
            i=j;
            break;

        case ATTRMEMBER_COORD:
            if (argv[i].Find(wxT('.'))!=wxNOT_FOUND)
            {
                m_Items[i].m_Coord.m_UseInt=false;
                m_Items[i].m_Coord.m_Float=(float)atof(argv[i].mb_str());
            }
            else
            {
                m_Items[i].m_Coord.m_UseInt=true;
                m_Items[i].m_Coord.m_Int=atoi(argv[i].mb_str());
            }

            break;
        case ATTRMEMBER_ENUM:
            for (j=0; j<(int)m_Member->m_Items.size(); j++)
            {
                if (argv[i].IsSameAs(m_Member->m_Items[j].m_String,false))
                {
                    m_Items[i].m_Int=j;
                    break;
                }
            }
            if (j==(int)m_Member->m_Items.size())
                m_Items[i].m_Int=atoi(argv[i].mb_str());

            if (m_Items[i].m_Int<0 || m_Items[i].m_Int>=(int)m_Member->m_Items.size())
                m_Items[i].m_Int=0;

            break;
        }
    }

    for (; i<(int)argv.size(); i++)
    {
        m_LineComment+=L" "+argv[i];
    }
}

//-----------------------------------------------------------------------------
CIwAttrDescMgr::CIwAttrDescMgr(bool IsExporter) : m_IsExporter(IsExporter),m_IsWizard(false),m_Extra(NULL),m_FrameRate(1)
{
    m_DefaultClass.m_Name=wxT("CIwManaged");
    m_DefaultClass.m_Parent=NULL;
    m_DefaultClass.m_Flags=ATTRCLASS_DEFAULT_F;
}

//-----------------------------------------------------------------------------
void CIwAttrDescMgr::Clear()
{
    for (int i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]!=NULL)
            delete m_Descs[i];
    }
    m_Descs.clear();
}

//-----------------------------------------------------------------------------
CIwAttrDescMgr::~CIwAttrDescMgr()
{
    Clear();
}

//-----------------------------------------------------------------------------
//get a class from name
CIwAttrClass* CIwAttrDescMgr::GetClass(const wxString& Name)
{
    for (int i=m_Descs.size()-1; i>=0; i--)
    {
        if (m_Descs[i]!=NULL)
        {
            CIwAttrClass* Class=m_Descs[i]->GetClass(Name);

            if (Class!=NULL)
                return Class;
        }
    }
    return NULL;
}

//-----------------------------------------------------------------------------
CIwAttrClass* CIwAttrDescMgr::MakeContainer(CIwAttrExportType type,const wxString& rootName)
{
    CIwAttrClass* currClass=new CIwAttrClass;
    currClass->m_Name=rootName;
    currClass->m_Flags|=ATTRCLASS_TEMP_F;

    for (int j=0; j<(int)m_Descs.size(); j++)
    {
        for (int i=0; i<(int)m_Descs[j]->m_ExportTypes[type].size(); i++)
        {
            CIwAttrMember* currMember=new CIwAttrMember;
            currMember->m_Name=m_Descs[j]->m_ExportTypes[type][i]->m_Name;
            currMember->m_Type=ATTRMEMBER_CHILD|ATTRMEMBER_0ORMORE;
            currMember->m_Indent=1;
            currMember->m_ArraySize=1;
            currMember->m_Sticky=false;
            currMember->m_Parent=currClass;

            currClass->m_Members.push_back(currMember);

            currMember->m_Items.resize(1);

            currMember->m_Items[0].m_Class=m_Descs[j]->m_ExportTypes[type][i];
        }
    }
    return currClass;
}

//-----------------------------------------------------------------------------
CIwAttrClass* CIwAttrDescMgr::MakeContainer(const wxString& ext,std::vector<CIwAttrClass*>& Classes)
{
    CIwAttrClass* currClass=new CIwAttrClass;
    currClass->m_Name=L"data";
    currClass->m_Flags|=ATTRCLASS_TEMP_F;

    for (int j=0; j<(int)m_Descs.size(); j++)
    {
        if (m_Descs[j]->m_FileExtMap.find(ext.Lower())==m_Descs[j]->m_FileExtMap.end())
            continue;

        for (int k=0; k<(int)m_Descs[j]->m_FileExtMap[ext.Lower()].m_Types.size(); k++)
        {
            CIwAttrExportType type=m_Descs[j]->m_FileExtMap[ext.Lower()].m_Types[k];

            for (int i=0; i<(int)m_Descs[j]->m_ExportTypes[type].size(); i++)
            {
                CIwAttrMember* currMember=new CIwAttrMember;
                currMember->m_Name=m_Descs[j]->m_ExportTypes[type][i]->m_Name;
                currMember->m_Type=ATTRMEMBER_CHILD|ATTRMEMBER_0ORMORE;
                currMember->m_Indent=1;
                currMember->m_ArraySize=1;
                currMember->m_Sticky=false;
                currMember->m_ExportType=type;
                currMember->m_Parent=currClass;

                currClass->m_Members.push_back(currMember);

                currMember->m_Items.resize(1);

                currMember->m_Items[0].m_Class=m_Descs[j]->m_ExportTypes[type][i];

                Classes.push_back(m_Descs[j]->m_ExportTypes[type][i]);
            }
        }
    }
    return currClass;
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::MakeInstance(const wxString& rootName)
{
    CIwAttrClass* currClass=new CIwAttrClass;
    currClass->m_Name=rootName;
    currClass->m_Flags|=ATTRCLASS_TEMP_F;

    CIwAttrMember* currMember=new CIwAttrMember;
    currMember->m_Name=rootName;
    currMember->m_Type=ATTRMEMBER_CHILD|ATTRMEMBER_0ORMORE;
    currMember->m_Indent=1;
    currMember->m_ArraySize=1;
    currMember->m_Sticky=false;
    currMember->m_Parent=currClass;

    currClass->m_Members.push_back(currMember);

    currMember->m_Items.resize(1);
    currMember->m_Items[0].m_Class=GetClass(rootName);

    CIwAttrInstance* Inst=new CIwAttrInstance;
    Inst->m_Mgr=this;
    Inst->m_ClassInfo=0;
    Inst->m_Parent=NULL;
    Inst->m_File=NULL;
    Inst->Reset(currClass);

    return Inst;
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::GetFromFile(const wxString& Path,CIwAttrFileGroup* group)
{
    std::vector<CIwAttrClass*> Classes;
    wxString ext=Path.AfterLast('.').Lower();

    CIwAttrFileGroup tempGroup;
    if (group==NULL) group=&tempGroup;

    CIwAttrInstance* Inst=new CIwAttrInstance;
    Inst->m_Mgr=this;
    Inst->m_ClassInfo=0;
    Inst->m_Parent=NULL;
    Inst->Reset(MakeContainer(ext,Classes));

    Inst->m_File=group;
    group->m_Inst=Inst;

    GetDerivedClasses(Classes);

    CIwAttrInstance* inst=GetFromFile2(Path,Classes,group);
    if (inst==NULL) return NULL;

    group->m_CurrType=EXPORTTYPE_NUM;
    group->m_Ext=ext;

    for (int k=0; k<EXPORTTYPE_NUM; k++)
    {
        int num=0;
        for (int j=0; j<(int)m_Descs.size() && num==0; j++)
        {
            for (int i=0; i<(int)m_Descs[j]->m_ExportTypes[k].size() && num==0; i++)
            {
                for (int m=0; m<(int)inst->m_Data.size() && num==0; m++)
                {
                    if ((inst->m_Data[m]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
                        continue;

                    for (int n=0; n<(int)m_Descs[j]->m_ExportTypes[k].size() && num==0; n++)
                    {
                        for (CIwAttrClass* klass=inst->m_Data[m]->m_Items[0].m_Inst->m_Class; klass!=NULL; klass=klass->m_Parent)
                        {
                            if (klass==m_Descs[j]->m_ExportTypes[k][n])
                            {
                                num++;
                                break;
                            }
                        }
                    }
                }
            }
        }
        if (num>0)
        {
            if (group->m_CurrType==EXPORTTYPE_NUM)
                group->m_CurrType=(CIwAttrExportType)k;
            else
            {
                group->m_CurrType=EXPORTTYPE_NUM;
                break;
            }
        }
    }
    return inst;
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::GetFromFile(const wxString& Path,CIwAttrExportType Type,const wxString& rootName,CIwAttrFileGroup* group)
{
    CIwAttrFileGroup* group2=group;
    CIwAttrFileGroup tempGroup;
    if (group==NULL) group2=&tempGroup;

    CIwAttrInstance* Inst=new CIwAttrInstance;
    Inst->m_Mgr=this;
    Inst->m_ClassInfo=0;
    Inst->m_Parent=NULL;
    Inst->m_File=group;
    Inst->Reset(MakeContainer(Type,rootName));
    group2->m_Inst=Inst;

    int i=0,j=0;
    std::vector<CIwAttrClass*> Classes;

    //find all possible root classes
    for (i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]!=NULL)
        {
            for (j=0; j<(int)m_Descs[i]->m_ExportTypes[Type].size(); j++)
            {
                Classes.push_back(m_Descs[i]->m_ExportTypes[Type][j]);
            }
        }

        GetDerivedClasses(Classes);
    }

    CIwAttrInstance* inst=GetFromFile2(Path,Classes,group2);
    tempGroup.m_Inst=NULL;
    return inst;
}

//-----------------------------------------------------------------------------
//create an instance from a file, Type is what sort of thing it is, Group is an optional container for the instance
CIwAttrInstance* CIwAttrDescMgr::GetFromFile(const wxString& Path,CIwAttrExportType Type,CIwAttrFileGroup* group)
{
    int i=0,j=0;
    std::vector<CIwAttrClass*> Classes;

    //find all possible root classes
    for (i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]!=NULL)
        {
            for (j=0; j<(int)m_Descs[i]->m_ExportTypes[Type].size(); j++)
            {
                Classes.push_back(m_Descs[i]->m_ExportTypes[Type][j]);
            }
        }

        GetDerivedClasses(Classes);
    }

    return GetFromFile2( Path, Classes, group);
}

void MakeNote(CIwAttrNote& Note,std::vector<wxString>& argv)
{
    Note.m_Name=argv[0];
    Note.m_Info=-2;
    for (int i=1; i<(int)argv.size(); i++)
    {
        /*if(argv[i].size()>1 && argv[i][0]=='{')
           {
            argv[i]=argv[i].Mid(1);
            argv.insert(argv.begin()+i,L"{");
            i++;
           }
           if(argv[i].size()>1 && argv[i][argv[i].size()-1]=='}')
           {
            argv[i].RemoveLast();
            argv.insert(argv.begin()+i+1,L"}");
           }

           if(argv[i]==L"{" || argv[i]==L"}") continue;*/

        Note.m_Data.append(argv[i]);
        Note.m_Data.append(wxT(" "));
    }
}

//-----------------------------------------------------------------------------
void CIwAttrDescMgr::MakeIntoNote(CIwAttrNote& note,const wxString& string)
{
    std::vector<wxString> argv;
    SuperSplit(string,argv,L" \t\n");
    MakeNote(note,argv);
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::GetFromExtraNotes(std::vector<CIwAttrNote>& inNotes,CIwAttrExportType Type,const wxString& rootName)
{
    CIwAttrFileGroup group;

    CIwAttrInstance* Inst=new CIwAttrInstance;
    Inst->m_Mgr=this;
    Inst->m_ClassInfo=0;
    Inst->m_Parent=NULL;
    Inst->m_File=&group;
    Inst->Reset(MakeContainer(Type,rootName));
    group.m_Inst=Inst;

    CIwAttrInstance* inst=GetFromExtraNotes2(inNotes,&group);
    group.m_Inst=NULL;
    return inst;
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::GetFromExtraNotes2(std::vector<CIwAttrNote>& inNotes,CIwAttrFileGroup* group)
{
    int Indent=0,Ignore=0;
    CIwAttrInstance* Inst=group->m_Inst;
    CIwAttrInstance* Inst2=Inst;
    std::vector<CIwAttrInstance*> Stack;

    for (int j=0; j<(int)inNotes.size(); j++)
    {
        bool ignoreNow=false;

        if (inNotes[j].m_Name.IsSameAs(wxT("{"),false))
        {
            if (Inst2==NULL) //if class was not found skip this section
            {
                Ignore++;
                ignoreNow=true;
            }
            else
            {
                Indent++;
                continue;
            }
        }
        else if (inNotes[j].m_Name.IsSameAs(wxT("}"),false))
        {
            if (Ignore>0)
            {
                Ignore--;
                ignoreNow=true;
            }
            else
            {
                Indent--;
                if (!Stack.empty())
                {
                    Inst=Stack.back();
                    Stack.pop_back();
                }

                continue;
            }
        }

        if (Ignore>0 || ignoreNow)
        {
            if (Inst!=NULL)
                Inst->m_ExtraData.push_back(inNotes[j]);

            continue;
        }

        // add data to current class
        std::vector<wxString> argv2;
        SuperSplit(inNotes[j].m_Data,argv2,wxT(", \t\n{}"));

        Inst2=Inst->AddFromNote(inNotes[j].m_Name,inNotes[j],argv2,Inst);
        if (Inst2!=NULL) // is this a sub section
        {
            Stack.push_back(Inst);
            Inst=Inst2;
        }
    }

    return Inst;
}

//-----------------------------------------------------------------------------
bool CIwAttrDescMgr::ReadFile(const wxString& Path,std::vector<LineStruct>& lines)
{
    wxTextFile fp(Path);
    if (!fp.Exists() || !fp.Open())
        return false;

    int mode=0;
    for (int j=0; j<(int)fp.GetLineCount(); j++)
    {
        int intoData=0;
        LineStruct line;
        line.m_Line=fp[j];
        wxString Line2;
        int start=-1,end=-1,iter=0;

        if (mode!=10) mode=0;

        for (int k=0; k<(int)fp[j].size(); k++)
        {
            switch (mode)
            {
            case 0:
                if (fp[j][k]=='#')
                {
                    intoData=0;
                    mode=3;
                }
                else if (fp[j][k]=='/')
                {
                    intoData=0;
                    mode=1;
                }
                else if (fp[j][k]=='{')
                {
                    intoData=1;
                    if (!Line2.empty())
                    {
                        line.m_Args.push_back(Line2);
                        Line2.clear();
                    }

                    line.m_Args.push_back(L"{");
                }
                else if (fp[j][k]=='}')
                {
                    intoData=1;
                    if (!Line2.empty())
                    {
                        line.m_Args.push_back(Line2);
                        Line2.clear();
                    }

                    line.m_Args.push_back(L"}");
                }
                else if (fp[j][k]=='\"')
                {
                    intoData=1;
                    mode=4;
                    if (!Line2.empty())
                    {
                        line.m_Args.push_back(Line2);
                        Line2.clear();
                    }
                }
                else if (fp[j][k]==' ' || fp[j][k]=='\t' || fp[j][k]=='\r' || fp[j][k]=='\n' || fp[j][k]==',')
                {
                    intoData=0;
                    if (!Line2.empty())
                    {
                        line.m_Args.push_back(Line2);
                        Line2.clear();
                    }
                }
                else
                {
                    intoData++;
                    Line2+=fp[j][k];
                }

                break;
            case 1:    //: /
                intoData=0;
                if (fp[j][k]=='/')
                    mode=2;
                else if (fp[j][k]=='*')
                    mode=10;
                else
                {
                    Line2+='/';
                    mode=0;
                    k--;
                }

                break;
            case 2:    //: //
                intoData=0;
                if (fp[j][k]==':')
                    mode=0;
                else
                    mode=3;

                break;
            case 3:    //: //xxx
                intoData=0;
                break;
            case 4:    //: "
                intoData++;
                if (fp[j][k]=='\\')
                    mode=5;
                else if (fp[j][k]=='\"')
                {
                    line.m_Args.push_back(Line2);
                    Line2.clear();
                    mode=0;
                }
                else
                    Line2+=fp[j][k];

                break;
            case 5:    //: "xxx\'
                intoData++;
                if (fp[j][k]=='\"')
                    Line2+='\"';
                else
                {
                    Line2+='\\';
                    k--;
                }

                mode=4;
                break;
            case 10:    //: /*xxx
                intoData=0;
                if (fp[j][k]=='*')
                    mode=11;

                break;
            case 11:    //: /*xxx*
                intoData=0;
                if (fp[j][k]=='/')
                    mode=0;
                else
                    mode=10;

                break;
            }
            if (intoData==1)
            {
                if (iter==1)
                    start=k;

                iter++;
            }

            if (intoData>0 && iter>1)
                end=k;
        }
        if (!Line2.empty())
        {
            line.m_Args.push_back(Line2);
            Line2.clear();
        }

        if (start!=-1 && end!=-1)
            line.m_Data=line.m_Line.SubString(start,end+1);

        lines.push_back(line);
    }


    return true;
}

//-----------------------------------------------------------------------------
CIwAttrInstance* CIwAttrDescMgr::GetFromFile2(const wxString& Path,std::vector<CIwAttrClass*>& Classes,CIwAttrFileGroup* group)
{
    wxString Dir;
    int FileNum=-2;

    int Indent=0,Ignore=0;
    int i;
    CIwAttrInstance* Inst=(group ? group->m_Inst : NULL);
    CIwAttrInstance* Inst2=Inst;
    std::vector<CIwAttrInstance*> Stack;
    std::vector<CIwAttrNote> notes;

    if (group!=NULL)
        group->m_FileName=Path;

    std::vector<LineStruct> lines;
    if (!ReadFile(Path,lines))
        return Inst;

    Dir=Path;
    Dir.Replace(wxT("\\"),wxT("/"));
    if (Dir.Find(wxT('/'))!=wxNOT_FOUND)
        Dir=Dir.BeforeLast(wxT('/'));
    else
        Dir.clear();

    for (int j=0; j<(int)lines.size(); j++)
    {
        bool ignoreNow=false;

        if (lines[j].m_Args.size()<1)
        {
            if (Inst!=NULL && !lines[j].m_Line.Trim().empty())
            {
                CIwAttrNote Note;
                Note.m_Name=lines[j].m_Line;
                Note.m_Info=-2;
                Inst->m_ExtraData.push_back(Note);
            }

            continue;
        }

        if (lines[j].m_Args[0].IsSameAs(wxT("include"),false) && lines[j].m_Args.size()>1)
        {
            CIwAttrFileSubGroup* group2=new CIwAttrFileSubGroup(group,Inst);
            wxString path2;
            if (lines[j].m_Args[1][0]=='.')
                path2=Dir+lines[j].m_Args[1].Mid(1);
            else
                path2=m_BaseDir+L"/"+lines[j].m_Args[1];

            Inst->m_File=group2;
            GetFromFile2(path2,Classes,group2);
            Inst->m_File=group;
            continue;
        }

        if (lines[j].m_Args[0].IsSameAs(wxT("{"),false))
        {
            if (Inst2==NULL) //if class was not found skip this section
            {
                Ignore++;
                ignoreNow=true;
            }
            else
            {
                Indent++;
                continue;
            }
        }
        else if (lines[j].m_Args[0].IsSameAs(wxT("}"),false))
        {
            if (Ignore>0)
            {
                Ignore--;
                ignoreNow=true;
            }
            else
            {
                Indent--;
                if (!Stack.empty())
                {
                    Inst=Stack.back();
                    Stack.pop_back();
                }

                continue;
            }
        }

        if (Ignore>0 || ignoreNow)
        {
            if (Inst!=NULL)
            {
                CIwAttrNote Note;
                MakeNote(Note,lines[j].m_Args);
                Note.m_Data=lines[j].m_Data;
                Note.m_Info=-2;
                Inst->m_ExtraData.push_back(Note);
            }

            continue;
        }

        if (Inst==NULL)
        {
            //find containing class
            for (i=0; i<(int)Classes.size(); i++)
            {
                if (Classes[i]->m_Name.IsSameAs(lines[j].m_Args[0],false))
                {
                    Inst=new CIwAttrInstance;
                    Inst->m_File=group;
                    Inst->m_Mgr=this;
                    Inst->m_Class=Classes[i];
                    Inst->m_ClassInfo=FileNum;
                    Inst->m_Parent=NULL;

                    Inst->AddDefaults(Classes[i]);
                    Inst2=Inst;

                    if (lines[j].m_Args.size()>1 && lines[j].m_Args[1][0]==wxT('{'))
                        Indent++;

                    for (int k=0; k<(int)notes.size(); k++)
                    {
                        Inst->m_ExtraData.push_back(notes[k]);
                    }
                    notes.clear();
                    break;
                }
            }
            if (i==(int)Classes.size())
            {
                CIwAttrNote Note;
                MakeNote(Note,lines[j].m_Args);
                Note.m_Data=lines[j].m_Data;
                notes.push_back(Note);
            }
        }
        else
        {
            // add data to current class
            CIwAttrNote Note;
            MakeNote(Note,lines[j].m_Args);
            Note.m_Data=lines[j].m_Data;

            std::vector<wxString> argv;
            for (i=1; i<(int)lines[j].m_Args.size(); i++)
            {
                argv.push_back(lines[j].m_Args[i]);
            }

            Inst2=Inst->AddFromNote(lines[j].m_Args[0],Note,argv,Inst);
            if (Inst2!=NULL) // is this a sub section
            {
                Stack.push_back(Inst);
                Inst=Inst2;
            }

            if ((int)argv.size()>0)
            {
                if (argv[0]==L"*")
                {
                    argv[0]=Note.m_Name;
                    j+=ReadPtrArray(Inst,argv,1,lines,j);
                }
                else
                {
                    i=0;
                    while (true)
                    {
                        std::vector<wxString> argv2;

                        if (i>=(int)argv.size())
                            break;

                        if (argv[i]==L"}")
                        {
                            Indent--;
                            if (!Stack.empty())
                            {
                                Inst=Stack.back();
                                Stack.pop_back();
                            }

                            break;
                        }

                        if (argv[i]==L"{")
                        {
                            Indent++;
                            i++;
                            continue;
                        }

                        Note.m_Name=argv[i];

                        i++;
                        if (i>=(int)argv.size())
                        {
                            Note.m_Data=L"";
                            Inst2=Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
                            if (Inst2!=NULL) // is this a sub section
                            {
                                Stack.push_back(Inst);
                                Inst=Inst2;
                            }

                            break;
                        }

                        if (argv[i]==L"}")
                        {
                            Note.m_Data=L"";
                            Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
                            Indent--;
                            break;
                        }

                        if (argv[i]==L"{")
                        {
                            Note.m_Data=L"";
                            for (i++; i<(int)argv.size(); i++)
                            {
                                if (argv[i]==L"}")
                                    break;
                                else
                                {
                                    if (!Note.m_Data.empty())
                                        Note.m_Data+=L" ";

                                    Note.m_Data+=argv[i];

                                    argv2.push_back(argv[i]);
                                }
                            }
                        }
                        else
                        {
                            argv2.push_back(argv[i]);
                            Note.m_Data=argv[i];
                        }

                        Inst2=Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
                        if (Inst2!=NULL) // is this a sub section
                        {
                            Stack.push_back(Inst);
                            Inst=Inst2;
                        }

                        i++;
                    }
                }
            }
        }
    }

    if ( !Stack.empty() )
    {
        wxLogError(L"Error: Object '%s' was not closed correctly", Inst->GetTreeName().c_str());
        Inst = Stack.front();
    }

    if (group!=NULL && group->m_Inst==NULL)
        group->m_Inst=Inst;

    return Inst;
}

//-----------------------------------------------------------------------------
int CIwAttrDescMgr::ReadPtrArray(CIwAttrInstance* Inst,std::vector<wxString>& argv,int arg,std::vector<LineStruct>& lines,int line)
{
    int Indent=0,num=0;
    std::vector<CIwAttrInstance*> Stack;
    CIwAttrData* data=Inst->FindData(argv[0]);
    CIwAttrInstance* Inst2;

    while (line<(int)lines.size())
    {
        while (true)
        {
            std::vector<wxString> argv2;
            CIwAttrNote Note;

            if (arg>=(int)argv.size())
                break;

            if (argv[arg]==L"}")
            {
                Indent--;
                if (!Stack.empty())
                {
                    Inst=Stack.back();
                    Stack.pop_back();
                }

                if (Indent==0)
                    return num;

                arg++;
                continue;
            }

            if (argv[arg]==L"{")
            {
                if (Indent==1)
                {
                    Inst2=new CIwAttrInstance;
                    Inst2->m_Mgr=this;
                    Inst2->m_ClassInfo=data->m_Instance->m_ClassInfo;
                    Inst2->m_Class=data->m_Member->m_Items[0].m_Class;
                    Inst2->AddDefaults(Inst2->m_Class);
                    Inst2->m_Parent=data;
                    Inst2->m_File=data->m_Instance->m_File;
                    Inst2->m_Dialog=data->m_Instance->m_Dialog;

                    CIwAttrMemberItem insert;
                    insert.m_Inst=Inst2;
                    insert.m_Flags=ATTRITEM_ALLOCED_F;
                    data->m_Items.push_back(insert);
                    data->m_FromDefault=false;

                    if (Inst2->m_File!=NULL)
                        Inst2->m_File->SetupInlinePtr(Inst2,data->m_Member);

                    Stack.push_back(Inst);
                    Inst=Inst2;
                }

                Indent++;
                arg++;
                continue;
            }

            Note.m_Name=argv[arg];

            arg++;
            if (arg>=(int)argv.size())
            {
                Note.m_Data=L"";
                Inst2=Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
                if (Inst2!=NULL) // is this a sub section
                {
                    Stack.push_back(Inst);
                    Inst=Inst2;
                }

                break;
            }

            if (argv[arg]==L"}")
            {
                Note.m_Data=L"";
                Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
                Indent--;
                break;
            }

            if (argv[arg]==L"{")
            {
                Note.m_Data=L"";
                for (arg++; arg<(int)argv.size(); arg++)
                {
                    if (argv[arg]==L"}")
                        break;
                    else
                    {
                        if (!Note.m_Data.empty())
                            Note.m_Data+=L" ";

                        Note.m_Data+=argv[arg];

                        argv2.push_back(argv[arg]);
                    }
                }
            }
            else
            {
                argv2.push_back(argv[arg]);
                Note.m_Data=argv[arg];
            }

            Inst2=Inst->AddFromNote(Note.m_Name,Note,argv2,Inst);
            if (Inst2!=NULL) // is this a sub section
            {
                Stack.push_back(Inst);
                Inst=Inst2;
            }

            arg++;
        }

        line++;
        num++;
        if (line==(int)lines.size())
            break;

        arg=0;
        argv.clear();
        for (int i=0; i<(int)lines[line].m_Args.size(); i++)
        {
            argv.push_back(lines[line].m_Args[i]);
        }
    }
    return num;
}

//-----------------------------------------------------------------------------
void CIwAttrDescMgr::IntegrateFromFile2(CIwAttrInstance* Inst,std::vector<wxString>& lines)
{
    //int FileNum=-2;
    int Indent=0,Ignore=0;
    int i;
    CIwAttrInstance* Inst2=Inst;
    std::vector<CIwAttrInstance*> Stack;

    for (int j=0; j<(int)lines.size(); j++)
    {
        std::vector<wxString> argv;
        if (SuperSplit(lines[j],argv,wxT(", \t\n"))<1)
            continue;

        if (argv[0].IsSameAs(wxT("{"),false))
        {
            if (Inst2==NULL) //if class was not found skip this section
                Ignore++;
            else
                Indent++;

            continue;
        }
        else if (argv[0].IsSameAs(wxT("}"),false))
        {
            if (Ignore>0)
                Ignore--;
            else
            {
                Indent--;
                if (!Stack.empty())
                {
                    Inst=Stack.back();
                    Stack.pop_back();
                }
            }

            continue;
        }

        if (Ignore>0) continue;

        // add data to current class
        CIwAttrNote Note;
        MakeNote(Note,argv);

        std::vector<wxString> argv2;
        for (i=1; i<(int)argv.size(); i++)
        {
            if (argv[i]!=L"{" && argv[i]!=L"}")
                argv2.push_back(argv[i]);
        }

        Inst2=Inst->AddFromNote(argv[0],Note,argv2,Inst);
        if (Inst2!=NULL) // is this a sub section
        {
            Stack.push_back(Inst);
            Inst=Inst2;
        }
    }
}

//-----------------------------------------------------------------------------
void CIwAttrDescMgr::IntegrateFromFile(const wxString& Path,CIwAttrInstance* Inst,bool ignoreName)
{
    int Indent=0;
    CIwAttrInstance* Inst2=NULL;
    wxString className;
    wxString instName;
    std::vector<wxString> lines;

    wxTextFile fp(Path);
    if (!fp.Exists() || !fp.Open())
        return;

    for (int j=0; j<(int)fp.GetLineCount(); j++)
    {
        std::vector<wxString> argv;
        if (SuperSplit(fp[j],argv,wxT(", \t\n"))<1)
            continue;

        if (argv[0].IsSameAs(wxT("{"),false))
        {
            Indent++;
            continue;
        }
        else if (argv[0].IsSameAs(wxT("}"),false))
        {
            Indent--;
            if (Indent==0)
            {
                if (ignoreName)
                    Inst2=Inst->FindChildClass(className);
                else
                    Inst2=Inst->FindChild(instName,className);

                if (Inst2!=NULL)
                {
                    if (Inst2->m_Class->m_Flags&ATTRCLASS_ORDERED_F)
                    {
                        for (int k=0; k<(int)Inst2->m_Data.size(); k++)
                        {
                            delete Inst2->m_Data[k];
                        }
                        Inst2->m_Data.clear();
                    }

                    IntegrateFromFile2(Inst2,lines);
                }

                instName.clear();
                className.clear();
                lines.clear();
            }

            continue;
        }

        if (Indent==0)
            className=argv[0];
        else if (argv[0].IsSameAs(L"name",false) && argv.size()>1)
            instName=argv[1];
        else
            lines.push_back(fp[j]);
    }
}


//-----------------------------------------------------------------------------
//create an instance from a list of notes, Type is what sort of thing it is
//if interactive return an instance of the first matching class when there is no explicit class
CIwAttrInstance* CIwAttrDescMgr::GetFromNotes(std::vector<CIwAttrNote>& Notes,CIwAttrExportType Type,bool Interactive,CIwAttrFileGroup* group)
{
    int i,j;

    std::vector<CIwAttrClass*> Classes;

    for (i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]!=NULL)
            for (j=0; j<(int)m_Descs[i]->m_ExportTypes[Type].size(); j++)
            {
                Classes.push_back(m_Descs[i]->m_ExportTypes[Type][j]);
            }
    }
    GetDerivedClasses(Classes);

    return GetFromNotes(Notes,Classes,Interactive,group);
}
//-----------------------------------------------------------------------------
//create an instance from a list of notes, Type is what sort of thing it is
//if interactive return an instance of the first matching class when there is no explicit class
CIwAttrInstance* CIwAttrDescMgr::GetFromNotes(std::vector<CIwAttrNote>& Notes,std::vector<CIwAttrClass*>& Classes,bool Interactive,CIwAttrFileGroup* group)
{
    int i,Num=0;
    CIwAttrClass* Class=NULL;
    CIwAttrClass* Found=NULL;
    int ClassInfo=-1;

    for (i=0; i<(int)Classes.size(); i++)
    {
        if ((Classes[i]->m_Flags&ATTRCLASS_BASE_F)==0)  //ignore base classes
            Num++;
    }
    if (Num==0)
        return NULL;

    //search for class <name> note
    for (i=0; i<(int)Notes.size(); i++)
    {
        if (Notes[i].m_Name.IsSameAs(wxT("class"),false))
        {
            wxString ClassName=wxT("C")+Notes[i].m_Data;

            Found=GetClass(ClassName);

            ClassInfo=Notes[i].m_Info;
            break;
        }
    }

    //find explicit or first class
    for (i=0; i<(int)Classes.size(); i++)
    {
        if ((Classes[i]->m_Flags&ATTRCLASS_BASE_F)==0)
        {
            if (Class==NULL && Interactive)
                Class=Classes[i];

            if (Classes[i]==Found)
            {
                Class=Found;
                break;
            }
        }
    }
    if (Class==NULL)
        return NULL;

    if (Class!=Found && Found!=NULL)
    {
        for (i=0; i<(int)Notes.size(); i++)
        {
            if (Notes[i].m_Name.IsSameAs(wxT("class"),false))
            {
                wxString ClassName=wxT("C")+Notes[i].m_Data;
                Notes[i].m_Name=ClassName;
                Notes[i].m_Data=L"{:100";

                break;
            }
        }
    }

    CIwAttrInstance* Inst=new CIwAttrInstance;
    Inst->m_File=group;
    Inst->m_Mgr=this;
    Inst->m_Class=Class;
    Inst->m_ClassInfo=ClassInfo;
    Inst->m_Parent=NULL;

    //actually fill the instance from the notes
    Inst->AddDefaults(Class);
    Inst->GetFromNotes(Notes);

    return Inst;
}

//-----------------------------------------------------------------------------
//load a description, slot 0 is the metabase.txt and slot 1 is expmetatemplate.txt
bool CIwAttrDescMgr::Load(const wxString& FileName,int Slot)
{
    if (Slot==-1)
    {
        Slot=m_Descs.size();
        m_Descs.push_back(new CIwAttrDesc(this));
    }
    else
        for (int i=m_Descs.size(); i<=Slot; i++)
        {
            m_Descs.push_back(new CIwAttrDesc(this));
        }

    m_Descs[Slot]->Clear();
    return m_Descs[Slot]->Load(FileName);
}

void CIwAttrDescMgr::GetAllMembersOfClass(const wxString& className,std::list<CIwAttrMember*>& members)
{
    members.clear();
    CIwAttrClass* pClass = CIwAttrDescMgr::GetClass(className);
    while ( pClass != NULL )
    {
        std::vector<CIwAttrMember*>::reverse_iterator memberIT;
        for ( memberIT = pClass->m_Members.rbegin(); memberIT != pClass->m_Members.rend(); ++memberIT )
        {
            members.push_front( *memberIT );
        }
        pClass = pClass->m_Parent;
    }
}


//-----------------------------------------------------------------------------
//temp structure for GetFromNotes
struct CIwAttrNoteLoader
{
    CIwAttrInstance*                    m_Inst;
    CIwAttrData*                        m_Data;
    std::vector<CIwAttrNote>            m_Notes;

    CIwAttrNoteLoader() : m_Inst(NULL), m_Data(NULL) {}
    CIwAttrNoteLoader(CIwAttrInstance* Inst) : m_Inst(Inst),m_Data(NULL) {}
};

//-----------------------------------------------------------------------------
//fill the instance from notes
//notes are of the format:
//	num: name value
//sub sections are of the from
//  num: class {num2
void CIwAttrInstance::GetFromNotes(std::vector<CIwAttrNote>& Notes)
{
    int i,j;
    CIwAttrInstance* Inst=this;

    std::vector<CIwAttrInstance*> Stack;
    std::map<int,CIwAttrNoteLoader> Insts;
    std::vector<int> Order;

    Insts[0]=CIwAttrNoteLoader(this);
    Order.push_back(0);

    //sort notes into sections per sub data
    for (i=0; i<(int)Notes.size(); i++)
    {
        if (Notes[i].m_ID!=-1)
        {
            if (Insts.find(Notes[i].m_ID)==Insts.end())
                Insts[Notes[i].m_ID]=CIwAttrNoteLoader(NULL);

            Insts[Notes[i].m_ID].m_Notes.push_back(Notes[i]);
        }
        else
            Insts[0].m_Notes.push_back(Notes[i]);
    }

    //for each section
    for (j=0; j<(int)Order.size(); j++)
    {
        CIwAttrNoteLoader& Loader=Insts[Order[j]];
        Inst=Loader.m_Inst;

        for (i=0; i<(int)Loader.m_Notes.size(); i++)
        {
            wxString Line=Loader.m_Notes[i].m_Data;
            int InstID=-1;

            if (Line[0]==wxT('{') && Line.size()>1)  //start of a sub section
            {
                InstID=atoi(Line.mb_str()+1);
                Line=wxT("{");
            }

            std::vector<wxString> argv;
            SuperSplit(Line,argv,wxT(" \t\n"));

            if (InstID!=-1 && Loader.m_Data!=NULL)
            {
                CIwAttrInstance* Inst2=new CIwAttrInstance;
                Inst2->m_Mgr=m_Mgr;
                Inst2->m_ClassInfo=Inst->m_ClassInfo;
                Inst2->m_Class=Loader.m_Data->m_Member->m_Items[0].m_Class;
                Inst2->AddDefaults(Inst2->m_Class);
                Inst2->m_Parent=Loader.m_Data;
                Inst2->m_File=Inst->m_File;
                Inst2->m_Dialog=Inst->m_Dialog;

                CIwAttrMemberItem insert;
                insert.m_Inst=Inst2;
                insert.m_Flags=ATTRITEM_ALLOCED_F;
                Loader.m_Data->m_Items.push_back(insert);

                if (Inst2->m_File!=NULL)
                    Inst2->m_File->SetupInlinePtr(Inst2,Loader.m_Data->m_Member);

                if (InstID!=-1)  //add sub section the the list to be processed
                {
                    Order.push_back(InstID);
                    if (Insts.find(InstID)==Insts.end())
                        Insts[InstID]=CIwAttrNoteLoader(Inst2);
                    else
                        Insts[InstID].m_Inst=Inst2;
                }
                else    //backwards compatability
                    Stack.push_back(Inst);
            }
            else if (Loader.m_Notes[i].m_Name.IsSameAs(wxT("}"),false)) //end of a sub section
            {
                if (!Stack.empty() && Order[j]==0)   //backwards compatability
                {
                    Inst=Stack.back();
                    Stack.pop_back();
                }
            }
            else
            {
                //do the actual data setup
                CIwAttrInstance* Inst2=Inst->AddFromNote(Loader.m_Notes[i].m_Name,Loader.m_Notes[i],argv,this);
                if (!argv.empty() && argv[0]==L"*")
                {
                    if (InstID!=-1)  //add sub section the the list to be processed
                    {
                        Order.push_back(InstID);
                        if (Insts.find(InstID)==Insts.end())
                            Insts[InstID]=CIwAttrNoteLoader(Inst);
                        else
                            Insts[InstID].m_Inst=Inst;

                        Insts[InstID].m_Data=Inst->FindData(Loader.m_Notes[i].m_Name);
                    }
                    else    //backwards compatability
                        Stack.push_back(Inst);
                }

                if (Inst2!=NULL)
                {
                    if (InstID!=-1)  //add sub section the the list to be processed
                    {
                        Order.push_back(InstID);
                        if (Insts.find(InstID)==Insts.end())
                            Insts[InstID]=CIwAttrNoteLoader(Inst2);
                        else
                            Insts[InstID].m_Inst=Inst2;
                    }
                    else    //backwards compatability
                    {
                        Stack.push_back(Inst);
                        Inst=Inst2;
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
//save to notes, change the class of an instance then read from the notes
void CIwAttrInstance::Reset(CIwAttrClass* Class)
{
    std::vector<CIwAttrNote> Notes;
    FillNotes(Notes,false);

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        delete m_Data[i];
    }
    m_Data.clear();
    m_ExtraData.clear();

    if (Class==NULL)
        return;

    m_Class=Class;
    AddDefaults(Class);
    GetFromNotes(Notes);
}

//-----------------------------------------------------------------------------
//setup data from a note
CIwAttrInstance* CIwAttrInstance::AddFromNote(const wxString& Name,CIwAttrNote& Note,std::vector<wxString>& argv,CIwAttrInstance* Base)
{
    int j;
    CIwAttrData* Data;
    CIwAttrData* GroupData=NULL;
    CIwAttrClass* Class=m_Mgr->GetClass(Name);
    CIwAttrMember* Member=NULL;
    CIwAttrMember* Group=NULL;
    int offset=0;

    //find value as a class or member
    if (Class!=NULL)
        Member=m_Class->GetClassMember(Class,&Group);

    if (Member==NULL && m_Class!=NULL)
        Member=m_Class->GetMember(Name,&Group);

    if (Member==NULL && m_Class!=NULL)
    {
        for (j=0; j<(int)m_Data.size(); j++)
        {
            if ((m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_DYNTEMPLATE)
            {
                Member=m_Data[j]->m_Items[0].m_Class->GetMember(Name,&Group);
                if (Member!=NULL)
                    break;
            }

            if ((m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_EXTRACLASS)
            {
                Member=m_Data[j]->m_Items[0].m_Class->GetMember(Name,&Group);
                if (Member!=NULL)
                    break;
            }

            if ((m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && m_Data[j]->m_Member->m_Items[m_Data[j]->m_Items[0].m_Int].m_Class!=NULL)
            {
                Member=m_Data[j]->m_Member->m_Items[m_Data[j]->m_Items[0].m_Int].m_Class->GetMember(Name,&Group);
                if (Member!=NULL)
                    break;
            }
        }
    }

    if (Member==NULL && m_File!=NULL)
    {
        Member=m_File->TryGetMember(Name,this);
        if (Member!=NULL)
            offset=1;
    }

    if (Member==NULL && argv.size()>0)
    {
        Class=m_Mgr->GetClass(argv[0]);
        if (Class!=NULL)
            Member=m_Class->GetClassMember(Class,&Group);

        if (Member!=NULL && Member->m_PreMember.empty())
            Member=NULL;

        if (Member!=NULL)
            offset=1;
        else
            Class=NULL;
    }

    if (Member==NULL)
    {
        //not found, add to extra data
        if (Note.m_Data==L"{")
        {
            //convert from name { to class name for extra notes
            Note.m_Data=Note.m_Name.Mid(1);
            Note.m_Name=wxT("class");
            m_ExtraData.push_back(Note);
            argv.clear();
            return this;    //push this inst so when } this inst is popped
        }

        if (!Name.IsSameAs(wxT("class"),false))
            m_ExtraData.push_back(Note);

        argv.clear();
        return NULL;
    }
    else if (Class==NULL && Note.m_Data==L"{")
    {
        Note.m_Data=L"";
        offset--;
    }

    for (j=0; j<(int)m_Data.size(); j++)
    {
        if (m_Data[j]->m_Member==Member)
            break;
    }

    bool more=false;
    more|=((Member->m_Type&ATTRMEMBER_1ORMORE)!=0);
    if (Group!=NULL)
        more|=((Group->m_Type&ATTRMEMBER_1ORMORE)!=0);

    more|=((Member->m_Parent->m_Flags&ATTRCLASS_ORDERED_F)!=0);

    if (m_File!=NULL)
        more=m_File->CheckAllowMultiple(this,Member,argv,more);

    //if we have got a member of this name already, do we add or replace
    if (j<(int)m_Data.size() && !more)
    {
        Data=m_Data[j];

        if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD || (Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
            delete Data->m_Items[0].m_Inst;
    }
    else
    {
        if (Group!=NULL && Group->m_Items[0].m_Member==Member) //if first item in group add group
        {
            GroupData=new CIwAttrData;
            GroupData->m_Instance=this;
            GroupData->m_Mgr=m_Mgr;
            GroupData->m_Group=NULL;
            GroupData->m_Member=Group;
            GroupData->m_Items.resize(1);
            if (Group->m_Type&ATTRMEMBER_HIDDEN)
                GroupData->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;
            else
                GroupData->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;

            m_Data.push_back(GroupData);
        }
        else    //find group
        {
            for (j=0; j<(int)m_Data.size(); j++)
            {
                if (m_Data[j]->m_Member==Group)
                    GroupData=m_Data[j];
            }
        }

        //create data
        Data=new CIwAttrData;
        Data->m_Instance=this;
        Data->m_Mgr=m_Mgr;
        Data->m_Group=GroupData;
        Data->m_Member=Member;
        Data->SetDefault(true);
        if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            Data->m_Items.resize(1);
            if (Member->m_Type&ATTRMEMBER_HIDDEN)
                Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;
            else
                Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
        }

        m_Data.push_back(Data);
    }

    Data->m_Info=Note.m_Info;

    for (j=0; j<(int)m_ExtraData.size(); j++)
    {
        if (m_ExtraData[j].m_Info==-2)
            Data->m_PreComments.push_back(m_ExtraData[j].m_Name+L" "+m_ExtraData[j].m_Data);
    }

    for (j=(int)m_ExtraData.size()-1; j>=0; j--)
    {
        if (m_ExtraData[j].m_Info==-2)
            m_ExtraData.erase(m_ExtraData.begin()+j);
    }

    if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
        Class=Member->m_Items[0].m_Class;

    if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD || (Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS) //we are a sub section create sub instance
    {
        CIwAttrInstance* Inst=new CIwAttrInstance;
        Inst->m_Mgr=m_Mgr;
        Inst->m_ClassInfo=Note.m_Info;
        Inst->m_Class=Class;
        Inst->AddDefaults(Class);
        Inst->m_Parent=Data;
        Inst->m_File=m_File;
        Inst->m_Dialog=m_Dialog;

        Data->m_Items.resize(1);
        Data->m_Items[0].m_Inst=Inst;
        Data->m_Items[0].m_Flags=ATTRITEM_ALLOCED_F;

        if (!Member->m_PreMember.empty() && offset==1)
        {
            std::vector<wxString> argv2;

            Note.m_Name=Member->m_PreMember;
            Note.m_Data=Name;
            argv2.push_back(Name);
            Inst->AddFromNote(Member->m_PreMember,Note,argv2,this);
        }

        if (!Member->m_PreMember.empty() && offset==0)
            Inst->AddFromNote(Member->m_PreMember,Note,argv,this);

        return Inst;
    }

    if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
    {
        if (Data->m_Member->m_Type&ATTRMEMBER_LIST)
        {
            argv.insert(argv.begin(),L"*");

            return NULL;
        }

        //find what is pointing to
        Data->m_Items.resize(1);
        Data->m_FromDefault=false;

        //inline
        if (argv.empty() || argv[0]==L"{")
        {
            CIwAttrInstance* Inst=new CIwAttrInstance;
            Inst->m_Mgr=m_Mgr;
            Inst->m_ClassInfo=Note.m_Info;
            Inst->m_Class=Member->m_Items[0].m_Class;
            Inst->AddDefaults(Inst->m_Class);
            Inst->m_Parent=Data;
            Inst->m_File=m_File;
            Inst->m_Dialog=m_Dialog;

            Data->m_Items[0].m_Inst=Inst;
            Data->m_Items[0].m_Flags=ATTRITEM_ALLOCED_F;

            if (m_File!=NULL)
                m_File->SetupInlinePtr(Inst,Member);

            return Inst;
        }
        else
        {
            if (m_File!=NULL)
                Data->m_Items[0].m_Inst=m_File->TryGetPtrItem(argv[0],Data);

            if (Data->m_Items[0].m_Inst==NULL)
                Data->m_Items[0].m_Inst=Base->FindChild(argv[0]);

            Data->m_Items[0].m_String=argv[0];
            Data->m_Items[0].m_Flags=0;

            argv.erase(argv.begin());
        }
    }
    else if (argv.size()+offset>0)  //set data from line
    {
        // check for bracketed data
        std::vector<wxString> argv2;
        int end=argv.size()-1;
        int indent=0;

        if (offset==1)
            argv2.push_back(Name);

        for (j=0; j<(int)argv.size(); j++)
        {
            if (argv[j]==L"{")
            {
                indent++;
                continue;
            }

            if (argv[j]==L"}")
            {
                indent--;
                if (indent==0)
                {
                    end=j;
                    break;
                }

                continue;
            }

            argv2.push_back(argv[j]);
        }

        Data->Set(argv2,Base);
        argv.erase(argv.begin(),argv.begin()+end+1);
    }

    if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        AddGroupDefaults(Member,Data);

    if ((Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && Member->m_Items[0].m_Class!=NULL)
    {
        if (Data->m_Items[0].m_Int!=0)
        {
            RemoveDefaults(Member->m_Items[0].m_Class);
            AddDefaults(Member->m_Items[Data->m_Items[0].m_Int].m_Class,GroupData,true);
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
//setup all members of a group
void CIwAttrInstance::AddGroupDefaults(CIwAttrMember* Member,CIwAttrData* Group)
{
    for (int i=0; i<(int)Member->m_Items.size(); i++)
    {
        if (Member->m_Items[i].m_Member->m_Type&ATTRMEMBER_0OR1)
            continue;

        if ((Member->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE && Member->m_Items[0].m_Class!=NULL)
        {
            AddDefaults(Member->m_Items[i].m_Member->m_Items[0].m_Class,Group,true);
            continue;
        }

        CIwAttrData* Data=new CIwAttrData;
        Data->m_Mgr=m_Mgr;
        Data->m_Instance=this;
        Data->m_Member=Member->m_Items[i].m_Member;
        Data->m_Info=-1;
        Data->m_Group=Group;
        Data->m_FromDefault=true;
        Data->SetDefault();
        m_Data.push_back(Data);

        if (m_Class->IsTemplateMember(Member->m_Items[i].m_Member))
            AddDefaults(Member->m_Items[i].m_Member->m_Items[0].m_Class,Group,true);

        if ((Member->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && Member->m_Items[i].m_Member->m_Items[0].m_Class!=NULL)
            AddDefaults(Member->m_Items[i].m_Member->m_Items[0].m_Class,Group,true);

        if ((Member->m_Items[i].m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
            AddGroupDefaults(Member->m_Items[i].m_Member,Data);
    }
}

//-----------------------------------------------------------------------------
//delete default data in an instance
void CIwAttrInstance::RemoveDefaults(CIwAttrClass* Class)
{
    if (Class==NULL)
        return;

    if (Class->m_Parent!=NULL)
        RemoveDefaults(Class->m_Parent);

    if (Class->m_Flags&ATTRCLASS_ORDERED_F)
        return;

    for (int i=0; i<(int)Class->m_Members.size(); i++)
    {
        int j;

        for (j=0; j<(int)m_Data.size(); j++)
        {
            if (m_Data[j]->m_Member->m_Name.IsSameAs(Class->m_Members[i]->m_Name,false))
            {
                if (m_Data[j]->m_FromDefault)
                {
                    delete m_Data[j];
                    m_Data.erase(m_Data.begin()+j);
                }

                break;
            }
        }
        if (j<(int)m_Data.size())
            continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_0OR1)
            continue;

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
            RemoveDefaults(Class->m_Members[i]->m_Items[0].m_Class);
    }
}

//-----------------------------------------------------------------------------
//set default data in an instance
void CIwAttrInstance::AddDefaults(CIwAttrClass* Class,CIwAttrData* group,bool inExtraClass)
{
    if (Class==NULL)
        return;

    if (Class->m_Parent!=NULL)
        AddDefaults(Class->m_Parent,group,inExtraClass);

    if (Class->m_Flags&ATTRCLASS_ORDERED_F)
        return;

    for (int i=0; i<(int)Class->m_Members.size(); i++)
    {
        int j;
        CIwAttrData* Data;

        for (j=0; j<(int)m_Data.size(); j++)
        {
            if (m_Data[j]->m_Member->m_Name.IsSameAs(Class->m_Members[i]->m_Name,false))
                break;
        }
        if (j<(int)m_Data.size())
            continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_0OR1)
            continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_NOTINEXTRACLASS && inExtraClass)
            continue;

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
        {
            AddDefaults(Class->m_Members[i]->m_Items[0].m_Class,group,inExtraClass);
            continue;
        }

        Data=new CIwAttrData;
        Data->m_Mgr=m_Mgr;
        Data->m_Instance=this;
        Data->m_Member=Class->m_Members[i];
        Data->m_Info=-1;
        Data->m_Group=group;
        Data->m_FromDefault=true;
        Data->SetDefault();
        m_Data.push_back(Data);

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
        {
            Data->m_Items.resize(1);
            if (Class->m_Members[i]->m_Type&ATTRMEMBER_HIDDEN)
                Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;
            else
                Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;

            AddGroupDefaults(Class->m_Members[i],Data);
        }

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
            AddDefaults(Class->m_Members[i]->m_Items[0].m_Class,group,true);
    }
}

//-----------------------------------------------------------------------------
//find all derived classes
void CIwAttrDescMgr::GetDerivedClasses(std::vector<CIwAttrClass*>& Classes)
{
    int From=0;
    int To=Classes.size();
    while (true)
    {
        for (int k=From; k<To; k++)
        {
            for (int i=0; i<(int)m_Descs.size(); i++)
            {
                if (m_Descs[i]!=NULL)
                {
                    for (int j=0; j<(int)m_Descs[i]->m_Classes.size(); j++)
                    {
                        if (m_Descs[i]->m_Classes[j]->m_Parent!=NULL)
                            if (m_Descs[i]->m_Classes[j]->m_Parent==Classes[k])
                                Classes.push_back(m_Descs[i]->m_Classes[j]);

                    }
                }
            }
        }

        if (To==(int)Classes.size())
            return;

        From=To;
        To=Classes.size();
    }
}

//-----------------------------------------------------------------------------
//convert class list to wx string array
void CIwAttrDescMgr::GetClassStrings(std::vector<CIwAttrClass*>& Classes,wxArrayString& Strings)
{
    for (int i=0; i<(int)Classes.size(); i++)
    {
        if (Classes[i]!=NULL)
            if ((Classes[i]->m_Flags&ATTRCLASS_BASE_F)==0)
                Strings.push_back(Classes[i]->m_Name);

    }
}

//-----------------------------------------------------------------------------
//get all classes available for a particular type of object
void CIwAttrDescMgr::GetClasses(std::vector<CIwAttrClass*>& Classes,CIwAttrExportType Type)
{
    int i,j;

    for (i=0; i<(int)m_Descs.size(); i++)
    {
        if (m_Descs[i]!=NULL)
            for (j=0; j<(int)m_Descs[i]->m_ExportTypes[Type].size(); j++)
            {
                Classes.push_back(m_Descs[i]->m_ExportTypes[Type][j]);
            }
    }
    GetDerivedClasses(Classes);
}

//-----------------------------------------------------------------------------
wxString CIwAttrDescMgr::GetMaterials()
{
    int i,k;
    wxString result;
    std::vector<CIwAttrClass*> Classes;

    Classes.push_back(GetClass(L"CIwFxParameters"));
    GetDerivedClasses(Classes);

    for (i=1; i<(int)Classes.size(); i++)
    {
        if (!result.empty()) result+=L";";

        result+=Classes[i]->m_Name;

        std::map<wxString,wxString>::iterator it;
        for (it=Classes[i]->m_Paths.begin(); it!=Classes[i]->m_Paths.end(); ++it)
        {
            result+=wxString::Format(L",!%s %s",it->first.c_str(),it->second.c_str());
        }

        for (CIwAttrClass* klass=Classes[i]; klass!=NULL; klass=klass->m_Parent)
        {
            for (k=0; k<(int)klass->m_Members.size(); k++)
            {
                switch (klass->m_Members[k]->m_Type&ATTRMEMBER_MASK)
                {
                case ATTRMEMBER_INT:
                    result+=wxString::Format(L",I%d ",klass->m_Members[k]->m_ArraySize);
                    break;
                case ATTRMEMBER_FLOAT:
                    result+=wxString::Format(L",F%d ",klass->m_Members[k]->m_ArraySize);
                    break;
                case ATTRMEMBER_FILETEXTURE:
                    result+=wxString::Format(L",T%d ",klass->m_Members[k]->m_ArraySize);
                    break;
                default:
                    continue;
                }
                result+=klass->m_Members[k]->m_Name;
            }
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
//	check the mebers for export "useGeo" field and traverse back up parent hierarchy
bool CIwAttrDescMgr::DoesClassContainUseGeo(const wxString& name)
{
    CIwAttrClass *pClass = GetClass(name);
    return DoesClassContainUseGeo(pClass);
}

bool CIwAttrDescMgr::DoesClassContainUseGeo(CIwAttrClass *pClass)
{
    bool result=false;
    while (pClass!=NULL)
    {
        unsigned numMembers=pClass->m_Members.size();
        for (unsigned loop=0; loop<numMembers; loop++)
        {
            if (pClass->IsTemplateMember(pClass->m_Members[loop]))
                result |= DoesClassContainUseGeo( pClass->m_Members[loop]->m_Items[0].m_Class );
            else
            {
                if (pClass->m_Members[loop]->m_ExportSection.IsSameAs(wxT("useGeo"),false))
                {
                    result = true;  // or add to list of classes that export?
                }
            }
        }
        pClass = pClass->m_Parent;
    }
    return result;

}


//-----------------------------------------------------------------------------
//returns true and adds correct file extension to name if type is valid
bool CIwAttrDescMgr::GetMemberFiletype(const wxString& type,wxString& name)
{
    for (int i=0; i<(int)m_Descs.size(); i++)
    {
        for (int j=0; j<(int)m_Descs[i]->m_ViewerFields.size(); j++)
        {
            if (m_Descs[i]->m_ViewerFields[j]->m_Ext.size()>0)
            {
                bool Found=false;
                if (m_Descs[i]->m_ViewerFields[j]->m_Primary.IsSameAs(type,false))
                    Found=true;
                else
                    for (int k=0; k<(int)m_Descs[i]->m_ViewerFields[j]->m_Alternatives.size(); k++)
                    {
                        if (m_Descs[i]->m_ViewerFields[j]->m_Alternatives[k].IsSameAs(type,false))
                            Found=true;
                    }

                if (!Found) continue;

                name+=m_Descs[i]->m_ViewerFields[j]->m_Ext;
                return true;
            }
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
//make a pretty version of a name: is_myHelp   mate -> Is My Help Mate
wxString CIwAttrData::MakeName(const wxString& From)
{
    wxString To;
    bool IsSpace=true;

    for (int i=0; i<(int)From.size(); i++)
    {
        if (IsSpace && (From[i]==wxT(' ') || From[i]==wxT('\t')))
            continue;

        if (From[i]==wxT(' ') || From[i]==wxT('\t'))
        {
            To.append(1,wxT(' '));
            IsSpace=true;
        }
        else if (From[i]==wxT('_'))
        {
            To.append(1,wxT(' '));
            IsSpace=true;
        }
        else if (IsSpace)
        {
            To.append(1,toupper(From[i]));
            IsSpace=false;
        }
        else
        {
            if (From[i]>=wxT('A') && From[i]<=wxT('Z'))
                To.append(1,wxT(' '));

            To.append(1,From[i]);
        }
    }
    return To;
}

//-----------------------------------------------------------------------------
//get pretty version of name, generating if not setup
//also generates pretty versions of enum values
wxString CIwAttrData::GetName()
{
    if (m_Member->m_Text.empty())
    {
        if ((m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM)
            m_Member->m_Text.resize(m_Member->m_ArraySize+1);
        else
            m_Member->m_Text.resize(1);

        m_Member->m_Text[0]=MakeName(m_Member->m_Name);

        if ((m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM)
            for (int i=0; i<m_Member->m_ArraySize; i++)
            {
                m_Member->m_Text[i+1]=MakeName(m_Member->m_Items[i].m_String);
            }
    }

    if (!m_Member->m_Text.empty() && !m_Member->m_Text[0].empty())
    {
        bool notLive=false;
        bool notPosLive=false;
        if (CIwAttrDescMgr::s_LiveEditing)
        {
            notLive=m_Member->m_ViewerDead;
            notPosLive=(m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM;
        }

        if (!notLive && m_Member->m_Text[0][m_Member->m_Text[0].size()-1]=='*')
            m_Member->m_Text[0].RemoveLast(2);

        if (!notPosLive && m_Member->m_Text[0][m_Member->m_Text[0].size()-1]=='#')
            m_Member->m_Text[0].RemoveLast(2);

        if (notLive && m_Member->m_Text[0][m_Member->m_Text[0].size()-1]!='*')
            m_Member->m_Text[0]+=L" *";

        if (!notLive && notPosLive && m_Member->m_Text[0][m_Member->m_Text[0].size()-1]!='#')
            m_Member->m_Text[0]+=L" #";
    }

    return m_Member->m_Text[0];
}

//-----------------------------------------------------------------------------
//is this data the same as the default value (to optimise output)
bool CIwAttrData::IsDefault()
{
    if (m_Member->m_Type&ATTRMEMBER_LIST)
        return m_Items.empty();

    for (int i=0; i<m_Member->m_ArraySize; i++)
    {
        switch (m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_ENUM:
            if (m_Items[i].m_Int!=0)
                return false;

            break;
        case ATTRMEMBER_PTR:
            if (m_Items[i].m_Inst!=NULL)
                return false;

            break;
        case ATTRMEMBER_RESOURCE:
        case ATTRMEMBER_CLASS:
        case ATTRMEMBER_TEMPLATE:
        case ATTRMEMBER_GROUP:
        case ATTRMEMBER_CHILD:
            return false;
        case ATTRMEMBER_DYNTEMPLATE:
        case ATTRMEMBER_EXTRACLASS:
            return m_Items[0].m_Class==m_Member->m_Items[0].m_Class;
        case ATTRMEMBER_BOOL:
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
        case ATTRMEMBER_INT124:
        case ATTRMEMBER_STRINGID:
        case ATTRMEMBER_COLOUR:
        case ATTRMEMBER_COLOUR3:
        case ATTRMEMBER_ALIGNMENT:
            if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            {
                if (m_Items[i].m_UInt!=m_Member->m_Items[i].m_UInt)
                    return false;
            }
            else if (m_Items[i].m_Int!=m_Member->m_Items[i].m_Int)
                return false;

            break;
        case ATTRMEMBER_FLOAT:
            if (m_Items[i].m_Float!=m_Member->m_Items[i].m_Float)
                return false;

            break;
        case ATTRMEMBER_STRING:
        case ATTRMEMBER_USETEMPLATE:
        case ATTRMEMBER_DATA:
        case ATTRMEMBER_FILE:
        case ATTRMEMBER_FILETEXTURE:
        case ATTRMEMBER_FILEFONT:
            if (!m_Items[i].m_String.IsSameAs(m_Member->m_Items[i].m_String,false))
                return false;

            break;
        case ATTRMEMBER_COORD:
            if (m_Items[i].m_Coord.m_UseInt!=m_Member->m_Items[i].m_Coord.m_UseInt)
                return false;

            if (m_Items[i].m_Coord.m_UseInt)
            {
                if (m_Items[i].m_Coord.m_Int!=m_Member->m_Items[i].m_Coord.m_Int)
                    return false;
            }
            else if (m_Items[i].m_Coord.m_Float!=m_Member->m_Items[i].m_Coord.m_Float)
                return false;

            break;
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
//convert data to string (for notes)
CIwAttrInstance* CIwAttrData::ToString(wxString& Line,int Num)
{
    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_LIST)
    {
        for (int i=0; i<(int)m_Items.size(); i++)
        {
            ToString(Line,i);
        }

        return NULL;
    }

    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_ARRAY)
    {
        for (int i=0; i<m_Member->m_ArraySize; i++)
        {
            ToString(Line,i);
        }

        return NULL;
    }

    if (Num==-1)
        Num=0;

    if (Num!=0)
        Line+=wxT(" ");

    if (m_Member->m_Type&ATTRMEMBER_NULLABLE)
        Line+=wxT("!");

    switch (m_Member->m_Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_BOOL:
        if (m_Items[Num].m_Int==0)
            Line+=wxT("false");
        else
            Line+=wxT("true");

        break;
    case ATTRMEMBER_BYTE:
    case ATTRMEMBER_SHORT:
    case ATTRMEMBER_INT:
    case ATTRMEMBER_INT124:
    case ATTRMEMBER_STRINGID:
    case ATTRMEMBER_ALIGNMENT:
    case ATTRMEMBER_ENUM:
        if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_UInt);
        else
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Int);

        break;
    case ATTRMEMBER_FLOAT:
        Line+=CleanFloat(m_Items[Num].m_Float);
        break;
    case ATTRMEMBER_RESOURCE:
        Line+=m_Items[0].m_String;
        if (m_Items[1].m_Inst!=NULL)
        {
            Line+=L"{ ";
            for (int i=0; i<(int)m_Items[1].m_Inst->m_Data.size(); i++)
            {
                Line+=m_Items[1].m_Inst->m_Data[i]->m_Member->m_Name+L" ";
                m_Items[1].m_Inst->m_Data[i]->ToString(Line);
                Line+=L" ";
            }
            Line+=L"}";
        }

        break;
    case ATTRMEMBER_STRING:
    case ATTRMEMBER_FILE:
    case ATTRMEMBER_FILETEXTURE:
    case ATTRMEMBER_FILEFONT:
        Line+=wxString::Format(wxT("\"%s\""),m_Items[Num].m_String.c_str());
        break;
    case ATTRMEMBER_DATA:
        Line+=m_Items[Num].m_String;
        break;
    case ATTRMEMBER_DYNTEMPLATE:
    case ATTRMEMBER_EXTRACLASS:
        Line+=m_Items[Num].m_Class->m_Name;
        break;
    case ATTRMEMBER_USETEMPLATE:
        Line+=m_Items[0].m_String+L" "+m_Items[1].m_String;
        break;

    case ATTRMEMBER_COLOUR:
        Line+=wxString::Format(wxT("%d %d %d %d"),m_Items[0].m_Int,m_Items[1].m_Int,m_Items[2].m_Int,m_Items[3].m_Int);
        break;
    case ATTRMEMBER_COLOUR3:
        Line+=wxString::Format(wxT("%d %d %d"),m_Items[0].m_Int,m_Items[1].m_Int,m_Items[2].m_Int);
        break;
    case ATTRMEMBER_COORD:
        if (m_Items[Num].m_Coord.m_UseInt)
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Coord.m_Int);
        else
            Line+=CleanFloat(m_Items[Num].m_Coord.m_Float);

        break;
    case ATTRMEMBER_PTR:
        if (m_Items[Num].m_Inst!=NULL)
        {
            if (m_Items[Num].m_Flags&ATTRITEM_ALLOCED_F)
            {
                Line+=wxT("{");
                return m_Items[Num].m_Inst;
            }
            else
            {
                CIwAttrData* data=m_Items[Num].m_Inst->FindData(L"name");
                if (data!=NULL)
                    Line+=data->m_Items[0].m_String;
                else
                    Line+=wxT(" ");
            }
        }
        else
            Line+=wxT(" ");

        break;
    case ATTRMEMBER_CHILD:
    case ATTRMEMBER_CLASS:
        Line+=wxT("{");
        return m_Items[Num].m_Inst;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//convert data to string (for file export), limiting int values to specified limits
CIwAttrInstance* CIwAttrData::ToString2(wxString& Line,int Num)
{
    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_LIST)
    {
        Line+=wxT(" { ");
        for (int i=0; i<(int)m_Items.size(); i++)
        {
            ToString2(Line,i);
        }
        Line+=wxT(" }");
        return NULL;
    }

    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_ARRAY)
    {
        int i,k=0;
        for (int j=0; j<(int)m_Member->m_Boundary.size(); j++)
        {
            Line+=wxT(" { ");
            for (i=0; i<m_Member->m_Boundary[j]; i++)
            {
                ToString2(Line,i+k);
            }
            k+=i;
            Line+=wxT(" }");
        }
        return NULL;
    }

    if (Num==-1)
        Num=0;

    if (Num!=0)
        Line+=wxT(" ");

    switch (m_Member->m_Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_BOOL:
        if (m_Items[Num].m_Int==0)
            Line+=wxT("false");
        else
            Line+=wxT("true");

        break;
    case ATTRMEMBER_BYTE:
        if (m_Member->m_Limits[2]!=0)
        {
            if (m_Items[Num].m_Int<m_Member->m_Limits[0]) m_Items[Num].m_Int=m_Member->m_Limits[0];

            if (m_Items[Num].m_Int>m_Member->m_Limits[1]) m_Items[Num].m_Int=m_Member->m_Limits[1];

            m_Items[Num].m_Int=(m_Items[Num].m_Int/m_Member->m_Limits[2])*m_Member->m_Limits[2];
        }
        else if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            if (m_Items[Num].m_UInt>255) m_Items[Num].m_UInt=255;

            if (m_Items[Num].m_UInt<0) m_Items[Num].m_UInt=0;
        }
        else
        {
            if (m_Items[Num].m_Int>127) m_Items[Num].m_Int=127;

            if (m_Items[Num].m_Int<-128) m_Items[Num].m_Int=-128;
        }

        if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            Line+=wxString::Format(wxT("%u"),m_Items[Num].m_UInt);
        else
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Int);

        break;
    case ATTRMEMBER_SHORT:
        if (m_Member->m_Limits[2]!=0)
        {
            if (m_Items[Num].m_Int<m_Member->m_Limits[0]) m_Items[Num].m_Int=m_Member->m_Limits[0];

            if (m_Items[Num].m_Int>m_Member->m_Limits[1]) m_Items[Num].m_Int=m_Member->m_Limits[1];

            m_Items[Num].m_Int=(m_Items[Num].m_Int/m_Member->m_Limits[2])*m_Member->m_Limits[2];
        }
        else if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            if (m_Items[Num].m_UInt>65535) m_Items[Num].m_UInt=65535;

            if (m_Items[Num].m_UInt<0) m_Items[Num].m_UInt=0;
        }
        else
        {
            if (m_Items[Num].m_Int>32767) m_Items[Num].m_Int=32767;

            if (m_Items[Num].m_Int<-32768) m_Items[Num].m_Int=-32768;
        }

        if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            Line+=wxString::Format(wxT("%u"),m_Items[Num].m_UInt);
        else
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Int);

        break;
    case ATTRMEMBER_INT124:
    case ATTRMEMBER_INT:
        if (m_Member->m_Limits[2]!=0)
        {
            if (m_Items[Num].m_Int<m_Member->m_Limits[0]) m_Items[Num].m_Int=m_Member->m_Limits[0];

            if (m_Items[Num].m_Int>m_Member->m_Limits[1]) m_Items[Num].m_Int=m_Member->m_Limits[1];

            m_Items[Num].m_Int=(m_Items[Num].m_Int/m_Member->m_Limits[2])*m_Member->m_Limits[2];
        }

        if (m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            Line+=wxString::Format(wxT("%u"),m_Items[Num].m_UInt);
        else
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Int);

        break;
    case ATTRMEMBER_STRINGID:
        Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Int);
        break;
    case ATTRMEMBER_FLOAT:
        Line+= CleanFloat(m_Items[Num].m_Float);
        break;
    case ATTRMEMBER_ALIGNMENT:
        switch (m_Items[Num].m_Int&0xf)
        {
        case 0:
            Line+=wxT("{ left ");   break;
        case 1:
            Line+=wxT("{ centre "); break;
        case 2:
            Line+=wxT("{ right ");  break;
        }
        switch (m_Items[Num].m_Int>>4)
        {
        case 0:
            Line+=wxT("top }"); break;
        case 1:
            Line+=wxT("centre }");  break;
        case 2:
            Line+=wxT("bottom }");  break;
        }
        break;
    case ATTRMEMBER_ENUM:
        Line+=wxString::Format(wxT("\"%s\""),m_Member->m_Items[m_Items[Num].m_Int].m_String.c_str());
        break;
    case ATTRMEMBER_RESOURCE:
        Line+=wxString::Format(wxT("\"%s\""),m_Items[Num].m_String.c_str());
        if (m_Items[1].m_Inst!=NULL)
        {
            Line+=L"{ ";
            for (int i=0; i<(int)m_Items[1].m_Inst->m_Data.size(); i++)
            {
                Line+=m_Items[1].m_Inst->m_Data[i]->m_Member->m_Name+L" ";
                m_Items[1].m_Inst->m_Data[i]->ToString2(Line);
                Line+=L" ";
            }
            Line+=L"}";
        }

        break;
    case ATTRMEMBER_STRING:
    case ATTRMEMBER_FILE:
    case ATTRMEMBER_FILETEXTURE:
    case ATTRMEMBER_FILEFONT:
        Line+=wxString::Format(wxT("\"%s\""),m_Items[Num].m_String.c_str());
        break;
    case ATTRMEMBER_DYNTEMPLATE:
    case ATTRMEMBER_EXTRACLASS:
        Line+=wxString::Format(wxT("\"%s\""),m_Items[Num].m_Class->m_Name.c_str());
        break;
    case ATTRMEMBER_USETEMPLATE:
        Line+=wxString::Format(wxT("\"%s\" \"%s\""),m_Items[0].m_String.c_str(),m_Items[1].m_String.c_str());
        break;
    case ATTRMEMBER_DATA:
        Line+=m_Items[Num].m_String;
        break;

    case ATTRMEMBER_COLOUR:
        Line+=wxString::Format(wxT("{ %d %d %d %d }"),m_Items[0].m_Int,m_Items[1].m_Int,m_Items[2].m_Int,m_Items[3].m_Int);
        break;
    case ATTRMEMBER_COLOUR3:
        Line+=wxString::Format(wxT("{ %d %d %d }"),m_Items[0].m_Int,m_Items[1].m_Int,m_Items[2].m_Int);
        break;
    case ATTRMEMBER_COORD:
        if (m_Items[Num].m_Coord.m_UseInt)
            Line+=wxString::Format(wxT("%d"),m_Items[Num].m_Coord.m_Int);
        else
            Line+=CleanFloat(m_Items[Num].m_Coord.m_Float);

        break;

    case ATTRMEMBER_PTR:
        if (m_Items[Num].m_Inst!=NULL)
        {
            if (m_Items[Num].m_Flags&ATTRITEM_ALLOCED_F)
                return m_Items[Num].m_Inst;
            else
            {
                CIwAttrData* data=m_Items[Num].m_Inst->FindData(L"name");
                if (data!=NULL)
                    Line+=data->m_Items[0].m_String;
                else
                    Line+=wxT(" ");
            }
        }
        else
            Line+=wxT(" \"\"");

        break;
    case ATTRMEMBER_CHILD:
    case ATTRMEMBER_CLASS:
        if (!m_Member->m_PreMember.empty() && m_Items[Num].m_Inst!=NULL)
        {
            CIwAttrData* data=m_Items[Num].m_Inst->FindData(m_Member->m_PreMember,0);
            if (data!=NULL)
                data->ToString2(Line);
        }

        return m_Items[Num].m_Inst;
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//bool data control wxCheckBox
class CIwAttrCheckBox : public CIwStyleCheckOD
{
    CIwAttrData* m_Data;
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
    wxWindow* m_Controlled;
    enum { CTRLID_CHECK };
public:
    CIwAttrCheckBox(wxWindow* Parent,CIwAttrData* Data,int Num,const wxString& Label,std::vector<CIwAttrData*>& Alts,wxWindow* controlled=NULL) :
        CIwStyleCheckOD(Parent,CTRLID_CHECK,Label,wxSize(-1,-1),wxCHK_3STATE),m_Data(Data),m_Num(Num),m_Alts(Alts),m_Controlled(controlled)
    {
        int j;
        if (m_Controlled!=NULL)
            m_Data->m_Items[m_Num].m_Int=((m_Data->m_Items[m_Num].m_Flags&ATTRITEM_OVERRIDE_F)==ATTRITEM_OVERRIDE_F) ? 1 : 0;

        for (j=0; j<(int)m_Alts.size(); j++)
        {
            if (m_Controlled!=NULL)
                m_Alts[j]->m_Items[m_Num].m_Int=((m_Alts[j]->m_Items[m_Num].m_Flags&ATTRITEM_OVERRIDE_F)==ATTRITEM_OVERRIDE_F) ? 1 : 0;

            if (m_Data->m_Items[m_Num].m_Int!=m_Alts[j]->m_Items[m_Num].m_Int)
            {
                Set3StateValue(wxCHK_UNDETERMINED);
                break;
            }
        }

        if (j==(int)m_Alts.size())
            SetValue(m_Data->m_Items[m_Num].m_Int==1);

        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Select %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Select %s here"),Data->GetName().c_str()));

        if (m_Controlled!=NULL)
            m_Controlled->Enable(Get3StateValue()==wxCHK_CHECKED);
    }

    void OnCheck(wxCommandEvent&)
    {
        int i;
        switch (Get3StateValue())
        {
        case wxCHK_UNCHECKED:
            if (m_Controlled!=NULL)
            {
                m_Controlled->Enable(false);

                m_Data->m_Items[m_Num].m_Flags&=~ATTRITEM_OVERRIDE_F;
                for (i=0; i<(int)m_Alts.size(); i++)
                {
                    m_Alts[i]->m_Items[m_Num].m_Flags&=~ATTRITEM_OVERRIDE_F;
                }
            }

            m_Data->m_Items[m_Num].m_Int=0;
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Int=0;
            }
            break;
        case wxCHK_CHECKED:
            if (m_Controlled!=NULL)
            {
                m_Controlled->Enable(true);

                m_Data->m_Items[m_Num].m_Flags|=ATTRITEM_OVERRIDE_F;
                for (i=0; i<(int)m_Alts.size(); i++)
                {
                    m_Alts[i]->m_Items[m_Num].m_Flags|=ATTRITEM_OVERRIDE_F;
                }
            }

            m_Data->m_Items[m_Num].m_Int=1;
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Int=1;
            }
            break;
        default:
            break;
        }
        m_Data->SetChanged();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrCheckBox, CIwStyleCheckOD)
    EVT_CHECKBOX(CTRLID_CHECK,CIwAttrCheckBox::OnCheck)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//string data control
class CIwAttrText : public wxTextCtrl
{
protected:
    std::vector<CIwAttrData*> m_Alts;
    CIwAttrData* m_Data;
    int m_Num;
    bool m_Updating;
    enum { CTRLID_TEXT };
public:
    CIwAttrText(wxWindow* Parent,CIwAttrData* Data,int Num,int Width,std::vector<CIwAttrData*>& Alts,bool IsString=true) :
        wxTextCtrl(Parent,CTRLID_TEXT,IsString ? Data->m_Items[Num].m_String : wxT(""),wxPoint(-1,-1),wxSize(Width,-1),wxTE_RICH),
        m_Alts(Alts),m_Data(Data),m_Num(Num),m_Updating(true)
    {
        if (m_Data->m_Member->m_Type&ATTRMEMBER_READONLY)
            if (m_Data->m_Member->m_Text.size()>1)
                SetValue(m_Data->m_Member->m_Text[1]);

        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Select %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Select %s here"),Data->GetName().c_str()));

        if (!IsString) return;

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            if (!m_Alts[i]->m_Items[Num].m_String.IsSameAs(Data->m_Items[Num].m_String,false))
            {
                SetValue(wxT(""));
                break;
            }
        }
        m_Updating=false;
    }

    virtual void OnText(wxCommandEvent&)
    {
        if (m_Updating) return;

        m_Data->m_Items[m_Num].m_String=GetValue();
        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_String=GetValue();
        }
        m_Data->SetChanged();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrText, wxTextCtrl)
    EVT_TEXT(CTRLID_TEXT,CIwAttrText::OnText)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
//base for panel controls
class CIwAttrGuiPanel : public wxPanel
{
public:
    std::vector<CIwAttrData*> m_Alts;
    CIwAttrData* m_Data;
    int m_Num;
    wxSizer* m_Sizer;

    CIwAttrGuiPanel(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) :
        wxPanel(Parent),m_Alts(Alts),m_Data(Data),m_Num(Num),m_Sizer(new wxBoxSizer(wxHORIZONTAL))
    {
        SetSizer(m_Sizer);
    }
};

//-----------------------------------------------------------------------------
//int value control, if data has limits adds slider
class CIwAttrTextInt : public CIwAttrGuiPanel
{
protected:
    enum { CTRLID_TEXT,CTRLID_SLIDER };

    wxSpinCtrl* m_Text;
    wxSlider* m_Slider;
    bool m_Adjust;
public:
    CIwAttrTextInt(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) : CIwAttrGuiPanel(Parent,Data,Num,Alts),m_Slider(NULL),m_Adjust(true)
    {
        int i,num=0;
        wxString Line;

        if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                if (m_Data->m_Items[m_Num].m_UInt!=m_Alts[i]->m_Items[m_Num].m_UInt)
                    break;
            }
        }
        else
        {
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                if (m_Data->m_Items[m_Num].m_Int!=m_Alts[i]->m_Items[m_Num].m_Int)
                    break;
            }
        }

        if (i==(int)m_Alts.size())
        {
            if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            {
                num=m_Data->m_Items[m_Num].m_UInt;
                Line.Printf(wxT("%u"),m_Data->m_Items[m_Num].m_UInt);
            }
            else
            {
                num=m_Data->m_Items[m_Num].m_Int;
                Line.Printf(wxT("%d"),m_Data->m_Items[m_Num].m_Int);
            }
        }

        m_Text=new wxSpinCtrl(this,CTRLID_TEXT,Line,wxPoint(-1,-1),wxSize(75,-1),wxTE_RICH);
        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Select %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Select %s here"),Data->GetName().c_str()));

        m_Sizer->Add(m_Text,0,wxRIGHT,8);

        if (m_Data->m_Member->m_Limits[2]!=0)
        {
            m_Slider=new wxSlider(this,CTRLID_SLIDER,num/m_Data->m_Member->m_Limits[2],
                                  m_Data->m_Member->m_Limits[0]/m_Data->m_Member->m_Limits[2],m_Data->m_Member->m_Limits[1]/m_Data->m_Member->m_Limits[2]);
            m_Slider->SetToolTip(wxT("Select Int Value"));
            m_Sizer->Add(m_Slider);

            m_Text->SetRange(m_Data->m_Member->m_Limits[0],m_Data->m_Member->m_Limits[1]);
        }
        else
        {
            if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
                m_Text->SetRange( 0, 0x7FFFFFFF );
            else
                m_Text->SetRange( 0x80000000, 0x7FFFFFFF );
        }

        m_Sizer->Layout();
        m_Adjust=false;
    }
    virtual void OnText(wxCommandEvent&)
    {
        if (m_Adjust) return;

        if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            m_Data->m_Items[m_Num].m_UInt=m_Text->GetValue();
        else
            m_Data->m_Items[m_Num].m_Int=m_Text->GetValue();

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
            m_Alts[i]->m_Items[m_Num].m_UInt=m_Data->m_Items[m_Num].m_UInt;
        }

        if (m_Slider!=NULL)
            m_Slider->SetValue(m_Data->m_Items[m_Num].m_Int/m_Data->m_Member->m_Limits[2]);

        m_Data->SetChanged();
    }
    void OnSlider(wxScrollEvent& e)
    {
        m_Data->m_Items[m_Num].m_Int=e.GetPosition()*m_Data->m_Member->m_Limits[2];
        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
        }

        m_Adjust=true;
        m_Text->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Int));
        m_Adjust=false;
        m_Data->SetChanged();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrTextInt, wxPanel)
    EVT_TEXT(CTRLID_TEXT,CIwAttrTextInt::OnText)
    EVT_COMMAND_SCROLL(CTRLID_SLIDER,CIwAttrTextInt::OnSlider)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//float data control
class CIwAttrTextFloat : public CIwAttrText
{
public:
    CIwAttrTextFloat(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) : CIwAttrText(Parent,Data,Num,75,Alts,false)
    {
        int i;
        m_Updating=true;
        float value=m_Data->m_Items[m_Num].m_Float;
        if (m_Data->m_Member->m_ExportSection.IsSameAs(L"firstframe",false))
            value*=m_Data->m_Mgr->m_FrameRate;

        for (i=0; i<(int)m_Alts.size(); i++)
        {
            if (m_Data->m_Items[m_Num].m_Float!=m_Alts[i]->m_Items[m_Num].m_Float)
                break;
        }

        if (i==(int)m_Alts.size())
            SetValue(CleanFloat(value,4));
        else
            SetValue(wxT(""));

        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

        m_Updating=false;
    }
    virtual void OnText(wxCommandEvent&)
    {
        if (m_Updating) return;

        float value=(float)atof(GetValue().mb_str());
        if (m_Data->m_Member->m_ExportSection.IsSameAs(L"firstframe",false))
            value/=m_Data->m_Mgr->m_FrameRate;

        m_Data->m_Items[m_Num].m_Float=value;
        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Float=value;
        }
        m_Data->SetChanged();
    }
};

//-----------------------------------------------------------------------------
//enum data control
class CIwAttrCombo : public wxComboBox
{
protected:
    CIwAttrData* m_Data;
    std::vector<CIwAttrData*> m_Alts;
    int m_Num;
    enum { CTRLID_COMBO };
public:
    CIwAttrCombo(wxWindow* Parent,CIwAttrData* Data,int Num,wxArrayString& Strings,std::vector<CIwAttrData*>& Alts) :
        wxComboBox(Parent,CTRLID_COMBO,Data->m_Member->m_Text[Data->m_Items[Num].m_Int+1],wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY),
        m_Data(Data),m_Alts(Alts),m_Num(Num)
    {
        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

        if (Strings[0].size()==0)
            SetValue(Strings[0]);
    }

    virtual void OnCombo(wxCommandEvent&)
    {
        for (int i=1; i<(int)m_Data->m_Member->m_Text.size(); i++)
        {
            if (m_Data->m_Member->m_Text[i].IsSameAs(GetValue(),false))
            {
                m_Data->m_Items[m_Num].m_Int=i-1;
                for (int j=0; j<(int)m_Alts.size(); j++)
                {
                    m_Alts[j]->m_Items[m_Num].m_Int=i-1;
                }

                CIwAttrInstance* inst=m_Data->m_Instance;
                if (m_Data->m_Member->m_Items[0].m_Class!=NULL)
                {
                    inst->m_Dialog->ScheduleReset();
                    inst->Reset(inst->m_Class);
                }

                m_Data->SetChanged();
                return;
            }
        }
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrCombo, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwAttrCombo::OnCombo)
END_EVENT_TABLE()


//-----------------------------------------------------------------------------
//dynamic template data control
class CIwAttrDynTemplate : public wxComboBox
{
protected:
    CIwAttrData* m_Data;
    std::vector<CIwAttrData*> m_Alts;
    int m_Num;
    enum { CTRLID_COMBO };
public:
    CIwAttrDynTemplate(wxWindow* Parent,CIwAttrData* Data,int Num,wxArrayString& Strings,std::vector<CIwAttrData*>& Alts) :
        wxComboBox(Parent,CTRLID_COMBO,L"",wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY),m_Data(Data),m_Alts(Alts),m_Num(Num)
    {
        int i;
        for (i=0; i<(int)Alts.size(); i++)
        {
            if (Alts[i]->m_Items[m_Num].m_Class!=m_Data->m_Items[m_Num].m_Class)
                break;
        }
        if (i==(int)Alts.size() && m_Data->m_Items[0].m_Class!=m_Data->m_Member->m_Parent)
            SetValue(m_Data->m_Items[0].m_Class->m_Name);

        if (m_Data->m_Member->m_ViewerDead)
            SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
        else
            SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));
    }

    virtual void OnCombo(wxCommandEvent&)
    {
        m_Data->m_Items[m_Num].m_Class=m_Data->m_Mgr->GetClass(GetValue());
        if (m_Data->m_Items[m_Num].m_Class==NULL)
            m_Data->m_Items[m_Num].m_Class=m_Data->m_Member->m_Parent;

        for (int j=0; j<(int)m_Alts.size(); j++)
        {
            m_Alts[j]->m_Items[m_Num].m_Class=m_Data->m_Items[m_Num].m_Class;
        }

        CIwAttrInstance* inst=m_Data->m_Instance;
        inst->m_Dialog->ScheduleReset();
        inst->Reset(inst->m_Class);
        m_Data->SetChanged();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrDynTemplate, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwAttrDynTemplate::OnCombo)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
//string id control, has editable id field, button to select item from list and read only value field
class CIwAttrGuiPanelStringID : public CIwAttrGuiPanel
{
    enum { CTRLID_ITEM,CTRLID_ITEM2 };
    wxTextCtrl* m_ComboBox;
    wxTextCtrl* m_TextCtrl;
    wxButton* m_Button;
    bool m_Adjusting;
public:
    CIwAttrGuiPanelStringID(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts);
    void OnItem(wxCommandEvent&);
    void OnButton(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
//create string id control
CIwAttrGuiPanelStringID::CIwAttrGuiPanelStringID(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) :
    CIwAttrGuiPanel(Parent,Data,Num,Alts),m_Adjusting(true)
{
    int i;
    wxString Line;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Data->m_Items[m_Num].m_Int!=m_Alts[i]->m_Items[m_Num].m_Int)
            break;
    }

    if (i==(int)m_Alts.size())
        Line=wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Int);

    m_ComboBox=new wxTextCtrl(this,CTRLID_ITEM,Line,wxPoint(-1,-1),wxSize(50,-1),wxTE_RICH);
    m_ComboBox->SetToolTip(wxString::Format(wxT("%s value"),m_Data->GetName().c_str()));

    m_Button=new wxButton(this,CTRLID_ITEM2,wxT("Change"),wxPoint(-1,-1),wxSize(50,-1));
    if (m_Data->m_Member->m_ViewerDead)
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
    else
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

    m_TextCtrl=new wxTextCtrl(this,wxID_ANY,wxT(""),wxPoint(-1,-1),wxSize(300,-1),wxTE_READONLY|wxTE_RICH);
    m_TextCtrl->SetBackgroundColour(wxColour(wxT("LIGHT GRAY")));
    m_TextCtrl->SetToolTip(wxString::Format(wxT("%s Text"),m_Data->GetName().c_str()));

    m_Sizer->Add(m_ComboBox,0,wxRIGHT,8);
    m_Sizer->Add(m_Button,0,wxRIGHT,8);
    m_Sizer->Add(m_TextCtrl);
    m_Sizer->Layout();

    if (m_Data->m_Mgr->m_Extra!=NULL && i==(int)m_Alts.size())
        m_TextCtrl->SetValue(m_Data->m_Mgr->m_Extra->GetString(ATTRSTRING_LOCALISE,m_Data->m_Items[m_Num].m_Int));

    m_Adjusting=false;
}

//--------------------------------------------------------------------------------
//ask for string id selection from list
void CIwAttrGuiPanelStringID::OnButton(wxCommandEvent&)
{
    int i;
    wxArrayString Strings;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Data->m_Items[m_Num].m_Int!=m_Alts[i]->m_Items[m_Num].m_Int)
            break;
    }

    if (m_Data->m_Mgr->m_Extra!=NULL)
        m_Data->m_Mgr->m_Extra->GetStrings(ATTRSTRING_LOCALISE,Strings,m_Data);

    if (Strings.size()<1) return;

    wxSingleChoiceDialog Dlg(this,wxT("Please choose a string"),wxT("String Database"),Strings);
    if (i==(int)m_Alts.size())
        Dlg.SetSelection(m_Data->m_Items[m_Num].m_Int);

    if (Dlg.ShowModal()==wxID_OK)
    {
        m_Adjusting=true;
        m_Data->m_Items[m_Num].m_Int=Dlg.GetSelection();
        for (i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
        }

        m_ComboBox->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Int));

        if (m_Data->m_Mgr->m_Extra)
            m_TextCtrl->SetValue(m_Data->m_Mgr->m_Extra->GetString(ATTRSTRING_LOCALISE,m_Data->m_Items[m_Num].m_Int));

        m_Adjusting=false;
        m_Data->SetChanged();
    }
}

//--------------------------------------------------------------------------------
//set value when id changes
void CIwAttrGuiPanelStringID::OnItem(wxCommandEvent&)
{
    if (m_Adjusting) return;

    m_Data->m_Items[m_Num].m_Int=atoi(m_ComboBox->GetValue().mb_str());
    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
    }

    m_TextCtrl->SetValue(m_Data->m_Mgr->m_Extra->GetString(ATTRSTRING_LOCALISE,m_Data->m_Items[m_Num].m_Int));
    m_Data->SetChanged();
}

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelStringID, wxPanel)
    EVT_TEXT(CTRLID_ITEM,CIwAttrGuiPanelStringID::OnItem)
    EVT_BUTTON(CTRLID_ITEM2,CIwAttrGuiPanelStringID::OnButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
//like string id but for a list of items (files or objects)
class CIwAttrGuiPanelFile : public CIwAttrGuiPanel
{
    enum { CTRLID_ITEM2 };
    wxTextCtrl* m_ComboBox;
    wxTextCtrl* m_TextCtrl;
    wxButton* m_Button;
    EIwAttrDialogStringType m_Type;
public:
    CIwAttrGuiPanelFile(wxWindow* Parent,CIwAttrData* Data,int Num,EIwAttrDialogStringType Type,std::vector<CIwAttrData*>& Alts);
    void OnItem(wxCommandEvent&);
    void OnButton(wxCommandEvent&);
    wxString GetStrings(wxString& Short,const wxString& Str);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
//find long/short strings
wxString CIwAttrGuiPanelFile::GetStrings(wxString& Short,const wxString& Str)
{
    Short=Str;
    Short.Replace(wxT("\\"),wxT("/"));
    Short.Replace(wxT(":"),wxT("/"));
    Short=Short.AfterLast(wxT('/'));
    Short=Short.BeforeFirst(wxT('.'));

    return Str;
}

//--------------------------------------------------------------------------------
//create file panel
CIwAttrGuiPanelFile::CIwAttrGuiPanelFile(wxWindow* Parent,CIwAttrData* Data,int Num,EIwAttrDialogStringType Type,std::vector<CIwAttrData*>& Alts) :
    CIwAttrGuiPanel(Parent,Data,Num,Alts),m_Type(Type)
{
    int i;
    wxString Long;
    wxString Short;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[Num].m_String.IsSameAs(Data->m_Items[Num].m_String,false))
            break;
    }

    if (i==(int)m_Alts.size())
    {
        if (Type==ATTRSTRING_PTR)
        {
            if (m_Data->m_Items[m_Num].m_Inst==NULL)
                Long=GetStrings(Short,wxT(""));
            else
            {
                CIwAttrData* data=m_Data->m_Items[m_Num].m_Inst->FindData(L"name");

                if (data==NULL)
                    Long=GetStrings(Short,wxT(""));
                else
                    Long=GetStrings(Short,data->m_Items[0].m_String);
            }
        }
        else
            Long=GetStrings(Short,m_Data->m_Items[m_Num].m_String);
    }
    else
        Long=GetStrings(Short,wxT(""));

    m_ComboBox=new wxTextCtrl(this,wxID_ANY,Short,wxPoint(-1,-1),wxSize(75,-1),wxTE_RICH);
    m_ComboBox->SetToolTip(wxString::Format(wxT("%s Filename"),m_Data->GetName().c_str()));

    m_Button=new wxButton(this,CTRLID_ITEM2,wxT("Change"),wxPoint(-1,-1),wxSize(50,-1));
    if (m_Data->m_Member->m_ViewerDead)
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
    else
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

    m_TextCtrl=new wxTextCtrl(this,wxID_ANY,Long,wxPoint(-1,-1),wxSize(300,-1),wxTE_READONLY|wxTE_RICH);
    m_TextCtrl->SetBackgroundColour(wxColour(wxT("LIGHT GRAY")));
    m_TextCtrl->SetToolTip(wxString::Format(wxT("%s Path"),m_Data->GetName().c_str()));

    m_Sizer->Add(m_ComboBox,0,wxRIGHT,8);
    m_Sizer->Add(m_Button,0,wxRIGHT,8);
    m_Sizer->Add(m_TextCtrl);
    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
//select from list
void CIwAttrGuiPanelFile::OnButton(wxCommandEvent&)
{
    if (m_Type==ATTRSTRING_PTR)
    {
        wxArrayString Strings;

        if (m_Data->m_Instance->m_File!=NULL)
            m_Data->m_Instance->m_File->GetPtrStrings(Strings,m_Data);

        if (Strings.size()<1) return;

        wxSingleChoiceDialog Dlg(this,wxT("Please choose an item"),wxT("Database"),Strings);
        for (int i=0; i<(int)Strings.size(); i++)
        {
            if (m_TextCtrl->GetValue().IsSameAs(Strings[i],false))
                Dlg.SetSelection(i);
        }

        if (Dlg.ShowModal()==wxID_OK)
        {
            m_Data->m_Items[m_Num].m_Inst=m_Data->m_Instance->m_File->TryGetPtrItem(Dlg.GetStringSelection(),m_Data);
            m_Data->m_Items[m_Num].m_Flags=0;

            GetStrings(m_Data->m_Items[m_Num].m_String,Strings[Dlg.GetSelection()]);

            for (int i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_String=m_Data->m_Items[m_Num].m_String;
            }

            m_ComboBox->SetValue(Dlg.GetStringSelection());
            m_TextCtrl->SetValue(Dlg.GetStringSelection());
            m_Data->SetChanged();
        }

        return;
    }

    wxFileName name(m_Data->m_Items[m_Num].m_String);
    name.MakeAbsolute(m_Data->m_Instance->m_Dialog->GetBaseDir(m_Data->m_Items[m_Num].m_String[0]=='.'));

    wxString prompt,wildcard;


    switch (m_Type)
    {
    case ATTRSTRING_FILEFONT :
        prompt=L"please select a font file";
        wildcard=L"font file (.gxfont)|*.gxfont|All Files (*.*)|*.*";
        break;
    case ATTRSTRING_FILETEXTURE:
        prompt=L"please select a texture file";
        wildcard=L"texture file (.tga;.bmp;.png;.gif)|*.tga;*.bmp;*.png;*.gif|All Files (*.*)|*.*";
        break;
    default:
        prompt=L"please select a file";
        wildcard=L"All Files (*.*)|*.*";
        break;
    }

    wxFileDialog dlg(this,prompt,name.GetPath(),name.GetFullName(),wildcard);
    if (dlg.ShowModal()==wxID_OK)
    {
        wxString Long;
        wxString Short;
        wxFileName name(dlg.GetPath());
        name.MakeRelativeTo(m_Data->m_Instance->m_Dialog->GetBaseDir(true));

        if (name.GetFullPath()[0]=='.')
            m_Data->m_Items[m_Num].m_String=name.GetFullPath();
        else
            m_Data->m_Items[m_Num].m_String=L"./"+name.GetFullPath();

        Long=GetStrings(Short,m_Data->m_Items[m_Num].m_String);

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_String=m_Data->m_Items[m_Num].m_String;
        }

        m_ComboBox->SetValue(Short);
        m_TextCtrl->SetValue(Long);
        m_Data->SetChanged();

    }
}

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelFile, wxPanel)
    EVT_BUTTON(CTRLID_ITEM2,CIwAttrGuiPanelFile::OnButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
//like string id but for a list of items (files or objects)
class CIwAttrGuiPanelPtr : public wxPanel
{
    std::vector<CIwAttrData*> m_Alts;
    CIwAttrData* m_Data;
    int m_Num;

    enum { CTRLID_ITEM2,CTRLID_EXPAND };
    wxTextCtrl* m_TextCtrl;
    wxButton* m_Button;
    CIwStyleButtonOD* m_Expand;
public:
    CIwAttrGuiPanelPtr(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts);
    void OnItem(wxCommandEvent&);
    void OnButton(wxCommandEvent&);

    void OnExpand(wxCommandEvent&)
    {
        m_Expand->m_On=!m_Expand->m_On;
        if (!m_Expand->m_On)
        {
            m_Data->m_Items[m_Num].m_Flags|=ATTRITEM_HIDDEN_F;
            for (int i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Flags|=ATTRITEM_HIDDEN_F;
            }
            SetToolTip(wxT("Click to show group"));
        }
        else
        {
            m_Data->m_Items[m_Num].m_Flags&=~ATTRITEM_HIDDEN_F;
            for (int i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Flags&=~ATTRITEM_HIDDEN_F;
            }
            SetToolTip(wxT("Click to hide group"));
        }

        Refresh();
        m_Data->SetChanged();
        m_Data->m_Instance->ResetDlg();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
//create file panel
CIwAttrGuiPanelPtr::CIwAttrGuiPanelPtr(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts) :
    wxPanel(Parent),m_Alts(Alts),m_Data(Data),m_Num(Num)
{
    int i;
    wxString Long;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[Num].m_String.IsSameAs(Data->m_Items[Num].m_String,false))
            break;
    }

    if (i==(int)m_Alts.size())
    {
        if (m_Data->m_Items[m_Num].m_Inst==NULL)
            Long=wxT("");
        else
        {
            CIwAttrData* data=m_Data->m_Items[m_Num].m_Inst->FindData(L"name");

            if (data==NULL)
                Long=wxT("");
            else
                Long=data->m_Items[0].m_String;
        }
    }
    else
        Long=wxT("");

    wxSizer* vert=new wxBoxSizer(wxVERTICAL);
    wxSizer* horiz=new wxBoxSizer(wxHORIZONTAL);
    SetSizer(vert);
    vert->Add(horiz);

    m_TextCtrl=new wxTextCtrl(this,wxID_ANY,Long,wxPoint(-1,-1),wxSize(150,-1),wxTE_READONLY|wxTE_RICH);
    m_TextCtrl->SetToolTip(wxString::Format(wxT("Ptr to%s"),m_Data->GetName().c_str()));

    m_Button=new wxButton(this,CTRLID_ITEM2,wxT("Change"),wxPoint(-1,-1),wxSize(50,-1));
    if (m_Data->m_Member->m_ViewerDead)
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here (no live update)"),Data->GetName().c_str()));
    else
        m_Button->SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

    m_Expand=new CIwStyleButtonOD(this,CTRLID_EXPAND,wxT("Expand"));
    m_Expand->Enable(false);

    horiz->Add(m_TextCtrl,0,wxRIGHT,8);
    horiz->Add(m_Button);
    horiz->Add(m_Expand);

    if (m_Data->m_Items[Num].m_Flags&ATTRITEM_ALLOCED_F)
    {
        m_TextCtrl->SetValue(L"-= inline =-");
        m_Expand->Enable(true);

        if (m_Data->m_Items[Num].m_Flags&=ATTRITEM_HIDDEN_F)
            m_Expand->m_On=false;
        else
        {
            wxPanel* panel=new wxPanel(this,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),wxSUNKEN_BORDER);
            CIwStyleCtrlGroup* Group2=new CIwStyleCtrlGroup(true,true,true);
            panel->SetSizer(Group2);
            vert->Add(panel);

            std::vector<CIwAttrInstance*> Bases;
            Bases.push_back(m_Data->m_Items[Num].m_Inst);
            for (i=0; i<(int)Alts.size(); i++)
            {
                Bases.push_back(Alts[i]->m_Items[Num].m_Inst);
            }

            m_Data->m_Instance->m_Mgr->FillMultiDialog(Group2,panel,Bases,NULL);
            Group2->Layout();
            m_Expand->m_On=true;
        }
    }

    vert->Layout();
}

//--------------------------------------------------------------------------------
//select from list
void CIwAttrGuiPanelPtr::OnButton(wxCommandEvent&)
{
    wxArrayString Strings;

    Strings.Add(L"");

    if (m_Data->m_Instance->m_File!=NULL)
        m_Data->m_Instance->m_File->GetPtrStrings(Strings,m_Data);

    Strings.Add(L"-= inline =-");

    wxSingleChoiceDialog Dlg(this,wxT("Please choose an item"),wxT("Database"),Strings);
    for (int i=0; i<(int)Strings.size(); i++)
    {
        if (m_TextCtrl->GetValue().IsSameAs(Strings[i],false))
            Dlg.SetSelection(i);
    }

    if (Dlg.ShowModal()==wxID_OK)
    {
        bool ChangedInline=false;
        if (Dlg.GetStringSelection()==L"-= inline =-")
        {
            if ((m_Data->m_Items[m_Num].m_Flags&ATTRITEM_ALLOCED_F)==0)
            {
                ChangedInline=true;

                CIwAttrInstance* Inst=new CIwAttrInstance;
                Inst->m_Mgr=m_Data->m_Instance->m_Mgr;
                Inst->m_ClassInfo=m_Data->m_Instance->m_ClassInfo;
                Inst->m_Class=m_Data->m_Member->m_Items[0].m_Class;
                Inst->AddDefaults(Inst->m_Class);
                Inst->m_Parent=m_Data;
                Inst->m_File=m_Data->m_Instance->m_File;
                Inst->m_Dialog=m_Data->m_Instance->m_Dialog;

                if (m_Data->m_Instance->m_File!=NULL)
                    m_Data->m_Instance->m_File->SetupInlinePtr(Inst,m_Data->m_Member);

                m_Data->m_Items[m_Num].m_Flags=ATTRITEM_ALLOCED_F;
                m_Data->m_Items[m_Num].m_Inst=Inst;
                m_Data->m_Items[m_Num].m_String=L"";
            }
        }
        else
        {
            if ((m_Data->m_Items[m_Num].m_Flags&ATTRITEM_ALLOCED_F)==ATTRITEM_ALLOCED_F)
                ChangedInline=true;

            m_Data->m_Items[m_Num].m_Flags&=~ATTRITEM_ALLOCED_F;
            m_Data->m_Items[m_Num].m_Inst=m_Data->m_Instance->m_File->TryGetPtrItem(Dlg.GetStringSelection(),m_Data);
            m_Data->m_Items[m_Num].m_String=Strings[Dlg.GetSelection()];
        }

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Flags=m_Data->m_Items[m_Num].m_Flags;
            m_Alts[i]->m_Items[m_Num].m_Inst=m_Data->m_Items[m_Num].m_Inst;
            m_Alts[i]->m_Items[m_Num].m_String=m_Data->m_Items[m_Num].m_String;
        }

        m_TextCtrl->SetValue(Dlg.GetStringSelection());
        m_Data->SetChanged();
        if (ChangedInline && m_Data->m_Instance->m_Dialog!=NULL)
            m_Data->m_Instance->m_Dialog->ScheduleReset();
    }
}

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelPtr, wxPanel)
    EVT_BUTTON(CTRLID_ITEM2,CIwAttrGuiPanelPtr::OnButton)
    EVT_BUTTON(CTRLID_EXPAND,CIwAttrGuiPanelPtr::OnExpand)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
//alignment data control, it has 2 dropdown lists
class CIwAttrGuiPanelAlignment : public CIwAttrGuiPanel
{
    enum { CTRLID_ITEM,CTRLID_ITEM2 };
    wxComboBox* m_ComboBox;
    wxComboBox* m_ComboBox2;
public:
    CIwAttrGuiPanelAlignment(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts);
    void OnItem(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};
//--------------------------------------------------------------------------------
//create alignment data control
CIwAttrGuiPanelAlignment::CIwAttrGuiPanelAlignment(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts) :
    CIwAttrGuiPanel(Parent,Item,Num,Alts)
{
    int i;
    wxString Val;
    wxArrayString Strings;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[m_Num].m_Int!=m_Data->m_Items[m_Num].m_Int)
            break;
    }

    if (i!=(int)m_Alts.size())
        Strings.Add(wxT(""));

    Strings.Add(wxT("left"));
    Strings.Add(wxT("centre"));
    Strings.Add(wxT("right"));

    if (i==(int)m_Alts.size())
        Val=Strings[m_Data->m_Items[m_Num].m_Int&3];

    m_ComboBox=new wxComboBox(this,CTRLID_ITEM,Val,wxPoint(-1,-1),wxSize(75,-1),Strings,wxCB_READONLY);
    m_ComboBox->SetToolTip(wxT("Select Horizontal Alignment"));
    m_Sizer->Add(m_ComboBox,0,wxRIGHT,8);

    Strings.clear();
    if (i!=(int)m_Alts.size())
        Strings.Add(wxT(""));

    Strings.Add(wxT("top"));
    Strings.Add(wxT("centre"));
    Strings.Add(wxT("bottom"));

    if (i==(int)m_Alts.size())
        Val=Strings[m_Data->m_Items[m_Num].m_Int>>4];

    m_ComboBox2=new wxComboBox(this,CTRLID_ITEM2,Val,wxPoint(-1,-1),wxSize(75,-1),Strings,wxCB_READONLY);
    m_ComboBox2->SetToolTip(wxT("Select Vertical Alignment"));
    m_Sizer->Add(m_ComboBox2);
    m_Sizer->Layout();
}

//--------------------------------------------------------------------------------
//set data from control
void CIwAttrGuiPanelAlignment::OnItem(wxCommandEvent&)
{
    m_Data->m_Items[m_Num].m_Int=0;
    if (m_ComboBox->GetValue().IsSameAs(wxT("centre"),false))
        m_Data->m_Items[m_Num].m_Int=1;
    else if (m_ComboBox->GetValue().IsSameAs(wxT("right"),false))
        m_Data->m_Items[m_Num].m_Int=2;

    if (m_ComboBox2->GetValue().IsSameAs(wxT("centre"),false))
        m_Data->m_Items[m_Num].m_Int|=1<<4;
    else if (m_ComboBox2->GetValue().IsSameAs(wxT("bottom"),false))
        m_Data->m_Items[m_Num].m_Int|=2<<4;

    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
    }
    m_Data->SetChanged();
}

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelAlignment, wxPanel)
    EVT_COMBOBOX(CTRLID_ITEM,CIwAttrGuiPanelAlignment::OnItem)
    EVT_COMBOBOX(CTRLID_ITEM2,CIwAttrGuiPanelAlignment::OnItem)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
//colour data control, has 4 int values and a coloured button
class CIwAttrGuiPanelColour : public CIwAttrGuiPanel
{
    enum { CTRLID_ITEMR,CTRLID_ITEMG,CTRLID_ITEMB,CTRLID_ITEMA,CTRLID_ITEM };
    wxTextCtrl* m_TextCtrlR;
    wxTextCtrl* m_TextCtrlG;
    wxTextCtrl* m_TextCtrlB;
    wxTextCtrl* m_TextCtrlA;
    wxButton* m_Button;
    bool m_Adjusting;
public:
    CIwAttrGuiPanelColour(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts);
    void OnItem(wxCommandEvent&);
    void OnButton(wxCommandEvent&);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
//create colour data control
CIwAttrGuiPanelColour::CIwAttrGuiPanelColour(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts) :
    CIwAttrGuiPanel(Parent,Item,Num,Alts),m_Adjusting(true)
{
    int i;
    wxString Line;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[m_Num].m_Int!=m_Data->m_Items[m_Num].m_Int)
            break;
    }

    if (i==(int)m_Alts.size())
        Line.Printf(wxT("%d"),m_Data->m_Items[m_Num].m_Int);

    m_TextCtrlR=new wxTextCtrl(this,CTRLID_ITEMR,Line,wxPoint(-1,-1),wxSize(50,-1),wxTE_RICH);
    m_TextCtrlR->SetToolTip(wxT("Set Red Value here"));
    m_Sizer->Add(m_TextCtrlR,0,wxRIGHT,8);

    if (i==(int)m_Alts.size())
        Line.Printf(wxT("%d"),m_Data->m_Items[m_Num+1].m_Int);

    m_TextCtrlG=new wxTextCtrl(this,CTRLID_ITEMG,Line,wxPoint(-1,-1),wxSize(50,-1),wxTE_RICH);
    m_TextCtrlG->SetToolTip(wxT("Set Green Value here"));
    m_Sizer->Add(m_TextCtrlG,0,wxRIGHT,8);

    if (i==(int)m_Alts.size())
        Line.Printf(wxT("%d"),m_Data->m_Items[m_Num+2].m_Int);

    m_TextCtrlB=new wxTextCtrl(this,CTRLID_ITEMB,Line,wxPoint(-1,-1),wxSize(50,-1),wxTE_RICH);
    m_TextCtrlB->SetToolTip(wxT("Set Blue Value here"));
    m_Sizer->Add(m_TextCtrlB,0,wxRIGHT,8);

    if (m_Data->m_Member->m_ArraySize>3)
    {
        if (i==(int)m_Alts.size())
            Line.Printf(wxT("%d"),m_Data->m_Items[m_Num+3].m_Int);

        m_TextCtrlA=new wxTextCtrl(this,CTRLID_ITEMA,Line,wxPoint(-1,-1),wxSize(50,-1),wxTE_RICH);
        m_TextCtrlA->SetToolTip(wxT("Set Alpha Value here"));
        m_Sizer->Add(m_TextCtrlA,0,wxRIGHT,8);
    }
    else
        m_TextCtrlA=NULL;

    m_Button=new wxButton(this,CTRLID_ITEM,wxT("Adjust Colour..."),wxPoint(-1,-1),wxSize(100,-1));
    m_Button->SetToolTip(wxT("Pick Colour using the dialog"));

    if (i==(int)m_Alts.size())
    {
        m_Button->SetBackgroundColour(wxColour(m_Data->m_Items[m_Num].m_Int,m_Data->m_Items[m_Num+1].m_Int,m_Data->m_Items[m_Num+2].m_Int));
        if (m_Data->m_Items[m_Num].m_Int+m_Data->m_Items[m_Num+1].m_Int+m_Data->m_Items[m_Num+2].m_Int>128*3)
            m_Button->SetForegroundColour(wxColour(wxT("BLACK")));
        else
            m_Button->SetForegroundColour(wxColour(wxT("WHITE")));
    }

    m_Sizer->Add(m_Button);
    m_Sizer->Layout();
    m_Adjusting=false;
}

//--------------------------------------------------------------------------------
//change the data and button colour
void CIwAttrGuiPanelColour::OnItem(wxCommandEvent&)
{
    if (m_Adjusting) return;

    m_Data->m_Items[m_Num  ].m_Int=atoi(m_TextCtrlR->GetValue().mb_str())&0xff;
    m_Data->m_Items[m_Num+1].m_Int=atoi(m_TextCtrlG->GetValue().mb_str())&0xff;
    m_Data->m_Items[m_Num+2].m_Int=atoi(m_TextCtrlB->GetValue().mb_str())&0xff;
    if (m_TextCtrlA!=NULL)
        m_Data->m_Items[m_Num+3].m_Int=atoi(m_TextCtrlA->GetValue().mb_str())&0xff;

    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num  ].m_Int=m_Data->m_Items[m_Num  ].m_Int;
        m_Alts[i]->m_Items[m_Num+1].m_Int=m_Data->m_Items[m_Num+1].m_Int;
        m_Alts[i]->m_Items[m_Num+2].m_Int=m_Data->m_Items[m_Num+2].m_Int;
        if (m_TextCtrlA!=NULL)
            m_Alts[i]->m_Items[m_Num+3].m_Int=m_Data->m_Items[m_Num+3].m_Int;
    }

    m_Button->SetBackgroundColour(wxColour(m_Data->m_Items[m_Num].m_Int,m_Data->m_Items[m_Num+1].m_Int,m_Data->m_Items[m_Num+2].m_Int));
    if (m_Data->m_Items[m_Num].m_Int+m_Data->m_Items[m_Num+1].m_Int+m_Data->m_Items[m_Num+2].m_Int>128*3)
        m_Button->SetForegroundColour(wxColour(wxT("BLACK")));
    else
        m_Button->SetForegroundColour(wxColour(wxT("WHITE")));

    m_Data->SetChanged();
}

//--------------------------------------------------------------------------------
//open the colour picker
void CIwAttrGuiPanelColour::OnButton(wxCommandEvent&)
{
    wxColourData Data;
    wxColourDialog Dlg(this,&Data);
    Data.SetColour(wxColour(m_Data->m_Items[m_Num].m_Int,m_Data->m_Items[m_Num+1].m_Int,m_Data->m_Items[m_Num+2].m_Int));

    if (Dlg.ShowModal()==wxID_OK)
    {
        m_Adjusting=true;
        m_Data->m_Items[m_Num  ].m_Int=Dlg.GetColourData().GetColour().Red();
        m_Data->m_Items[m_Num+1].m_Int=Dlg.GetColourData().GetColour().Green();
        m_Data->m_Items[m_Num+2].m_Int=Dlg.GetColourData().GetColour().Blue();

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num  ].m_Int=m_Data->m_Items[m_Num  ].m_Int;
            m_Alts[i]->m_Items[m_Num+1].m_Int=m_Data->m_Items[m_Num+1].m_Int;
            m_Alts[i]->m_Items[m_Num+2].m_Int=m_Data->m_Items[m_Num+2].m_Int;
            if (m_TextCtrlA!=NULL)
                m_Alts[i]->m_Items[m_Num+3].m_Int=m_Data->m_Items[m_Num+3].m_Int;
        }

        m_TextCtrlR->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Int));
        m_TextCtrlG->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num+1].m_Int));
        m_TextCtrlB->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num+2].m_Int));

        m_Button->SetBackgroundColour(wxColour(m_Data->m_Items[m_Num].m_Int,m_Data->m_Items[m_Num+1].m_Int,m_Data->m_Items[m_Num+2].m_Int));
        if (m_Data->m_Items[m_Num].m_Int+m_Data->m_Items[m_Num+1].m_Int+m_Data->m_Items[m_Num+2].m_Int>128*3)
            m_Button->SetForegroundColour(wxColour(wxT("BLACK")));
        else
            m_Button->SetForegroundColour(wxColour(wxT("WHITE")));

        m_Adjusting=false;
        m_Data->SetChanged();
    }
}

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelColour, wxPanel)
    EVT_TEXT(CTRLID_ITEMR,CIwAttrGuiPanelColour::OnItem)
    EVT_TEXT(CTRLID_ITEMG,CIwAttrGuiPanelColour::OnItem)
    EVT_TEXT(CTRLID_ITEMB,CIwAttrGuiPanelColour::OnItem)
    EVT_TEXT(CTRLID_ITEMA,CIwAttrGuiPanelColour::OnItem)
    EVT_BUTTON(CTRLID_ITEM,CIwAttrGuiPanelColour::OnButton)
END_EVENT_TABLE()


//--------------------------------------------------------------------------------
//coordinate data control, this has a float/int field and a tick box to swap between the 2 modes
//float is a % of size, int is absolute
class CIwAttrGuiPanelCoord : public wxPanel
{
    enum { CTRLID_ITEM,CTRLID_ITEM2 };
public:
    std::vector<CIwAttrData*> m_Alts;
    CIwAttrData* m_Data;
    int m_Num;
    wxSizer* m_Sizer;

    wxCheckBox* m_CheckBox;
    wxTextCtrl* m_TextCtrl;
    bool m_Alter;
public:
    CIwAttrGuiPanelCoord(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts);
    void OnCheck(wxCommandEvent&);
    void OnItem(wxCommandEvent&);
    void Set(const wxString& Val);

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGuiPanelCoord, wxPanel)
    EVT_CHECKBOX(CTRLID_ITEM,CIwAttrGuiPanelCoord::OnCheck)
    EVT_TEXT(CTRLID_ITEM2,CIwAttrGuiPanelCoord::OnItem)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
//create coordinate data control
CIwAttrGuiPanelCoord::CIwAttrGuiPanelCoord(wxWindow* Parent,CIwAttrData* Item,int Num,std::vector<CIwAttrData*>& Alts) :
    wxPanel(Parent),m_Alts(Alts),m_Data(Item),m_Num(Num),m_Sizer(new wxBoxSizer(wxVERTICAL)),m_TextCtrl(NULL),m_Alter(true)
{
    int i;
    wxString Line;
    bool Multi=false,MultiTick=false;

    for (i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt!=m_Data->m_Items[m_Num].m_Coord.m_UseInt)
        {
            Multi=true;
            MultiTick=true;
            continue;
        }

        if (!m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt)
        {
            if (m_Alts[i]->m_Items[m_Num].m_Coord.m_Float!=m_Data->m_Items[m_Num].m_Coord.m_Float)
                Multi=true;
        }
        else if (m_Alts[i]->m_Items[m_Num].m_Coord.m_Int!=m_Data->m_Items[m_Num].m_Coord.m_Int)
            Multi=true;
    }

    if (!Multi)
    {
        if (!m_Data->m_Items[m_Num].m_Coord.m_UseInt)
            Line.Printf(CleanFloat(m_Data->m_Items[m_Num].m_Coord.m_Float));
        else
            Line.Printf(wxT("%d"),m_Data->m_Items[m_Num].m_Coord.m_Int);
    }

    SetSizer(m_Sizer);

    m_CheckBox=new wxCheckBox(this,CTRLID_ITEM,wxT("Screen Coords"),wxPoint(-1,-1),wxSize(75,-1),wxCHK_3STATE);
    if (MultiTick)
        m_CheckBox->Set3StateValue(wxCHK_UNDETERMINED);
    else
        m_CheckBox->SetValue(!m_Data->m_Items[m_Num].m_Coord.m_UseInt);

    m_CheckBox->SetToolTip(wxT("Select Screen (Absolute) or Percentage (Relative) Coordinates"));

    m_Sizer->Add(m_CheckBox,0,wxEXPAND|wxRIGHT,8);
    m_TextCtrl=new wxTextCtrl(this,CTRLID_ITEM2,Line,wxPoint(-1,-1),wxSize(75,-1),wxTE_RICH);
    if (MultiTick)
    {
        m_TextCtrl->Enable(false);
        m_TextCtrl->SetToolTip(wxT("Multi Values"));
    }
    else if (!m_Data->m_Items[m_Num].m_Coord.m_UseInt)
        m_TextCtrl->SetToolTip(wxT("Relative Coordinate Value"));
    else
        m_TextCtrl->SetToolTip(wxT("Absolute Coordinate Value"));

    m_Sizer->Add(m_TextCtrl);

    m_Sizer->Layout();
    m_Alter=false;
}

//--------------------------------------------------------------------------------
//set int value
void CIwAttrGuiPanelCoord::Set(const wxString& Val)
{
    m_Alter=true;
    m_CheckBox->SetValue(false);
    m_TextCtrl->SetValue(Val);
    m_Alter=false;
}

//--------------------------------------------------------------------------------
//change between int and float
void CIwAttrGuiPanelCoord::OnCheck(wxCommandEvent&)
{
    bool Old=m_Data->m_Items[m_Num].m_Coord.m_UseInt;
    m_Data->m_Items[m_Num].m_Coord.m_UseInt=m_CheckBox->GetValue() ? 0 : 1;

    //find the screen size
    wxSize Size(1,1);
    bool Portrait=false;
    bool Multi=false;
    if (m_Data->m_Mgr->m_Extra!=NULL)
        m_Data->m_Mgr->m_Extra->GetViewerSize(&Size,&Portrait);

    //convert
    if (Old && !m_Data->m_Items[m_Num].m_Coord.m_UseInt)
    {
        if (m_Num==0)
            m_Data->m_Items[m_Num].m_Coord.m_Float=(float)m_Data->m_Items[m_Num].m_Coord.m_Int/Size.x;
        else
            m_Data->m_Items[m_Num].m_Coord.m_Float=(float)m_Data->m_Items[m_Num].m_Coord.m_Int/Size.y;
    }

    if (!Old && m_Data->m_Items[m_Num].m_Coord.m_UseInt)
    {
        if (m_Num==0)
            m_Data->m_Items[m_Num].m_Coord.m_Int=Size.x*m_Data->m_Items[m_Num].m_Coord.m_Float;
        else
            m_Data->m_Items[m_Num].m_Coord.m_Int=Size.y*m_Data->m_Items[m_Num].m_Coord.m_Float;
    }

    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        if (m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt && !m_Data->m_Items[m_Num].m_Coord.m_UseInt)
        {
            if (m_Num==0)
                m_Alts[i]->m_Items[m_Num].m_Coord.m_Float=(float)m_Alts[i]->m_Items[m_Num].m_Coord.m_Int/Size.x;
            else
                m_Alts[i]->m_Items[m_Num].m_Coord.m_Float=(float)m_Alts[i]->m_Items[m_Num].m_Coord.m_Int/Size.y;
        }

        if (!m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt && m_Data->m_Items[m_Num].m_Coord.m_UseInt)
        {
            if (m_Num==0)
                m_Alts[i]->m_Items[m_Num].m_Coord.m_Int=Size.x*m_Alts[i]->m_Items[m_Num].m_Coord.m_Float;
            else
                m_Alts[i]->m_Items[m_Num].m_Coord.m_Int=Size.y*m_Alts[i]->m_Items[m_Num].m_Coord.m_Float;
        }

        m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt=m_Data->m_Items[m_Num].m_Coord.m_UseInt;

        if (!m_Alts[i]->m_Items[m_Num].m_Coord.m_UseInt)
        {
            if (m_Alts[i]->m_Items[m_Num].m_Coord.m_Float!=m_Data->m_Items[m_Num].m_Coord.m_Float)
                Multi=true;
        }
        else if (m_Alts[i]->m_Items[m_Num].m_Coord.m_Int!=m_Data->m_Items[m_Num].m_Coord.m_Int)
            Multi=true;
    }

    m_TextCtrl->Enable();
    if (!Multi)
    {
        if (!m_Data->m_Items[m_Num].m_Coord.m_UseInt)
            m_TextCtrl->SetValue(CleanFloat(m_Data->m_Items[m_Num].m_Coord.m_Float));
        else
            m_TextCtrl->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Coord.m_Int));
    }

    m_Data->SetChanged();

    if (!m_Data->m_Items[m_Num].m_Coord.m_UseInt)
        m_TextCtrl->SetToolTip(wxT("Relative Coordinate Value"));
    else
        m_TextCtrl->SetToolTip(wxT("Absolute Coordinate Value"));
}

//--------------------------------------------------------------------------------
//set data from control
void CIwAttrGuiPanelCoord::OnItem(wxCommandEvent&)
{
    if (m_Alter) return;

    if (m_TextCtrl==NULL) return;

    if (!m_Data->m_Items[m_Num].m_Coord.m_UseInt)
    {
        m_Data->m_Items[m_Num].m_Coord.m_Float=(float)atof(m_TextCtrl->GetValue().mb_str());

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Coord.m_Float=m_Data->m_Items[m_Num].m_Coord.m_Float;
        }
    }
    else
    {
        m_Data->m_Items[m_Num].m_Coord.m_Int=atoi(m_TextCtrl->GetValue().mb_str());

        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[m_Num].m_Coord.m_Int=m_Data->m_Items[m_Num].m_Coord.m_Int;
        }
    }

    m_Data->SetChanged();
}

//--------------------------------------------------------------------------------
class CIwAttrGroupToggle : public CIwStyleButtonOD
{
    enum { CTRLID_ITEM };
    CIwAttrData* m_Data;
    CIwAttrInstance* m_Inst;
    std::vector<CIwAttrData*> m_Alts;
public:
    CIwAttrGroupToggle(wxWindow* Parent,bool On,CIwAttrData* Data,CIwAttrInstance* Inst,std::vector<CIwAttrData*>& Alts) :
        CIwStyleButtonOD(Parent,CTRLID_ITEM),m_Data(Data),m_Inst(Inst),m_Alts(Alts)
    {
        m_On=On;
        if (Data->m_Member->m_Text.size()<1)
            SetLabel(wxString::Format(wxT("Show/Hide %s Group"),Data->m_Member->m_Name.c_str()));
        else
            SetLabel(wxString::Format(wxT("Show/Hide %s Group"),Data->m_Member->m_Text[0].c_str()));

        if (!m_On)
            SetToolTip(wxT("Click to show group"));
        else
            SetToolTip(wxT("Click to hide group"));
    }
    void OnItem(wxCommandEvent&)
    {
        m_On=!m_On;
        if (!m_On)
        {
            m_Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;
            for (int i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;
            }
            SetToolTip(wxT("Click to show group"));
        }
        else
        {
            m_Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
            for (int i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
            }
            SetToolTip(wxT("Click to hide group"));
        }

        Refresh();
        m_Inst->SetChanged(true);
        m_Inst->ResetDlg();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrGroupToggle,CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_ITEM,CIwAttrGroupToggle::OnItem)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//button for removing data that can be removed
class CIwAttrRemoveButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrData* m_Data;
    CIwAttrInstance* m_Inst;
    CIwAttrRemoveButton(wxWindow* Parent,CIwAttrData* Data,CIwAttrInstance* Inst) : CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("-"),wxSize(0,1)),m_Data(Data),m_Inst(Inst) {}
    void OnRemove(wxCommandEvent&)
    {
        for (std::vector<CIwAttrData*>::iterator it=m_Inst->m_Data.begin(); it!=m_Inst->m_Data.end(); )
        {
            CIwAttrData* data;
            for (data=(*it); data!=NULL; data=data->m_Group)
            {
                if (data==m_Data)
                {
                    it=m_Inst->m_Data.erase(it);
                    break;
                }
            }
            if (data==NULL)
                ++it;
        }
        delete m_Data;
        m_Inst->SetChanged(true);
        m_Inst->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrRemoveButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrRemoveButton::OnRemove)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
//button for removing data that can be removed
class CIwAttrRemoveListButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrData* m_Data;
    int m_Num;
    CIwAttrRemoveListButton(wxWindow* Parent,CIwAttrData* Data,int num) : CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("-"),wxSize(0,1)),m_Data(Data),m_Num(num) {}
    void OnRemove(wxCommandEvent&)
    {
        m_Data->m_Items.erase(m_Data->m_Items.begin()+m_Num);
        m_Data->SetChanged();
        m_Data->m_Instance->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrRemoveListButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrRemoveListButton::OnRemove)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
//button for removing data that can be removed
class CIwAttrAddListButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrData* m_Data;
    CIwAttrAddListButton(wxWindow* Parent,CIwAttrData* Data) : CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("Add ")+Data->GetName(),wxSize(0,1)),m_Data(Data) {}
    void OnAdd(wxCommandEvent&)
    {
        m_Data->m_Items.resize(m_Data->m_Items.size()+1);
        m_Data->SetChanged();
        m_Data->m_Instance->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrAddListButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrAddListButton::OnAdd)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//button for removing data that can be removed
class CIwAttrTemplateAddRemoveButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrData* m_Data;
    CIwAttrTemplateAddRemoveButton(wxWindow* Parent,CIwAttrData* Data) : CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("-"),wxSize(0,1)),m_Data(Data)
    {
        if (m_Data->m_Items[1].m_Inst==NULL)
            SetLabel(L"+");
    }
    void OnChange(wxCommandEvent&)
    {
        if (m_Data->m_Items[1].m_Inst==NULL)
        {
            CIwAttrClass* klass=m_Data->m_Mgr->GetTemplateClassForExt(m_Data->m_Items[0].m_String);
            if (klass!=NULL)
            {
                m_Data->m_Items[1].m_Flags=ATTRITEM_ALLOCED_F;
                m_Data->m_Items[1].m_Inst=new CIwAttrInstance;
                m_Data->m_Items[1].m_Inst->m_Mgr=m_Data->m_Mgr;
                m_Data->m_Items[1].m_Inst->m_Class=klass;
                m_Data->m_Items[1].m_Inst->AddDefaults(m_Data->m_Items[1].m_Inst->m_Class);
                m_Data->m_Items[1].m_Inst->m_File=m_Data->m_Instance->m_File;
                m_Data->m_Items[1].m_Inst->m_Dialog=m_Data->m_Instance->m_Dialog;

                m_Data->m_Items[1].m_Inst->m_Parent=m_Data;
                SetLabel(L"-");
            }
        }
        else
        {
            SetLabel(L"+");
            delete m_Data->m_Items[1].m_Inst;
            m_Data->m_Items[1].m_Inst=NULL;
            m_Data->m_Items[1].m_Flags=0;
        }

        m_Data->SetChanged();
        m_Data->m_Instance->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrTemplateAddRemoveButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrTemplateAddRemoveButton::OnChange)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//button for removing data that can be removed
class CIwAttrTemplateBrowseButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrData* m_Data;
    CIwAttrText* m_Text;
    std::vector<CIwAttrData*> m_Alts;
    CIwAttrTemplateBrowseButton(wxWindow* Parent,CIwAttrData* Data,CIwAttrText* text,std::vector<CIwAttrData*>& Alts) :
        CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("Browse...")),m_Data(Data),m_Text(text),m_Alts(Alts) {}
    void OnBrowse(wxCommandEvent&)
    {
        m_Data->m_Instance->m_File->BrowseForFile(m_Data->m_Items[0].m_String);

        m_Text->SetValue(m_Data->m_Items[0].m_String);
        for (int i=0; i<(int)m_Alts.size(); i++)
        {
            m_Alts[i]->m_Items[0].m_String=m_Data->m_Items[0].m_String;
        }

        m_Data->SetChanged();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrTemplateBrowseButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrTemplateBrowseButton::OnBrowse)
END_EVENT_TABLE()


//-----------------------------------------------------------------------------
//button for moving data that can be moved
class CIwAttrMoveButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    int m_Offset;
    int m_Direction;
    CIwAttrInstance* m_Inst;
    CIwAttrMoveButton(wxWindow* Parent,const wxString& text,int offset,int direction,CIwAttrInstance* Inst) :
        CIwStyleButtonOD(Parent,CTRLID_BUTTON,text,wxSize(0,1)),m_Offset(offset),m_Direction(direction),m_Inst(Inst) {}
    void OnMove(wxCommandEvent&)
    {
        CIwAttrData* data=m_Inst->m_Data[m_Offset];
        int swapWith=m_Offset+m_Direction;
        if (swapWith<0 || swapWith>=(int)m_Inst->m_Data.size())
            return;

        CIwAttrData* data2=m_Inst->m_Data[swapWith];

        m_Inst->m_Data[swapWith]=data;
        m_Inst->m_Data[m_Offset]=data2;

        m_Inst->SetChanged(true);
        m_Inst->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrMoveButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrMoveButton::OnMove)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//button to add sub sections, this has a dropdown list and a button
class CIwAttrAddButton : public CIwStyleButtonOD
{
public:
    enum { CTRLID_BUTTON };
    CIwAttrInstance* m_Inst;
    std::vector<CIwAttrInstance*> m_Bases;
    wxComboBox* m_Combo;
    CIwAttrAddButton(wxWindow* Parent,wxComboBox* Combo,CIwAttrInstance* Inst) :
        CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("+")),m_Inst(Inst),m_Combo(Combo) {}
    CIwAttrAddButton(wxWindow* Parent,wxComboBox* Combo,std::vector<CIwAttrInstance*>& bases) :
        CIwStyleButtonOD(Parent,CTRLID_BUTTON,wxT("+")),m_Inst(NULL),m_Bases(bases),m_Combo(Combo) {}
    void OnAdd(wxCommandEvent&)
    {
        if (m_Combo->GetValue().empty())
            return;

        CIwAttrNote Note;
        Note.m_Info=-1;
        Note.m_Data=L"{";

        std::vector<wxString> argv;
        argv.push_back(wxT("{"));

        if (m_Inst!=NULL)
        {
            CIwAttrInstance* Inst=m_Inst->AddFromNote(m_Combo->GetValue(),Note,argv,m_Inst);

            if (Inst!=NULL)
                m_Inst->m_Mgr->DoAddWizard(Inst);

            m_Inst->SetChanged(true);
        }

        if (!m_Bases.empty())
        {
            for (int i=0; i<(int)m_Bases.size(); i++)
            {
                m_Inst=m_Bases[i];

                CIwAttrInstance* Inst=m_Inst->AddFromNote(m_Combo->GetValue(),Note,argv,m_Inst);

                if (Inst!=NULL)
                    m_Inst->m_Mgr->DoAddWizard(Inst);

                m_Inst->SetChanged(true);
            }
        }

        m_Inst->ResetDlg();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrAddButton, CIwStyleButtonOD)
    EVT_BUTTON(CTRLID_BUTTON,CIwAttrAddButton::OnAdd)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//drop down list to change sections class
class CIwAttrClassCombo : public wxComboBox
{
public:
    enum { CTRLID_COMBO };
    std::vector<CIwAttrInstance*> m_Insts;
    wxComboBox* m_Combo;
    CIwAttrClassCombo(wxWindow* Parent,const wxString& Value,wxArrayString& Strings,std::vector<CIwAttrInstance*>& Insts) :
        wxComboBox(Parent,CTRLID_COMBO,Value,wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY),m_Insts(Insts)
    {
        SetToolTip(wxT("Select Class Type"));
    }
    void OnChange(wxCommandEvent&)
    {
        if (GetValue().empty())
            return;

        m_Insts[0]->SetChanged(true);
        for (int i=0; i<(int)m_Insts.size(); i++)
        {
            m_Insts[i]->Reset(m_Insts[0]->m_Mgr->GetClass(GetValue()));
            m_Insts[0]->m_Dialog->Changed(m_Insts[i]);
        }
        m_Insts[0]->m_Dialog->ScheduleReset();
    }
    DECLARE_EVENT_TABLE()
};

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrClassCombo, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwAttrClassCombo::OnChange)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
//dialog for DoAddWizard
class CIwAttrAddWizard : public CIwStyleDialog
{
public:
    enum { CTRLID_OK,CTRLID_CANCEL };
    CIwStyleCtrlGroup* m_Sizer;
    wxSizer* m_MainSizer;
public:
    CIwAttrAddWizard(wxWindow *Parent) : CIwStyleDialog(Parent,wxT("Add Class:"))
    {
        m_MainSizer=new wxBoxSizer(wxVERTICAL);
        m_MainSizer->Add(new CIwStyleHeader(this,wxT("Add New Class Wizard"),wxT("Fill in the important members of a new class")));

        m_Sizer=new CIwStyleCtrlGroup(true,true,true);
        m_MainSizer->Add(m_Sizer);

        SetSizer(m_MainSizer);
    }
    bool ShowIt()
    {
        CIwStyleButtonBar* Bar=new CIwStyleButtonBar(this);
        Bar->Add(new CIwStyleButtonOD(this,wxID_OK,wxT("OK")));
        Bar->Add(new CIwStyleButtonOD(this,wxID_CANCEL,wxT("Cancel")));
        m_MainSizer->Add(Bar);

        m_MainSizer->Layout();
        m_MainSizer->Fit(this);

        return ShowModal()!=wxID_CANCEL;
    }
};

//-----------------------------------------------------------------------------
//creates a dialog to create a new sub section presenting the user with only the important fields
void CIwAttrDescMgr::DoAddWizard(CIwAttrInstance* Inst)
{
    bool Found=false;
    if (Inst->m_Parent==NULL) return;

    for (int i=0; i<(int)Inst->m_Data.size(); i++)
    {
        if (!Inst->m_Data[i]->m_Member->m_Wizard.empty())
        {
            if (Inst->m_Data[i]->m_Member->m_Wizard[0].IsSameAs(wxT("set"),false))
            {
                Found=true;
                break;
            }
        }
    }
    if (!Found) return;

    m_IsWizard=true;
    CIwAttrAddWizard Dlg(Inst->m_Dialog);
    FillDialog(Dlg.m_Sizer,&Dlg,Inst,NULL);
    if (!Dlg.ShowIt()) //if failed remove added data
    {
        std::vector<CIwAttrData*>::iterator it;
        for (it=Inst->m_Parent->m_Instance->m_Data.begin(); it!=Inst->m_Parent->m_Instance->m_Data.end(); ++it)
        {
            if ((*it)==Inst->m_Parent)
                break;
        }
        if (it!=Inst->m_Parent->m_Instance->m_Data.end())
            Inst->m_Parent->m_Instance->m_Data.erase(it);

        delete Inst->m_Parent;
    }
    else
    {
        Inst->SetChanged(true); //update
    }

    m_IsWizard=false;
}

//-----------------------------------------------------------------------------
//fill a dialog with controls from an instance
void CIwAttrDescMgr::FillDialog(CIwStyleCtrlGroup* Sizer,wxWindow* Win,CIwAttrInstance* Base,wxArrayString* ClassStrings,const wxChar** Exclude,bool readOnly)
{
    std::vector<CIwAttrInstance*> Bases;
    int i;

    Bases.push_back(Base);

    //class chooser control
    if (ClassStrings!=NULL && ClassStrings->size()>0)
    {
        CIwAttrClassCombo* combo=new CIwAttrClassCombo(Win,Base->m_Class->m_Name,*ClassStrings,Bases);
        if (readOnly) combo->Enable(false);

        Sizer->Add(combo,wxT("Class:"));
    }

    std::vector<CIwAttrData*> Alts;
    for (i=0; i<(int)Base->m_Data.size(); i++)
    {
        if (!m_IsExporter && (Base->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD ) continue;

        if (m_IsWizard && Base->m_Data[i]->m_Member->m_Wizard.empty()) continue;

        Base->m_Data[i]->GetDlgItem(Win,Sizer,Alts,-1,Exclude,readOnly);

        if ((Base->m_Class->m_Flags&ATTRCLASS_ORDERED_F)!=0)
        {
            wxSizer* buttons=new wxBoxSizer(wxHORIZONTAL);
            CIwStyleButtonOD* button=new CIwAttrMoveButton(Win,L"/\\",i,-1,Base);
            if (i<=0 || readOnly)
                button->Enable(false);

            buttons->Add(button);

            button=new CIwAttrMoveButton(Win,L"\\/",i,1,Base);
            if (i>=(int)Base->m_Data.size()-1 || readOnly)
                button->Enable(false);

            buttons->Add(button);

            button=new CIwAttrRemoveButton(Win,Base->m_Data[i],Base);
            if (readOnly)
                button->Enable(false);

            buttons->Add(button);
            Sizer->AddAtLeft(buttons);
        }
        else if ((Base->m_Data[i]->m_Member->m_Type&ATTRMEMBER_0ORMORE)!=0)
        {
            CIwAttrRemoveButton* button=new CIwAttrRemoveButton(Win,Base->m_Data[i],Base);
            if (readOnly) button->Enable(false);

            Sizer->Add(button);
        }
    }

    //add sub section control
    wxArrayString Strings;
    std::vector<CIwAttrClass*> Classes;
    Base->GetChildClasses(Classes,Base->m_Class,Exclude);
    GetDerivedClasses(Classes);
    GetClassStrings(Classes,Strings);
    Base->GetAddStrings(Strings,Base->m_Class,(Base->m_Class->m_Flags&ATTRCLASS_ORDERED_F)!=0);
    if (Strings.size()==0)
        return;

    wxComboBox* Combo=new wxComboBox(Win,wxID_ANY,Strings[0],wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY);
    Combo->SetToolTip(wxT("Select Class Type to Add"));
    if (readOnly) Combo->Enable(false);

    Sizer->Add(Combo,wxT("Add:"));

    CIwAttrAddButton* Button=new CIwAttrAddButton(Win,Combo,Base);
    Button->SetToolTip(wxT("Add Sub Class"));
    if (readOnly) Button->Enable(false);

    Sizer->Add(Button);

    if (CIwAttrDescMgr::s_LiveEditing)
    {
        wxStaticText* info1=new wxStaticText(Win,wxID_ANY,L"* These item(s) are not live editable");
        Sizer->Add(info1,L"Note:");
        wxStaticText* info2=new wxStaticText(Win,wxID_ANY,L"# These item(s) are live editable except the default value");
        Sizer->Add(info2,L"Note:");
    }
}

//-----------------------------------------------------------------------------
bool CIwAttrDescMgr::FindAlts(std::vector<CIwAttrInstance*>& Bases,std::vector<CIwAttrData*>& Alts,CIwAttrData* Data)
{
    if (Data->m_Instance->m_Class->m_Flags&ATTRCLASS_ORDERED_F)
        return false;

    if (Data->m_Member->m_Type&ATTRMEMBER_1ORMORE)  //TODO find 1 or more  members
        return false;

    if (Data->m_Group!=NULL) //TODO find group members
        return false;

    for (int i=1; i<(int)Bases.size(); i++)
    {
        for (int j=0; j<(int)Bases[i]->m_Data.size(); j++)
        {
            if (Bases[i]->m_Data[j]->m_Member==Data->m_Member)
            {
                Alts.push_back(Bases[i]->m_Data[j]);
                break;
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
class CIwAttrFileTypeCombo : public wxComboBox
{
    enum { CTRLID_COMBO };
    CIwAttrFileGroup* m_Group;
public:
    CIwAttrFileTypeCombo(wxWindow* parent,const wxString& value,const wxArrayString& choices,CIwAttrFileGroup* group) :
        wxComboBox(parent,CTRLID_COMBO,value,wxPoint(-1,-1),wxSize(-1,-1),choices,wxCB_READONLY),m_Group(group) {}

    void OnSelect(wxCommandEvent&)
    {
        m_Group->m_CurrType=GetExportType(GetStringSelection());
        m_Group->m_Inst->SetChanged(true);
        m_Group->m_Inst->ResetDlg();
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrFileTypeCombo, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwAttrFileTypeCombo::OnSelect)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
wxComboBox* CIwAttrDescMgr::AddTypesSetup(CIwStyleCtrlGroup* Sizer,wxWindow* Win,CIwAttrFileGroup* group)
{
    std::vector<CIwAttrExportType> types;
    wxArrayString strings;
    wxString sel=L"<Any>";
    strings.push_back(sel);

    for (int j=0; j<(int)m_Descs.size(); j++)
    {
        if (m_Descs[j]->m_FileExtMap.find(group->m_Ext)==m_Descs[j]->m_FileExtMap.end())
            continue;

        for (int k=0; k<(int)m_Descs[j]->m_FileExtMap[group->m_Ext].m_Types.size(); k++)
        {
            CIwAttrExportType type=m_Descs[j]->m_FileExtMap[group->m_Ext].m_Types[k];

            if (type==group->m_CurrType)
                sel=exportTypeMap[type];

            if (type==group->m_CurrType || group->m_CurrType==EXPORTTYPE_NUM)
                types.push_back(type);

            strings.Add(exportTypeMap[type]);
        }
    }
    for (int i=0; i<(int)group->m_Inst->m_Class->m_Members.size(); i++)
    {
        if (group->m_Inst->m_Class->m_Members[i]->m_ExportType==EXPORTTYPE_NUM)
            continue;

        group->m_Inst->m_Class->m_Members[i]->m_UseClassInAdd=false;
        for (int k=0; k<(int)types.size(); k++)
        {
            if (types[k]==group->m_Inst->m_Class->m_Members[i]->m_ExportType)
            {
                group->m_Inst->m_Class->m_Members[i]->m_UseClassInAdd=true;
                break;
            }
        }
    }

    wxComboBox* combo=new CIwAttrFileTypeCombo(Win,sel,strings,group);
    Sizer->Add(combo,L"File Type:");
    if (strings.size()<3) combo->Enable(false);

    return combo;
}

//-----------------------------------------------------------------------------
void CIwAttrDescMgr::GetResTemplateStrings(wxArrayString& Strings)
{
    for (int j=0; j<(int)m_Descs.size(); j++)
    {
        std::map<wxString,CIwAttrDescFileExt>::iterator it;
        for (it=m_Descs[j]->m_FileExtMap.begin(); it!=m_Descs[j]->m_FileExtMap.end(); ++it)
        {
            if (it->second.m_TemplateClass!=NULL)
                Strings.Add(it->first);
        }
    }
}

//-----------------------------------------------------------------------------
wxString CIwAttrDescMgr::FindResTemplateString(const wxString& name)
{
    for (int j=0; j<(int)m_Descs.size(); j++)
    {
        std::map<wxString,CIwAttrDescFileExt>::iterator it;
        for (it=m_Descs[j]->m_FileExtMap.begin(); it!=m_Descs[j]->m_FileExtMap.end(); ++it)
        {
            wxString fir=it->first;
            if (it->second.m_TemplateClass!=NULL && it->second.m_TemplateClass->m_Name.IsSameAs(name,false))
                return it->first;
        }
    }
    return L"";
}

//-----------------------------------------------------------------------------
//fill a dialog with controls from multiple instances
void CIwAttrDescMgr::FillMultiDialog(CIwStyleCtrlGroup* Sizer,wxWindow* Win,std::vector<CIwAttrInstance*>& Bases,wxArrayString* ClassStrings,const wxChar** Exclude,bool readOnly)
{
    int i;
    if (Bases.size()<1) return;

    bool AllSameClass=true;
    for (i=1; i<(int)Bases.size(); i++)
    {
        if (Bases[0]->m_Class!=Bases[i]->m_Class)
        {
            AllSameClass=false;
            break;
        }
    }

    //class chooser control
    if (ClassStrings!=NULL && ClassStrings->size()>0)
    {
        CIwAttrClassCombo* combo;
        if (!AllSameClass)
        {
            ClassStrings->Insert(wxT(""),0);

            combo=new CIwAttrClassCombo(Win,wxT(""),*ClassStrings,Bases);
        }
        else
            combo=new CIwAttrClassCombo(Win,Bases[0]->m_Class->m_Name,*ClassStrings,Bases);

        if (readOnly) combo->Enable(false);

        Sizer->Add(combo,wxT("Class:"));
    }

    if (!AllSameClass) return;

    for (i=0; i<(int)Bases[0]->m_Data.size(); i++)
    {
        if (!m_IsExporter && (Bases[0]->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD ) continue;

        if (m_IsWizard && Bases[0]->m_Data[i]->m_Member->m_Wizard.empty()) continue;

        std::vector<CIwAttrData*> Alts;
        if (Bases.size()>1)
            if (!FindAlts(Bases,Alts,Bases[0]->m_Data[i]))
                continue;

        Bases[0]->m_Data[i]->GetDlgItem(Win,Sizer,Alts,-1,Exclude,readOnly);

        if ((Bases[0]->m_Class->m_Flags&ATTRCLASS_ORDERED_F)!=0)
        {
            wxSizer* buttons=new wxBoxSizer(wxHORIZONTAL);
            CIwStyleButtonOD* button=new CIwAttrMoveButton(Win,L"/\\",i,-1,Bases[0]);
            if (i<=0 || readOnly)
                button->Enable(false);

            buttons->Add(button);

            button=new CIwAttrMoveButton(Win,L"\\/",i,1,Bases[0]);
            if (i>=(int)Bases[0]->m_Data.size()-1 || readOnly)
                button->Enable(false);

            buttons->Add(button);

            button=new CIwAttrRemoveButton(Win,Bases[0]->m_Data[i],Bases[0]);
            if (readOnly)
                button->Enable(false);

            buttons->Add(button);
            Sizer->AddAtLeft(buttons);
        }
        else if ((Bases[0]->m_Data[i]->m_Member->m_Type&ATTRMEMBER_0ORMORE)!=0)
        {
            CIwAttrRemoveButton* button=new CIwAttrRemoveButton(Win,Bases[0]->m_Data[i],Bases[0]);
            if (readOnly) button->Enable(false);

            Sizer->Add(button);
        }
    }

    //add sub section control
    wxArrayString Strings;
    std::vector<CIwAttrClass*> Classes;
    Bases[0]->GetChildClasses(Classes,Bases[0]->m_Class,Exclude);
    GetDerivedClasses(Classes);
    GetClassStrings(Classes,Strings);
    Bases[0]->GetAddStrings(Strings,Bases[0]->m_Class,(Bases[0]->m_Class->m_Flags&ATTRCLASS_ORDERED_F)!=0);
    if (Strings.size()==0)
        return;  /*
                    if(Bases.size()>1)
                    {
                     Strings.clear();
                     Strings.Add(wxT(""));
                    }*/

    wxComboBox* Combo=new wxComboBox(Win,wxID_ANY,Strings[0],wxPoint(-1,-1),wxSize(-1,-1),Strings,wxCB_READONLY);
    Combo->SetToolTip(wxT("Select Class Type to Add"));
    if (readOnly) Combo->Enable(false);

    Sizer->Add(Combo,wxT("Add:"));

    CIwAttrAddButton* Button=new CIwAttrAddButton(Win,Combo,Bases);
    Button->SetToolTip(wxT("Add Sub Class"));
    if (readOnly) Button->Enable(false);

    Sizer->Add(Button);
}
//-----------------------------------------------------------------------------
void CIwAttrDescMgr::FindClassBases(int Slot, std::vector<ClassBaseStruct>& bases)
{
    // For each class in the given slot, find the first parent class not in the slot.
    CIwAttrDesc* pDesc = m_Descs[Slot];
    if (pDesc)
    {
        std::vector<CIwAttrClass*>& classes = pDesc->m_Classes;

        for (int i=0; i<(int)classes.size(); ++i)
        {
            CIwAttrClass* pClass = classes[i];
            CIwAttrClass* pParent = pClass->m_Parent;
            while (pParent &&
                   std::find(classes.begin(), classes.end(), pParent) != classes.end())
            {
                pParent = pParent->m_Parent;
            }
            ClassBaseStruct cb = { pClass, pParent };
            bases.push_back(cb);
        }
    }
}

//-----------------------------------------------------------------------------
//sub sections etc, you can add, checks if they have already been added
void CIwAttrInstance::GetAddStrings(wxArrayString& Strings,CIwAttrClass* Class,bool ordered)
{
    for (int i=0; i<(int)Class->m_Members.size(); i++)
    {
        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
        {
            GetAddStrings(Strings,Class->m_Members[i]->m_Items[0].m_Class,ordered);
            continue;
        }

        if (!ordered)
            if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CLASS &&
                (Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_GROUP)
                continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_READONLY)
            continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_FROMCHILD)
            continue;

        if (Class->m_Members[i]->m_Type&ATTRMEMBER_1ORMORE || ordered)
            Strings.push_back(Class->m_Members[i]->m_Name);
        else if (Class->m_Members[i]->m_Type&ATTRMEMBER_0OR1)
        {
            int j;
            for (j=0; j<(int)m_Data.size(); j++)
            {
                if (m_Data[j]->m_Member==Class->m_Members[i])
                    break;
            }

            if (j==(int)m_Data.size())
                Strings.push_back(Class->m_Members[i]->m_Name);
        }

        if (m_Class->IsTemplateMember(Class->m_Members[i]))
            GetAddStrings(Strings,Class->m_Members[i]->m_Items[0].m_Class,ordered);

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
        {
            CIwAttrData* data=FindData(Class->m_Members[i]->m_Name,0);
            if (data!=NULL && Class->m_Members[i]->m_Items[data->m_Items[0].m_Int].m_Class!=NULL)
                GetAddStrings(Strings,Class->m_Members[i]->m_Items[data->m_Items[0].m_Int].m_Class,ordered);
        }
    }

    if (Class->m_Parent!=NULL)
        GetAddStrings(Strings,Class->m_Parent,(Class->m_Parent->m_Flags&ATTRCLASS_ORDERED_F)!=0);
}

//-----------------------------------------------------------------------------
//get all sub section classes, excluding any in Exclude
void CIwAttrInstance::GetChildClasses(std::vector<CIwAttrClass*>& Classes,CIwAttrClass* Class,const wxChar** Exclude)
{
    for (int i=0; i<(int)Class->m_Members.size(); i++)
    {
        if (!Class->m_Members[i]->m_UseClassInAdd)
            continue;

        if (m_Class->IsTemplateMember(Class->m_Members[i]))
            GetChildClasses(Classes,Class->m_Members[i]->m_Items[0].m_Class,Exclude);

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_ENUM && Class->m_Members[i]->m_Items[0].m_Class!=NULL)
        {
            CIwAttrData* data=FindData(Class->m_Members[i]->m_Name,0);
            if (data!=NULL && Class->m_Members[i]->m_Items[data->m_Items[0].m_Int].m_Class!=NULL)
                GetChildClasses(Classes,Class->m_Members[i]->m_Items[data->m_Items[0].m_Int].m_Class,Exclude);
        }

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        if ((Class->m_Members[i]->m_Type&ATTRMEMBER_FROMCHILD)==ATTRMEMBER_FROMCHILD)
            continue;

        if (Exclude!=NULL)
        {
            int j;
            for (j=0; Exclude[j]!=NULL; j++)
            {
                if (Class->m_Members[i]->m_Name.IsSameAs(Exclude[j],false))
                    break;
            }

            if (Exclude[j]!=NULL)
                continue;
        }

        for (int k=0; k<(int)Class->m_Members[i]->m_Items.size(); k++)
        {
            if (Class->m_Members[i]->m_Items[k].m_Class==NULL)
                continue;

            if (Class->m_Members[i]->m_Type&ATTRMEMBER_1ORMORE)
                Classes.push_back(Class->m_Members[i]->m_Items[k].m_Class);
            else if (Class->m_Members[i]->m_Type&ATTRMEMBER_0OR1)
            {
                int j;
                for (j=0; j<(int)m_Data.size(); j++)
                {
                    if (m_Data[j]->m_Member==Class->m_Members[i])
                        break;
                }

                if (j==(int)m_Data.size())
                    Classes.push_back(Class->m_Members[i]->m_Items[k].m_Class);
            }
        }
    }

    if (Class->m_Parent!=NULL)
        GetChildClasses(Classes,Class->m_Parent,Exclude);
}

//-----------------------------------------------------------------------------
//get list of values for pointer data
void CIwAttrInstance::GetItemStrings(const wxString& Prefix,wxArrayString& Strings,CIwAttrData* Data)
{
    bool GotClass=false;
    wxString Line=Prefix;

    if (Data==NULL || (Data->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_PTR)
        GotClass=true;
    else
    {
        for (CIwAttrClass* Class=m_Class; Class!=NULL && !GotClass; Class=Class->m_Parent)
        {
            for (int i=0; i<(int)Data->m_Member->m_Items.size(); i++)
            {
                if (Class==Data->m_Member->m_Items[i].m_Class)
                {
                    GotClass=true;
                    break;
                }
            }
        }
    }

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        switch (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_STRING:
            if (m_Data[i]->m_Member->m_Name.IsSameAs(wxT("name"),false))
            {
                Line+=m_Data[i]->m_Items[0].m_String;
                if (GotClass)
                    Strings.Add(Line);

                Line+=wxT(":");
            }

            break;
        case ATTRMEMBER_CHILD:
        case ATTRMEMBER_CLASS:
            m_Data[i]->m_Items[0].m_Inst->GetItemStrings(Line,Strings,Data);
            break;
        }
    }
}

//-----------------------------------------------------------------------------
bool CIwAttrInstance::HasExportField(const wxString& Field)
{
    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if (m_Data[i]->m_Member->m_ExportSection.IsSameAs(Field,false))
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
bool CIwAttrInstance::HasExtraField(const wxString& Field)
{
    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if (m_Data[i]->m_Member->m_ExtraSection.IsSameAs(Field,false))
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
//find sub section with data name is Name
CIwAttrInstance* CIwAttrInstance::FindChild(const wxString& Name,const wxString& Class,bool checkDerivedClasses)
{
    CIwAttrInstance* Inst=NULL;

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        switch (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_STRING:
            if (m_Data[i]->m_Member->m_Name.IsSameAs(wxT("name"),false))
            {
                if (m_Data[i]->m_Items[0].m_String.IsSameAs(Name,false))
                {
                    if (Class.empty())
                        return this;

                    if (m_Class->m_Name.IsSameAs(Class,false))
                        return this;

                    if (checkDerivedClasses)
                        for (CIwAttrClass* klass=m_Class; klass!=NULL; klass=klass->m_Parent)
                        {
                            if (klass->m_Name.IsSameAs(Class,false))
                                return this;
                        }
                }
            }

            break;
        case ATTRMEMBER_CHILD:
        case ATTRMEMBER_CLASS:
            Inst=m_Data[i]->m_Items[0].m_Inst->FindChild(Name,Class,checkDerivedClasses);
            if (Inst!=NULL)
                return Inst;

            break;
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
CIwAttrData* CIwAttrInstance::FindData(const wxString& Name,int mode)
{
    CIwAttrData* Data=NULL;

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if ((mode&FINDMODE_EXPORTERTAG)==FINDMODE_EXPORTERTAG)
        {
            if (m_Data[i]->m_Member->m_ExportSection.IsSameAs(Name,false))
                return m_Data[i];
        }
        else if ((mode&FINDMODE_EXTRATAG)==FINDMODE_EXTRATAG)
        {
            if (m_Data[i]->m_Member->m_ExtraSection.IsSameAs(Name,false))
                return m_Data[i];
        }
        else if ((mode&FINDMODE_VIEWERTAG)==FINDMODE_VIEWERTAG)
        {
            if (m_Data[i]->m_Member->m_ViewerType.IsSameAs(Name,false))
                return m_Data[i];
        }
        else if (m_Data[i]->m_Member->m_Name.IsSameAs(Name,false))
            return m_Data[i];

        switch (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_CHILD:
        case ATTRMEMBER_CLASS:
            if ((mode&FINDMODE_RECURSIVE)==0) break;

            Data=m_Data[i]->m_Items[0].m_Inst->FindData(Name,mode);
            if (Data!=NULL)
                return Data;

            break;
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
//find sub section that is called Class
CIwAttrInstance* CIwAttrInstance::FindChildClass(const wxString& Class,int num)
{
    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
        {
            if (m_Data[i]->m_Member->m_Name.IsSameAs(Class,false))
            {
                if (num==0)
                    return m_Data[i]->m_Items[0].m_Inst;
                else
                    num--;
            }
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
//fill notes from instance
void CIwAttrInstance::FillNote(std::vector<CIwAttrNote>& Notes,bool sticky,int ID,int& ID2,CIwAttrData* data,bool newDefReqs)
{
    CIwAttrNote Note;

    if (data!=NULL && ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS || (data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR))
        Note.m_Name=data->m_Member->m_Name;
    else
        Note.m_Name=m_Class->m_Name;

    if (sticky)
        Note.m_Info=1;
    else
        Note.m_Info=0;

    int ID1=++ID2;
    Note.m_Data=wxString::Format(wxT("{%d"),ID1);
    Note.m_ID=ID;

    Notes.push_back(Note);

    ID2=FillNotes(Notes,false,ID2,newDefReqs);

    Note.m_Name=wxT("}");
    Note.m_Data=wxT("");
    Note.m_ID=ID1;

    Notes.push_back(Note);
}

//-----------------------------------------------------------------------------
//fill notes from instance
int CIwAttrInstance::FillNotes(std::vector<CIwAttrNote>& Notes,bool AddClass,int ID,bool newDefReqs)
{
    int i;
    int ID2=ID;
    for (i=0; i<(int)m_Data.size(); i++)
    {
        if (m_Data[i]->m_Member->m_ExportSection.size()>0 && m_Mgr->m_IsExporter)
        {
            if (!(m_Data[i]->m_Member->m_Type&ATTRMEMBER_CHANGEABLE))
            {
                if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_NULLABLE)
                {
                    if ((m_Data[i]->m_Items[0].m_Flags&ATTRITEM_OVERRIDE_F)==0)
                        continue;
                }
                else
                    continue;
            }
        }

        if (newDefReqs)
        {
            if (m_Data[i]->m_FromDefault)
                continue;
        }
        else if (m_Data[i]->IsDefault() && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_NODEFAULT)==0)
            continue;

        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_LIST && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
        {
            CIwAttrNote Note;
            Note.m_Name=m_Data[i]->m_Member->m_Name;
            if (m_Data[i]->m_Member->m_Sticky)
                Note.m_Info=1;
            else
                Note.m_Info=0;

            Note.m_Data=wxString::Format(wxT("{%d"),++ID2);
            Note.m_ID=ID;
            Notes.push_back(Note);

            int ID1=ID2;
            for (int j=0; j<(int)m_Data[i]->m_Items.size(); j++)
            {
                wxString Line;
                CIwAttrInstance* Inst=m_Data[i]->ToString(Line,j);

                if (Inst!=NULL)
                    Inst->FillNote(Notes,m_Data[i]->m_Member->m_Sticky,ID1,ID2,m_Data[i],newDefReqs);
                else
                {
                    if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
                        Note.m_Name=m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name;
                    else
                        Note.m_Name=m_Data[i]->m_Member->m_Name;

                    Note.m_Data=Line;
                    Note.m_ID=ID1;

                    Notes.push_back(Note);
                }
            }

            Note.m_Name=wxT("}");
            Note.m_Data=wxT("");
            Note.m_ID=ID1;
            Notes.push_back(Note);
            continue;
        }

        wxString Line;
        CIwAttrInstance* Inst=m_Data[i]->ToString(Line);

        if (Inst!=NULL)
            Inst->FillNote(Notes,m_Data[i]->m_Member->m_Sticky,ID,ID2,m_Data[i],newDefReqs);
        else
        {
            CIwAttrNote Note;

            if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
                Note.m_Name=m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name;
            else
                Note.m_Name=m_Data[i]->m_Member->m_Name;

            if (m_Data[i]->m_Member->m_Sticky)
                Note.m_Info=1;
            else
                Note.m_Info=0;

            Note.m_Data=Line;
            Note.m_ID=ID;

            Notes.push_back(Note);
        }
    }

    //in root section export class
    if (AddClass)
    {
        CIwAttrNote Note;

        Note.m_Name=wxT("class");
        Note.m_Info=-1;
        Note.m_Data=m_Class->m_Name.Mid(1);

        Notes.push_back(Note);
    }

    return ID2;
}

//-----------------------------------------------------------------------------
//get data for file export, not saving if in exclude list
wxString CIwAttrInstance::WriteNotes(int Indent,const wxChar** Exclude,bool SkipComments,bool ForLive,bool Valid,bool NoChildren)
{
    int i,j;
    wxString Notes;

    for (i=0; i<(int)m_Data.size(); i++)
    {
        if (ForLive && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            continue;
        else if (ForLive && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_ENUM &&
                 (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_STRING && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_DATA)
        {
            if (m_Data[i]->m_Instance->m_File!=NULL && m_Data[i]->m_Instance->m_File->m_Override!=NULL)
                if (m_Data[i]->m_Instance->m_File->m_Override->OverrideDefault(m_Data[i],i,false))
                    continue;

        }
        else if (m_Data[i]->IsDefault() && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_NODEFAULT)==0)
        {
            if (m_Data[i]->m_Instance->m_File!=NULL && m_Data[i]->m_Instance->m_File->m_Override!=NULL)
            {
                if (m_Data[i]->m_Instance->m_File->m_Override->OverrideDefault(m_Data[i],i,true))
                    continue;
            }
            else
                continue;

            //if(m_Data[i]->m_Group==NULL)
            //	continue;
            //if((m_Data[i]->m_Group->m_Member->m_Type&ATTRMEMBER_0ORMORE)==0)
            //	continue;
        }

        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_SKIP)
            continue;

        if (Exclude!=NULL)
        {
            for (j=0; Exclude[j]!=NULL; j++)
            {
                if (m_Data[i]->m_Member->m_Name.IsSameAs(Exclude[j],false))
                    break;
            }

            if (Exclude[j]!=NULL)
                continue;
        }

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
            continue;

        if (m_Data[i]->m_Member->m_ViewerDead && ForLive)
            continue;

        if (Valid && (m_Data[i]->m_Member->m_Parent->m_Flags&(ATTRCLASS_VIEWERGROUP_F|ATTRCLASS_TEMP_F))==0)
            continue;

        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_LIST && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR && !NoChildren)
        {
            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }

            Notes+=m_Data[i]->m_Member->m_Name;
            Notes+=wxT(" {\n");

            for (j=0; j<(int)m_Data[i]->m_Items.size(); j++)
            {
                Notes+=WriteNotesInternal(m_Data[i],j,Indent+1,Exclude,SkipComments,ForLive,Valid,NoChildren);
            }

            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }
            Notes+=wxT("}\n");
            continue;
        }

        Notes+=WriteNotesInternal(m_Data[i],-1,Indent,Exclude,SkipComments,ForLive,Valid,NoChildren);
    }
    return Notes;
}

//-----------------------------------------------------------------------------
wxString CIwAttrInstance::WriteNotesInternal(CIwAttrData* data,int num,int Indent,const wxChar** Exclude,bool SkipComments,bool ForLive,bool Valid,bool NoChildren)
{
    int j,k;
    wxString Line;
    wxString Line2;
    wxString Notes;

    if (data->m_Instance->m_File!=NULL && data->m_Instance->m_File->m_Override!=NULL)
        if (data->m_Instance->m_File->m_Override->Override(data,num,Indent,Notes))
            return Notes;

    CIwAttrInstance* Inst=data->ToString2(Line,num);

    if (Inst!=NULL && NoChildren)
        return Notes;

    if (!SkipComments)
    {
        for (j=0; j<(int)data->m_PreComments.size(); j++)
        {
            for (k=0; k<Indent; k++)
            {
                Notes+=wxT("\t");
            }
            Notes+=data->m_PreComments[j]+wxT("\n");
        }
    }

    j=0;
    if (data->m_Member->m_Type&ATTRMEMBER_COMMENTED)
    {
        Notes+=wxT("//: ");
        j++;
    }

    for (; j<Indent; j++)
    {
        Notes+=wxT("\t");
    }

    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
    {
        Line2=data->m_Items[0].m_Inst->m_Class->m_Name;
        if (Valid)
        {
            CIwAttrClass* klass;
            for (klass=data->m_Items[0].m_Inst->m_Class; klass!=NULL; klass=klass->m_Parent)
            {
                if ((klass->m_Flags&(ATTRCLASS_VIEWERGROUP_F|ATTRCLASS_TEMP_F))!=0)
                    break;
            }
            if (klass!=NULL)
                Line2=klass->m_Name;
        }
    }
    else
        Line2=data->m_Member->m_Name;

    if (Inst!=NULL)
        Line2=Line2.AfterLast(wxT(':'));

    if ((data->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_RESOURCE && num==-1)
    {
        Notes+=Line2;
        Notes+=wxT(" ");
    }

    Notes+=Line;
    if (!SkipComments)
        Notes+=data->m_LineComment;

    Notes+=wxT("\n");

    if (Inst!=NULL && !NoChildren)
    {
        for (j=0; j<Indent; j++)
        {
            Notes+=wxT("\t");
        }

        Notes+=wxT("{\n");

        Notes+=Inst->WriteNotes(Indent+1,Exclude,SkipComments,ForLive,Valid,NoChildren);

        for (j=0; j<Indent; j++)
        {
            Notes+=wxT("\t");
        }
        Notes+=wxT("}\n");
    }

    if (data->m_Instance->m_File!=NULL && data->m_Instance->m_File->m_Override!=NULL)
        data->m_Instance->m_File->m_Override->Append(data,num,Indent,Notes);

    return Notes;
}

//-----------------------------------------------------------------------------
wxString CIwAttrInstance::WriteExtraNotes(int Indent,const wxChar** Exclude)
{
    wxString Notes;
    for (int i=0; i<(int)m_ExtraData.size(); i++)
    {
        int j;
        for (j=0; j<(int)m_Mgr->m_InvalidExtraNotes.size(); j++)
        {
            if (m_Mgr->m_InvalidExtraNotes[j].IsSameAs(m_ExtraData[i].m_Name,false))
                break;
        }
        if (j<(int)m_Mgr->m_InvalidExtraNotes.size())
            continue;

        if (Exclude!=NULL)
        {
            for (j=0; Exclude[j]!=NULL; j++)
            {
                if (m_ExtraData[i].m_Name.IsSameAs(Exclude[j],false))
                    break;
            }

            if (Exclude[j]!=NULL)
                continue;
        }

        if (!m_ExtraData[i].m_Data.empty() && m_ExtraData[i].m_Data[0]=='{')
            continue;

        for (j=0; j<Indent; j++)
        {
            Notes+=wxT("\t");
        }

        wxString data=m_ExtraData[i].m_Data;
        while (data[0]=='\"' && data[data.size()-1]=='\"')
            data=data.SubString(1,data.size()-2);

        Notes+=m_ExtraData[i].m_Name;
        Notes+=wxT(" ");
        Notes+=data;
        Notes+=wxT("\n");
    }
    return Notes;
}

//-----------------------------------------------------------------------------
//get data for file export, only saving if it is a valid viewer field, returns what classes it could be
wxString CIwAttrInstance::WriteValidNotes(int Indent,std::vector<CIwAttrClass*>& classes)
{
    wxString Notes;
    int i,j;
    bool First=true;
    int numRequired=0;

    for (i=0; i<(int)m_Data.size(); i++)
    {
        if (m_Data[i]->IsDefault() && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_NODEFAULT)==0)
        {
            if (m_Data[i]->m_Group==NULL)
                continue;

            if ((m_Data[i]->m_Group->m_Member->m_Type&ATTRMEMBER_0ORMORE)==0)
                continue;
        }

        //if(m_Data[i]->m_Member->m_Type&ATTRMEMBER_SKIP)
        //	continue;

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
            continue;

        wxString Line;
        wxString Line2;

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            Line2=m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name;
        else
            Line2=m_Data[i]->m_Member->m_Name;

        Line2=Line2.AfterLast(wxT(':')); //allow duplicate classes (ie called CCurve:1), all from the : is stripped

        bool Found=false;
        for (j=0; j<(int)m_Mgr->m_Descs.size(); j++)
        {
            for (int k=0; k<(int)m_Mgr->m_Descs[j]->m_ViewerFields.size(); k++)
            {
                if (m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_Primary.IsSameAs(Line2,false))
                    Found=true;

                for (int m=0; m<(int)m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_Alternatives.size(); m++)
                {
                    if (m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_Alternatives[m].IsSameAs(Line2,false))
                    {
                        Line2=m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_Primary;
                        Found=true;
                        break;
                    }
                }
                if (Found)
                {
                    if (m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_Required)
                        numRequired++;

                    if (First)
                    {
                        for (int m=0; m<(int)m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_InClasses.size(); m++)
                        {
                            classes.push_back(m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_InClasses[m]);
                        }
                        First=false;
                    }
                    else
                    {
                        for (int n=0; n<(int)classes.size(); n++)
                        {
                            int m;
                            for (m=0; m<(int)m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_InClasses.size(); m++)
                            {
                                if (classes[n]==m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_InClasses[m])
                                    break;
                            }
                            if (m==(int)m_Mgr->m_Descs[j]->m_ViewerFields[k]->m_InClasses.size())
                                classes[n]=NULL;
                        }
                    }

                    break;
                }
            }
            if (Found) break;
        }

        if (!Found) continue;

        CIwAttrInstance* Inst=m_Data[i]->ToString2(Line);


        j=0;
        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_COMMENTED)
        {
            Notes+=wxT("//: ");
            j++;
        }

        for (; j<Indent; j++)
        {
            Notes+=wxT("\t");
        }

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_RESOURCE)
        {
            Notes+=Line2;
            Notes+=wxT(" ");
        }

        Notes+=Line;
        Notes+=wxT("\n");

        if (Inst!=NULL)
        {
            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }

            Notes+=wxT("{\n");

            Notes+=Inst->WriteNotes(Indent+1,NULL,true);

            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }
            Notes+=wxT("}\n");
        }
    }

    for (int n=0; n<(int)classes.size(); n++)
    {
        if (classes[n]!=NULL)
            if (classes[n]->m_NumRequired>numRequired)
                classes[n]=NULL;

    }

    return Notes;
}

//-----------------------------------------------------------------------------
//as write notes, but also return information about points for the exporter to add extra data
void CIwAttrInstance::WriteNotes2(int Indent,wxString& Notes,std::vector<CIwAttrExtraNoteData>& Spec,wxString Name,const wxChar** Exclude)
{
    int i,j;

    for (i=0; i<(int)m_Data.size(); i++)
    {
        if (m_Data[i]->m_Member->m_Name.IsSameAs(wxT("name"),false))
            Name=m_Data[i]->m_Items[0].m_String;

        if (Exclude!=NULL)
        {
            for (j=0; Exclude[j]!=NULL; j++)
            {
                if (m_Data[i]->m_Member->m_Name.IsSameAs(Exclude[j],false))
                    break;
            }

            if (Exclude[j]!=NULL)
                continue;
        }

        if (m_Data[i]->m_Member->m_ExtraSection.size()>0)
            Spec.push_back(CIwAttrExtraNoteData(Notes.size(),Name,m_Data[i]->m_Member->m_ExtraSection,Indent,m_Data[i]->m_Member->m_Name));

        if (m_Data[i]->IsDefault() && (m_Data[i]->m_Member->m_Type&ATTRMEMBER_NODEFAULT)==0 && m_Data[i]->m_Group==NULL)
            continue;

        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_SKIP)
            continue;

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
            continue;

        wxString Line;
        wxString Line2;
        CIwAttrInstance* Inst=m_Data[i]->ToString2(Line);

        for (j=0; j<(int)m_Data[i]->m_PreComments.size(); j++)
        {
            Line+=m_Data[i]->m_PreComments[j]+wxT("\n");
        }

        j=0;
        if (m_Data[i]->m_Member->m_Type&ATTRMEMBER_COMMENTED)
        {
            Notes+=wxT("//: ");
            j++;
        }

        for (; j<Indent; j++)
        {
            Notes+=wxT("\t");
        }

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            Line2=m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name;
        else
            Line2=m_Data[i]->m_Member->m_Name;

        if (Inst!=NULL)
        {
            Line2=Line2.AfterLast(wxT(':')); //allow duplicate classes (ie called CCurve:1), all from the : is stripped


        }

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_RESOURCE)
        {
            Notes+=Line2;
            Notes+=wxT(" ");
        }

        Notes+=Line;
        Notes+=m_Data[i]->m_LineComment;
        Notes+=wxT("\n");

        if (Inst!=NULL)
        {
            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }

            Notes+=wxT("{\n");

            Inst->WriteNotes2(Indent+1,Notes,Spec,Name,Exclude);

            for (j=0; j<Indent; j++)
            {
                Notes+=wxT("\t");
            }
            Notes+=wxT("}\n");
        }
    }
}


//--------------------------------------------------------------------------------
int CIwAttrData::Index()
{
    if (m_Instance==NULL) return -1;

    for (int i=0; i<(int)m_Instance->m_Data.size(); i++)
    {
        if (m_Instance->m_Data[i]==this)
            return i;
    }

    return -1;
}

//-----------------------------------------------------------------------------
//name of the class
wxString CIwAttrInstance::GetClassName()
{
    return m_Class->m_Name;
}

//-----------------------------------------------------------------------------
//is this class allowed for export
bool CIwAttrInstance::IsLegal()
{
    if ((m_Class->m_Flags&(ATTRCLASS_BASE_F|ATTRCLASS_TEMP_F))==0)
        return true;

    for (int i=0; i<(int)m_Mgr->m_Descs.size(); i++)
    {
        if (m_Mgr->m_Descs[i]!=NULL)
            if (m_Mgr->m_Descs[i]->m_Restrict)
                return false;

    }
    return true;
}

//-----------------------------------------------------------------------------
wxString CIwAttrInstance::GetTreeName(bool shortName)
{
    wxString Line;

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_STRING)
        {
            if (m_Data[i]->m_Member->m_Name.IsSameAs(wxT("name"),false))
            {
                Line=m_Data[i]->m_Items[0].m_String;
                break;
            }
        }
    }
    if (Line.empty())
        Line=L"???";

    if (!shortName)
        Line+=wxString::Format(wxT(" - (%s)"),m_Class->m_Name.c_str());

    return Line;
}

//-----------------------------------------------------------------------------
//add section info to tree control
void CIwAttrInstance::AddToTree(wxTreeCtrl *Tree,wxTreeItemId Id)
{
    int i;
    wxString Line=GetTreeName();

    m_TreeId=Tree->AppendItem(Id,Line);
    Tree->SetItemData(m_TreeId,new CIwAttrTreeItem(this));

    for (i=0; i<(int)m_Data.size(); i++)
    {
        if (((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)||
            ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS))
            m_Data[i]->m_Items[0].m_Inst->AddToTree(Tree,m_TreeId);
    }
}

//-----------------------------------------------------------------------------
//make a control for this data item
void CIwAttrData::GetDlgItem(wxWindow* Parent,wxSizer* Sizer,std::vector<CIwAttrData*>& Alts,int OrigNum,const wxChar** Exclude,bool readOnly)
{
    wxSizer* Sizer2=NULL;
    wxArrayString Strings;
    std::vector<CIwAttrClass*> Classes;
    std::vector<CIwAttrInstance*> Bases;
    CIwStyleCtrlGroup* Group=(CIwStyleCtrlGroup*)Sizer;
    CIwStyleCtrlGroup* Group2=NULL;
    int i,j,Num=OrigNum;

    if (m_Group!=NULL && (m_Group->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)==ATTRITEM_HIDDEN_F)
        return;

    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_LIST) //make a set of controls for a list item
    {
        //Group2=new CIwStyleCtrlGroup(false,true,true);
        for (i=0; i<(int)m_Items.size(); i++)
        {
            GetDlgItem(Parent,Group,Alts,i,Exclude,readOnly);

            CIwAttrRemoveListButton* button=new CIwAttrRemoveListButton(Parent,this,i);
            if (readOnly) button->Enable(false);

            Group->Add(button);
        }
        CIwAttrAddListButton* button=new CIwAttrAddListButton(Parent,this);
        if (readOnly) button->Enable(false);

        Group->Add(button,GetName());
        return;
    }

    if (Num==-1 && m_Items.empty())
        return;

    if (Num==-1 && m_Member->m_Type&ATTRMEMBER_ARRAY)    //make a set of controls for a list item
    {
        Sizer2=new wxBoxSizer(wxHORIZONTAL);
        std::vector<int> boundary;
        if (m_Member->m_Boundary.size()>1)
            boundary=m_Member->m_Boundary;
        else if (m_Member->m_ArraySize>4)
        {
            for (i=2; (float)m_Member->m_ArraySize/i>4.0f; i++)
            {
                ;
            }
            for (j=0; j<m_Member->m_ArraySize; j+=m_Member->m_ArraySize/i)
            {
                boundary.push_back(j+m_Member->m_ArraySize/i);
            }
        }

        for (i=0,j=0; i<m_Member->m_ArraySize; i++)
        {
            // Split bounded items over multiple lines
            if (boundary.size()>1 && i==boundary[j])
            {
                Sizer2->Layout();
                j++;
                Group->Add(Sizer2,GetName()+wxString::Format(L"(%d)",j),Parent,1);
                Sizer2=new wxBoxSizer(wxHORIZONTAL);
            }

            GetDlgItem(Parent,Sizer2,Alts,i,Exclude,readOnly);
        }

        Sizer2->Layout();
        j++;
        Group->Add(Sizer2,boundary.size()>1 ? GetName()+wxString::Format(L"(%d)",j) : GetName(),Parent,1);
        return;
    }

    if (Num==-1)
        Num=0;

    wxWindow* Win=NULL;
    if (m_Member->m_ExtraSection.size()>0)
    {
        Win=new wxTextCtrl(Parent,wxID_ANY,wxString::Format(wxT("Exporter inserts %s here"),m_Member->m_ExtraSection.c_str()));
        Win->Enable(false);
    }
    else
    {
        // Check for an overloaded member dialog item
        if (m_Instance->m_Dialog!=NULL)
            Win = m_Instance->m_Dialog->GetDlgItem(Parent,this,Num,Alts);

        // Build default item if no overload
        if (!Win)
        {
            switch (m_Member->m_Type&ATTRMEMBER_MASK)
            {
            case ATTRMEMBER_BOOL:   //checkbox
                Win=new CIwAttrCheckBox(Parent,this,Num,GetName(),Alts);
                break;
            case ATTRMEMBER_BYTE:   //text
            case ATTRMEMBER_SHORT:  //text
            case ATTRMEMBER_INT:        //text
            case ATTRMEMBER_INT124:     //text
                Win=new CIwAttrTextInt(Parent,this,Num,Alts);
                break;
            case ATTRMEMBER_FLOAT:  //text
                Win=new CIwAttrTextFloat(Parent,this,Num,Alts);
                break;
            case ATTRMEMBER_RESOURCE:
            {
                wxPanel* panel=new wxPanel(Parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),wxSUNKEN_BORDER);
                Group2=new CIwStyleCtrlGroup(true,true,true);
                panel->SetSizer(Group2);

                wxSizer* sizer=new wxBoxSizer(wxHORIZONTAL);
                Group2->Add(sizer,L"File",panel);
                CIwAttrText* text=new CIwAttrText(panel,this,Num,250,Alts);
                sizer->Add(text);
                sizer->Add(new CIwAttrTemplateBrowseButton(panel,this,text,Alts));

                Group2->Add(new CIwAttrTemplateAddRemoveButton(panel,this));

                if (m_Items[1].m_Inst!=NULL)
                {
                    Bases.clear();
                    Bases.push_back(m_Items[1].m_Inst);
                    for (i=0; i<(int)Alts.size(); i++)
                    {
                        Bases.push_back(Alts[i]->m_Items[1].m_Inst);
                    }

                    m_Mgr->FillMultiDialog(Group2,panel,Bases,NULL);
                }

                Group2->Layout();
                Win=panel;
            }
            break;
            case ATTRMEMBER_DATA:
            case ATTRMEMBER_STRING: //text
                Win=new CIwAttrText(Parent,this,Num,150,Alts);
                break;
            case ATTRMEMBER_USETEMPLATE:
            {
                wxPanel* panel=new wxPanel(Parent);
                wxSizer* sizer=new wxBoxSizer(wxHORIZONTAL);
                sizer->Add(new CIwAttrText(panel,this,0,100,Alts));
                sizer->Add(new CIwAttrText(panel,this,1,100,Alts));
                panel->SetSizer(sizer);
                Win=panel;
            }
            break;
            case ATTRMEMBER_STRINGID: //text,button,static
                Win=new CIwAttrGuiPanelStringID(Parent,this,Num,Alts);
                break;

            case ATTRMEMBER_FILE:   //just use a textbox for now
                Win=new CIwAttrText(Parent,this,Num,150,Alts);
                break;
            case ATTRMEMBER_FILETEXTURE: //text,button
                Win=new CIwAttrGuiPanelFile(Parent,this,Num,ATTRSTRING_FILETEXTURE,Alts);
                break;
            case ATTRMEMBER_FILEFONT: //text,button
                Win=new CIwAttrGuiPanelFile(Parent,this,Num,ATTRSTRING_FILEFONT,Alts);
                break;

            case ATTRMEMBER_ALIGNMENT: //combo.combo
                Win=new CIwAttrGuiPanelAlignment(Parent,this,Num,Alts);
                break;
            case ATTRMEMBER_COLOUR: //text,text,text,text
            case ATTRMEMBER_COLOUR3: //text,text,text,text
                Win=new CIwAttrGuiPanelColour(Parent,this,Num,Alts);
                break;
            case ATTRMEMBER_COORD:  //checkbox,text
                Win=new CIwAttrGuiPanelCoord(Parent,this,Num,Alts);
                break;
            case ATTRMEMBER_ENUM:   //combo
                for (i=1; i<(int)m_Member->m_Text.size(); i++)
                {
                    Strings.Add(m_Member->m_Text[i]);
                }
                for (i=0; i<(int)Alts.size(); i++)
                {
                    if (Alts[i]->m_Items[Num].m_Int!=m_Items[Num].m_Int)
                    {
                        Strings.Insert(wxT(""),0);
                        break;
                    }
                }
                Win=new CIwAttrCombo(Parent,this,Num,Strings,Alts);
                break;

            case ATTRMEMBER_PTR:        //text,button
                Win=new CIwAttrGuiPanelPtr(Parent,this,Num,Alts);
                break;

            case ATTRMEMBER_TEMPLATE: //expand
                Bases.clear();
                if (m_Items[0].m_Inst==NULL) break;

                Bases.push_back(m_Items[0].m_Inst);
                for (i=0; i<(int)Alts.size(); i++)
                {
                    Bases.push_back(Alts[i]->m_Items[0].m_Inst);
                }
                m_Mgr->FillMultiDialog(Group,Parent,Bases,NULL);
                break;
            case ATTRMEMBER_DYNTEMPLATE: //combo
                Classes.push_back(m_Member->m_Parent);
                m_Mgr->GetDerivedClasses(Classes);
                m_Mgr->GetClassStrings(Classes,Strings);

                Strings.RemoveAt(0);
                Strings.Insert(wxT(""),0);

                Win=new CIwAttrDynTemplate(Parent,this,Num,Strings,Alts);
                break;
            case ATTRMEMBER_EXTRACLASS: //combo
                if (m_Instance->m_File!=NULL)
                    m_Mgr->GetClasses(Classes,m_Instance->m_File->m_CurrType);
                else
                    m_Mgr->GetClasses(Classes,EXPORTTYPE_GUI);

                m_Mgr->GetClassStrings(Classes,Strings);

                Win=new CIwAttrDynTemplate(Parent,this,Num,Strings,Alts);
                break;
            case ATTRMEMBER_GROUP:  //section,button
                //Win=new CIwStyleLabelOD(Parent,wxID_ANY,wxT("Group"));
                if (Alts.size()>0)
                    break;

                Win=new CIwAttrGroupToggle(Parent,(m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)==0,this,m_Instance,Alts);
                break;
            case ATTRMEMBER_CHILD:  //section[,button]
                for (i=0; i<(int)m_Member->m_Items.size(); i++)
                {
                    Classes.push_back(m_Member->m_Items[i].m_Class);
                }
                m_Mgr->GetDerivedClasses(Classes);
                m_Mgr->GetClassStrings(Classes,Strings);

                Win=new wxPanel(Parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),wxSUNKEN_BORDER);
                Group2=new CIwStyleCtrlGroup(true,true,true);
                ((wxPanel*)Win)->SetSizer(Group2);

                Bases.clear();
                Bases.push_back(m_Items[0].m_Inst);
                for (i=0; i<(int)Alts.size(); i++)
                {
                    Bases.push_back(Alts[i]->m_Items[0].m_Inst);
                }

                m_Mgr->FillMultiDialog(Group2,Win,Bases,&Strings);
                Group2->Layout();
                break;
            case ATTRMEMBER_CLASS:  //section
                Win=new wxPanel(Parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),wxSUNKEN_BORDER);
                Group2=new CIwStyleCtrlGroup(true,true,true);
                ((wxPanel*)Win)->SetSizer(Group2);

                Bases.clear();
                Bases.push_back(m_Items[0].m_Inst);
                for (i=0; i<(int)Alts.size(); i++)
                {
                    Bases.push_back(Alts[i]->m_Items[0].m_Inst);
                }

                m_Mgr->FillMultiDialog(Group2,Win,Bases,NULL);
                Group2->Layout();
                break;
            }
        }
    }

    if (Win!=NULL)
    {
        if ((m_Member->m_Type&(ATTRMEMBER_CHANGEABLE|ATTRMEMBER_NULLABLE))!=0)
        {
            if (OrigNum==-1)
            {
                Sizer2=new wxBoxSizer(wxHORIZONTAL);

                Group->Add(Sizer2,GetName(),Parent,1);
            }
            else
                Sizer2=Sizer;

            Sizer2->Add(Win,1,wxEXPAND|wxRIGHT,8);
        }
        else if (OrigNum==-1 || (m_Member->m_Type&ATTRMEMBER_LIST)!=0)
            Group->Add(Win,GetName(),1);
        else if (OrigNum==m_Member->m_ArraySize-1)
            Sizer->Add(Win,1,wxEXPAND);
        else
            Sizer->Add(Win,1,wxEXPAND|wxRIGHT,8);

        if (m_Member->m_Type&ATTRMEMBER_READONLY || readOnly)
            Win->Enable(false);

        if (m_Member->m_Type&ATTRMEMBER_NULLABLE && m_Mgr->m_IsExporter)
        {
            Win=new CIwAttrCheckBox(Parent,this,Num,wxT("Override?"),Alts,Win);

            if (OrigNum==m_Member->m_ArraySize-1)
                Sizer2->Add(Win,0,wxEXPAND|wxALIGN_CENTRE_VERTICAL);
            else
                Sizer2->Add(Win,0,wxEXPAND|wxRIGHT|wxALIGN_CENTRE_VERTICAL,8);

            Sizer2->Layout();
        }
    }
}

//-----------------------------------------------------------------------------
void CIwAttrInstance::SetupDlg(CIwAttrDialog* Dialog,CIwAttrFileGroup* file)
{
    m_Dialog=Dialog;
    if (file!=NULL)
        m_File=file;
    else
        file=m_File;

    for (int i=0; i<(int)m_Data.size(); i++)
    {
        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD || (m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
            m_Data[i]->m_Items[0].m_Inst->SetupDlg(Dialog,file);

        if ((m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
            for (int j=0; j<(int)m_Data[i]->m_Items.size(); j++)
            {
                if ((m_Data[i]->m_Items[j].m_Flags&ATTRITEM_ALLOCED_F) && m_Data[i]->m_Items[j].m_Inst!=NULL)
                    m_Data[i]->m_Items[j].m_Inst->SetupDlg(Dialog,file);
            }
    }
}

//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwAttrTextCombo, wxComboBox)
    EVT_COMBOBOX(CTRLID_COMBO,CIwAttrTextCombo::OnCombo)
    EVT_TEXT(CTRLID_COMBO,CIwAttrTextCombo::OnCombo)
END_EVENT_TABLE()

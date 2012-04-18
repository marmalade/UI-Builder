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
#ifndef IW_ATTR_DESC_H
#define IW_ATTR_DESC_H

#include "wxInclude.h"

// Simple Stub class to ensure proper destruction of ExtraData in WinProjectFiles
class CIwBase
{
public:
    virtual ~CIwBase(){};

    // Return true to skip default save process
    virtual bool SaveExtra(wxTextFile& fp){return false; }
};

#define cBR_OP wxT('{')
#define cBR_CL wxT('}')
#define strBR_OP wxT("{")
#define strBR_CL wxT("}")

#include "IwStyleSheet/IwStyleSheet.h"

class CIwAttrDescMgr;
class CIwAttrClass;
class CIwAttrMember;
class CIwAttrInstance;
class CIwAttrData;
class CIwAttrFileGroup;

//-----------------------------------------------------------------------------

// Use linked list of IIwAttrDataEditor so we can clear CIwAttrData ptrs,
// held by widgets, when they are destroyed.
class IIwAttrDataEditor
{
public:
    IIwAttrDataEditor(CIwAttrData* pData);
    ~IIwAttrDataEditor();

protected:
    void ClearData(CIwAttrData* pData);

    friend class CIwAttrData;
    CIwAttrData* m_Data;
    IIwAttrDataEditor* m_NextEditor;
};

//-----------------------------------------------------------------------------
//matches object type
enum CIwAttrExportType      //MUST CHANGE exportTypeMap if you change this
{EXPORTTYPE_WORLD,
 EXPORTTYPE_MATERIAL,
 EXPORTTYPE_SELSET,
 EXPORTTYPE_MESH,
 EXPORTTYPE_CAMERA,
 EXPORTTYPE_JOINT,
 EXPORTTYPE_CURVE,
 EXPORTTYPE_GUI,
 EXPORTTYPE_LODGROUP,
 EXPORTTYPE_GFX,
 EXPORTTYPE_GROUP,
 EXPORTTYPE_TEMPLATE,
 EXPORTTYPE_TRANSFORM,
 EXPORTTYPE_TIMELINE,
 EXPORTTYPE_WORLDIMPORT,
 EXPORTTYPE_NUM,};
//-----------------------------------------------------------------------------
//member of class'e type
enum CIwAttrMemberType
{
    ATTRMEMBER_BOOL,        //i,i,checkbox
    ATTRMEMBER_BYTE,        //i,i,text
    ATTRMEMBER_SHORT,       //i,i,text
    ATTRMEMBER_INT,         //i,i,text
    ATTRMEMBER_FLOAT,       //f,f,text
    ATTRMEMBER_STRING,      //s,s,text
    ATTRMEMBER_STRINGID,    //i,i,text,button,static
    ATTRMEMBER_DATA,        //s,s,text (no quotes)

    ATTRMEMBER_FILE,        //s,s,text,button
    ATTRMEMBER_FILETEXTURE, //s,s,text,button
    ATTRMEMBER_FILEFONT,    //s,s,text,button

    ATTRMEMBER_ALIGNMENT,   //i,i,combo.combo
    ATTRMEMBER_COLOUR,      //i,i,text,text,text,text
    ATTRMEMBER_COLOUR3,     //i,i,text,text,text
    ATTRMEMBER_COORD,       //i/f,i/f,checkbox,text
    ATTRMEMBER_ENUM,        //(s),i,combo

    ATTRMEMBER_PTR,         //(c),n,text,button
    ATTRMEMBER_CLASS,       //(c),n,section

    ATTRMEMBER_TEMPLATE,    //(c),_,expand
    ATTRMEMBER_GROUP,       //(m),_,section[,button]
    ATTRMEMBER_CHILD,       //(c),n,section[,button]
    ATTRMEMBER_RESOURCE,    //s,s,text
    ATTRMEMBER_USETEMPLATE, //s,s,text.test
    ATTRMEMBER_DYNTEMPLATE, //(c),_,combo,expand
    ATTRMEMBER_EXTRACLASS,  //(c),_,combo,expand
    ATTRMEMBER_INT124,      //i,i,text,swap

    ATTRMEMBER_MASK=255,

    ATTRMEMBER_0OR1=256,
    ATTRMEMBER_1ORMORE=512,
    ATTRMEMBER_0ORMORE=256+512,

    ATTRMEMBER_ARRAY=1024,
    ATTRMEMBER_UNSIGNED=2048,
    ATTRMEMBER_READONLY=4096,
    ATTRMEMBER_FROMCHILD=8192,
    ATTRMEMBER_SKIP=16384,
    ATTRMEMBER_HIDDEN=32768,
    ATTRMEMBER_NODEFAULT=65536,
    ATTRMEMBER_NULLABLE=131072,
    ATTRMEMBER_CHANGEABLE=262144,
    ATTRMEMBER_COMMENTED=524288,
    ATTRMEMBER_LIST=1048576,
    ATTRMEMBER_NOTINEXTRACLASS=2097152,
};

//-----------------------------------------------------------------------------
//class flags
enum CIwAttrClassFlags
{
    ATTRCLASS_BASE_F=1,
    ATTRCLASS_DEFAULT_F=2,
    ATTRCLASS_VIEWERREADY_F=4,
    ATTRCLASS_TEMP_F=8,
    ATTRCLASS_VIEWERGROUP_F=16,
    ATTRCLASS_ORDERED_F=32,
    ATTRCLASS_NOMESH_F=64,
};

//-----------------------------------------------------------------------------
//percent or absolute value
struct CIwAttrMemberCoord
{
    int   m_Int;
    float m_Float;
    bool  m_UseInt;
};

//-----------------------------------------------------------------------------
//class flags
enum CIwAttrMemberItemFlags
{
    ATTRITEM_NONE_F=0,
    ATTRITEM_ALLOCED_F=1,
    ATTRITEM_HIDDEN_F=2,
    ATTRITEM_FIRSTFRAME_F=4,
    ATTRITEM_OVERRIDE_F=8,
};

//-----------------------------------------------------------------------------
//actual item data
struct CIwAttrMemberItem
{
    CIwAttrMemberItem() :
        m_Int(0),      m_UInt(0), m_Float(0.0f), m_Class(NULL), m_Member(NULL), m_Inst(NULL),m_Flags(ATTRITEM_NONE_F)
    {
        m_Coord.m_Int=0;
        m_Coord.m_Float = 0.0f;
        m_Coord.m_UseInt = true;
    }

    int                m_Int;
    unsigned int       m_UInt;
    float              m_Float;
    CIwAttrClass*      m_Class;
    CIwAttrMember*     m_Member;
    CIwAttrInstance*   m_Inst;
    CIwAttrMemberCoord m_Coord;
    wxString           m_String;
    int                m_Flags; //CIwAttrMemberItemFlags
};

//-----------------------------------------------------------------------------
//definition of a member of a class
class CIwAttrMember
{
public:
    int m_Type; //CIwAttrMemberType
    wxString m_Name;
    std::vector<CIwAttrMemberItem> m_Items;
    int m_Indent;
    int m_ArraySize;
    int m_Limits[3];
    CIwAttrClass* m_Parent;

    wxString m_ExportSection;
    wxString m_ExtraSection;
    int m_ExportOffset;
    wxString m_ViewerType;
    wxString m_AttrEd; // Optional Editor Type to Override ASD Attr Editor Controls
    std::vector<wxString> m_AltNames;
    std::vector<wxString> m_Text;
    std::vector<wxString> m_Wizard;
    std::vector<int> m_Boundary;
    bool m_Sticky;
    bool m_ViewerDead;
    CIwAttrExportType m_ExportType;
    bool m_UseClassInAdd;
    wxString m_PreMember;
    int m_Priority;

    void SetupDefault();
    CIwAttrMember() : m_ViewerDead(false),m_UseClassInAdd(true),m_Priority(0) {}
    ~CIwAttrMember();
};

enum CIwAttrTimelineFlags
{
    TIMELINEFLAGS_NOCREATE=0,
    TIMELINEFLAGS_CREATESTART,
    TIMELINEFLAGS_CREATEKEYS,
    TIMELINEFLAGS_CREATEVISIBLE,

    TIMELINEFLAGS_CREATEMASK=3,

    TIMELINEFLAGS_CREATEFIRSTONLY=4,
    TIMELINEFLAGS_CREATEIFHIERROOT=8,
    TIMELINEFLAGS_CREATEIFANIM=16,
    TIMELINEFLAGS_CREATEIFCAMERA=32,
    TIMELINEFLAGS_CREATEIFKEY=64,
    TIMELINEFLAGS_CREATEIFMODEL=128,

    TIMELINEFLAGS_AFFECTSANIM=256,
    TIMELINEFLAGS_AFFECTSWORLD=512,
};

struct CIwAttrTimeline
{
    unsigned int                                         m_Flags; //CIwAttrTimelineFlags
    wxString                                             m_CreateClass;
    wxString                                             m_DurItem;
    wxString                                             m_DurStart;
    wxString                                             m_DurEnd;
    int                                                  m_Priority;

    CIwAttrTimeline() : m_Flags(TIMELINEFLAGS_NOCREATE), m_Priority(0) {}
    void ReadTimelineLine(const std::vector<wxString>& argv);
    int GetDurationWidth(std::vector<CIwAttrInstance*>& list,int start,float from,float* width);
    bool IsSameObject(CIwAttrInstance* a,CIwAttrInstance* b);
    CIwAttrData* GetDurationDataItem(CIwAttrInstance* inst,float* rate=NULL);
};

//-----------------------------------------------------------------------------
//class definition
class CIwAttrClass
{
public:
    wxString m_Name;
    std::vector<CIwAttrMember*> m_Members;
    std::vector<wxString> m_AltNames;
    CIwAttrClass* m_Parent;
    int m_Flags;    //CIwAttrClassFlags
    int m_NumRequired;
    CIwAttrTimeline m_TimeLine;
    std::map<wxString,wxString> m_Paths;
public:
    CIwAttrClass() : m_Parent(NULL),m_Flags(0),m_NumRequired(0) {}
    ~CIwAttrClass();
    CIwAttrMember* GetMember(const wxString& Name,CIwAttrMember** RetGroup=NULL);
    CIwAttrMember* GetGroupMember(const wxString& Name,CIwAttrMember* Group,CIwAttrMember** RetGroup=NULL);
    CIwAttrMember* GetMember(unsigned int Mask,CIwAttrMember** RetGroup=NULL);
    CIwAttrMember* GetGroupMember(unsigned int Mask,CIwAttrMember* Group,CIwAttrMember** RetGroup=NULL);
    CIwAttrMember* GetClassMember(CIwAttrClass* Class,CIwAttrMember** RetGroup=NULL);
    CIwAttrMember* GetGroupClassMember(CIwAttrClass* Class,CIwAttrMember* Group,CIwAttrMember** RetGroup=NULL);
    bool IsTemplateMember(CIwAttrMember* member);
};

//-----------------------------------------------------------------------------
//
class CIwAttrViewerField
{
public:
    wxString m_Ext;
    wxString m_Primary;
    std::vector<wxString> m_Alternatives;
    bool m_Required;

    std::vector<CIwAttrClass*> m_InClasses;

    CIwAttrViewerField() : m_Required(false) {}
};

//-----------------------------------------------------------------------------
struct CIwAttrDescFileExt
{
    CIwAttrClass*                  m_TemplateClass;
    std::vector<CIwAttrExportType> m_Types;

    CIwAttrDescFileExt() : m_TemplateClass(NULL) {}
};

//-----------------------------------------------------------------------------
//a set of classes
class CIwAttrDesc : public CIwBase
{
    CIwAttrDescMgr* m_Mgr;
    CIwAttrClass* m_CurrClass;
    CIwAttrMember* m_CurrMember;
    CIwAttrMember* m_CurrGroup;
    int m_Indent;
public:
    wxString m_FileName;
    wxString m_Version;
    bool m_Restrict;
    std::vector<CIwAttrClass*> m_Classes;
    std::vector<CIwAttrClass*> m_ExportTypes[EXPORTTYPE_NUM];
    std::vector<CIwAttrViewerField*> m_ViewerFields;
    std::map<wxString,CIwAttrDescFileExt> m_FileExtMap;
    wxString m_File;        //used by ASD
public:
    CIwAttrDesc(CIwAttrDescMgr* Mgr) : m_Mgr(Mgr),m_CurrClass(NULL),m_CurrMember(NULL),m_CurrGroup(NULL),m_Indent(0),m_Restrict(false) {}
    void Clear();
    bool Load(const wxString& FileName);
    void ReadLine(const std::vector<wxString>& argv,const wxString& Dir);
    CIwAttrClass* GetClass(const wxString& Name);
    ~CIwAttrDesc();

    int GetType(const std::vector<wxString>& argv,CIwAttrMember* Member);
    int GetDefault(const std::vector<wxString>& argv,CIwAttrMember* Member);
    void FindFieldInClasses(CIwAttrViewerField* field);
    void LoadIFX(const wxString& fileName,const wxString& shortName);
};

//-----------------------------------------------------------------------------
//format for intermediate and extra notes
struct CIwAttrNote
{
    wxString                    m_Name;
    int                         m_Info;
    wxString                    m_Data;
    int                         m_ID;

    CIwAttrNote() : m_Info(-1), m_ID(-1) {}
};

//-----------------------------------------------------------------------------
//data of a member of an instance
class CIwAttrData
{
public:
    CIwAttrDescMgr* m_Mgr;
    CIwAttrInstance* m_Instance;    // Back Pointer
    CIwAttrMember* m_Member;
    CIwAttrData* m_Group;
    int m_Info;
    std::vector<CIwAttrMemberItem> m_Items;
    std::vector<wxString> m_PreComments;
    wxString m_LineComment;
    bool m_FromDefault;

    IIwAttrDataEditor* m_Editor;

    void SetDefault(bool skipInstances=false);
    void Set(std::vector<wxString>& argv,CIwAttrInstance* Base);
    wxString GetName();
    wxString MakeName(const wxString& From);
    void GetDlgItem(wxWindow* Win,wxSizer* Sizer,std::vector<CIwAttrData*>& Alts,int Num=-1,const wxChar** Exclude=NULL,bool readOnly=false);
    CIwAttrInstance* ToString(wxString& Line,int Num=-1);
    CIwAttrInstance* ToString2(wxString& Line,int Num=-1);
    bool IsDefault();
    int Index();
    void SetChanged(bool Changed=true);

    CIwAttrData() : m_Mgr(NULL),m_Instance(NULL),m_Member(NULL),m_Group(NULL),m_Info(0),m_FromDefault(false),m_Editor(NULL) {}
    ~CIwAttrData();
};

//-----------------------------------------------------------------------------
//data for tree control
class CIwAttrTreeItem : public wxTreeItemData
{
public:
    CIwAttrTreeItem(CIwAttrInstance* Sect) : m_Inst(Sect) {}
    CIwAttrInstance* m_Inst;
};

//intermediate format
struct CIwAttrExtraNoteData
{
    wxString m_Name;
    wxString m_Type;
    wxString m_MemberName;
    int      m_Offset;
    int      m_Indent;
    CIwAttrExtraNoteData(int Offset,const wxString& Name,const wxString& Type,int Indent,const wxString& MemberName) :
             m_Offset(Offset),m_Indent(Indent) { m_Name=Name; m_Type=Type; m_MemberName=MemberName; }
};

//available lists of extra values
enum EIwAttrDialogStringType
{
    ATTRSTRING_LOCALISE,
    ATTRSTRING_FILE,
    ATTRSTRING_FILETEXTURE,
    ATTRSTRING_FILEFONT,
    ATTRSTRING_PTR,
};

//-----------------------------------------------------------------------------
//base class for dialog controls are in
class CIwAttrDialog : public CIwStyleDialog
{
public:
    CIwAttrDialog(wxWindow* Parent,const wxString& Title,int Style) : CIwStyleDialog(Parent,Title,wxSize(-1,-1),Style) {}
    virtual void Reset()=0;
    virtual void ScheduleReset()=0;
    virtual void SetChanged(bool Changed)=0;
    virtual wxString GetBaseDir(bool fileBase)=0;
    virtual void Changed(CIwAttrInstance* Inst){}
    virtual wxWindow* GetDlgItem(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts){return NULL; }
    virtual void GridOverride(wxGrid* grid,int startLine,int endLine,CIwAttrData* data) {}
    virtual CIwAttrData* OverrideDataForGrid(CIwAttrData* data,bool checkFirst) { return NULL; }
    virtual bool GridOverrideActivate(wxGrid* grid,CIwAttrData* data,std::vector<CIwAttrData*>& alts,int row,int col) { return false; }
    virtual void DealWithChange(CIwAttrData* data) {}
    virtual bool GridOverrideCheck(CIwAttrData* data) { return false; }
};

//-----------------------------------------------------------------------------
//concrete instance of a class
class CIwAttrInstance : public CIwBase
{
public:
    enum EIwAttrFindMode
    {
        FINDMODE_RECURSIVE=1,
        FINDMODE_EXPORTERTAG=2,
        FINDMODE_EXTRATAG=4,
        FINDMODE_VIEWERTAG=8,
    };

    CIwAttrDescMgr* m_Mgr;
    CIwAttrClass* m_Class;
    std::vector<CIwAttrData*> m_Data;
    std::vector<CIwAttrNote> m_ExtraData;
    int m_ClassInfo;
    wxTreeItemId m_TreeId;
    CIwAttrData* m_Parent;
    CIwAttrFileGroup* m_File;
    CIwAttrDialog* m_Dialog;
    CIwAttrClass* m_TempClass;
    int m_Temp;

    CIwAttrInstance() : m_File(NULL),m_Dialog(NULL),m_TempClass(NULL) {}
    ~CIwAttrInstance()
    {
        for (int i=0; i<(int)m_Data.size(); i++)
        {
            delete m_Data[i];
        }
        if (m_Class->m_Flags==ATTRCLASS_TEMP_F)
            delete m_Class;
    }
    void Reset(CIwAttrClass* Class);

    void SetupDlg(CIwAttrDialog* Dialog,CIwAttrFileGroup* file=NULL);
    void SetChanged(bool Changed) { if (m_Dialog!=NULL) m_Dialog->SetChanged(Changed); }
    void ResetDlg() { if (m_Dialog!=NULL) m_Dialog->Reset(); }

    void AddDefaults(CIwAttrClass* Class,CIwAttrData* group=NULL,bool inExtraClass=false);
    void RemoveDefaults(CIwAttrClass* Class);
    void AddGroupDefaults(CIwAttrMember* Member,CIwAttrData* Group);
    void FillNote(std::vector<CIwAttrNote>& Notes,bool sticky,int ID,int& ID2,CIwAttrData* data,bool newDefReqs=false);
    int FillNotes(std::vector<CIwAttrNote>& Notes,bool AddClass=true,int ID=0,bool newDefReqs=false);
    wxString GetClassName();
    CIwAttrInstance* AddFromNote(const wxString& Name,CIwAttrNote& Note,std::vector<wxString>& argv,CIwAttrInstance* Base);
    void GetChildClasses(std::vector<CIwAttrClass*>& Classes,CIwAttrClass* Class,const wxChar** Exclude=NULL);
    CIwAttrInstance* FindChild(const wxString& Name,const wxString& Class=wxT(""),bool checkDerivedClasses=false);
    CIwAttrInstance* FindChildClass(const wxString& Class,int num=0);
    CIwAttrData* FindData(const wxString& Name,int mode=FINDMODE_RECURSIVE);    //EIwAttrFindMode
    bool HasExportField(const wxString& Field);
    bool HasExtraField(const wxString& Field);
    void GetFromNotes(std::vector<CIwAttrNote>& Notes);
    bool IsLegal();
    wxString GetTreeName(bool shortName=false);
    void AddToTree(wxTreeCtrl *Tree,wxTreeItemId Id);
    void GetItemStrings(const wxString& Prefix,wxArrayString& Strings,CIwAttrData* Data);
    void GetAddStrings(wxArrayString& Strings,CIwAttrClass* Class,bool ordered);
    //CIwAttrData* FindNote(const char* )

    wxString WriteNotes(int Indent,const wxChar** Exclude=NULL,bool SkipComments=false,bool ForLive=false,bool Valid=false,bool NoChildren=false);
    wxString WriteNotesInternal(CIwAttrData* data,int num,int Indent,const wxChar** Exclude,bool SkipComments,bool ForLive,bool Valid,bool NoChildren=false);
    wxString WriteValidNotes(int Indent,std::vector<CIwAttrClass*>& classes);
    void WriteNotes2(int Indent,wxString& Notes,std::vector<CIwAttrExtraNoteData>& Spec,wxString Name,const wxChar** Exclude=NULL);
    wxString WriteExtraNotes(int Indent,const wxChar** Exclude=NULL);

    bool SaveExtra(wxTextFile& fp,bool addClassRoot=false,bool valid=false);
};

//-----------------------------------------------------------------------------
//base class for getting lists of extra values
class CIwAttrExtra
{
public:
    virtual void GetStrings(EIwAttrDialogStringType Type,wxArrayString& Strings,CIwAttrData* Data) {}
    virtual wxString GetString(EIwAttrDialogStringType Type,int Num) { return wxT(""); }
    virtual int GetViewerSize(wxSize* Size,bool* Portrait) { return -1; }
    virtual void DoWizard(CIwAttrData* Data) { }
};

//-----------------------------------------------------------------------------
class CIwAttrFileGroupWriteOverride
{
public:
    virtual bool Override(CIwAttrData* data,int num,int Indent,wxString& Notes)=0;
    virtual bool OverrideDefault(CIwAttrData* data,int num,bool initial)=0;
    virtual void Append(CIwAttrData* data,int num,int Indent,wxString& Notes)=0;
};

//-----------------------------------------------------------------------------
//a sub group within an instance
class CIwAttrFileGroup : public CIwBase
{
public:
    CIwAttrFileGroup* m_Parent;
    CIwAttrInstance* m_Inst;
    CIwAttrExportType m_CurrType;
    wxString m_Ext;
    wxString m_FileName;
    std::vector<CIwAttrFileGroup*> m_SubGroups;
    CIwAttrFileGroupWriteOverride* m_Override;

    CIwAttrFileGroup() : m_Parent(NULL),m_Inst(NULL),m_CurrType(EXPORTTYPE_NUM),m_Override(NULL) {}
    virtual ~CIwAttrFileGroup()
    {
        if (m_Inst!=NULL) delete m_Inst;

        for (int i=0; i<(int)m_SubGroups.size(); i++)
        {
            delete m_SubGroups[i];
        }
    }

    virtual CIwAttrMember* TryGetMember(const wxString& Name,CIwAttrInstance* inst) { return NULL; }
    virtual void BrowseForFile(wxString& fileName) {}

    virtual CIwAttrInstance* TryGetPtrItem(const wxString& Name,CIwAttrData* data) { return NULL; }
    virtual wxString MakeNew(EIwAttrDialogStringType type,CIwAttrData* data=NULL,int offset=0) { return L""; }
    virtual void GetPtrStrings(wxArrayString& strings,CIwAttrData* data) {}
    virtual void SetupInlinePtr(CIwAttrInstance* inst,CIwAttrMember* member) {}
    virtual bool CheckAllowMultiple(CIwAttrInstance* inst,CIwAttrMember* member,std::vector<wxString>& argv,bool more) { return more; }
    virtual wxBitmap* GetBitmap(CIwAttrData* data,EIwAttrDialogStringType type,bool* shouldHave=NULL) { if (shouldHave!=NULL) (*shouldHave)=false;

                                                                                                        return NULL; }
    virtual wxBitmap* GetIcon(const wxString& textName) { return NULL; }
};

//-----------------------------------------------------------------------------
class CIwAttrFileSubGroup : public CIwAttrFileGroup
{
public:
    CIwAttrFileSubGroup(CIwAttrFileGroup* parent,CIwAttrInstance* inst)
    {
        m_Parent=parent;
        m_Inst=inst;
        if (m_Parent!=NULL)
            m_Parent->m_SubGroups.push_back(this);
    }
    ~CIwAttrFileSubGroup() { m_Inst=NULL; }

    virtual CIwAttrMember* TryGetMember(const wxString& Name,CIwAttrInstance* inst) { return (m_Parent==NULL) ? NULL : m_Parent->TryGetMember(Name,inst); }
    virtual void BrowseForFile(wxString& fileName) { if (m_Parent!=NULL) m_Parent->BrowseForFile(fileName); }

    virtual CIwAttrInstance* TryGetPtrItem(const wxString& Name,CIwAttrData* data) { return (m_Parent==NULL) ? NULL : m_Parent->TryGetPtrItem(Name,data); }
    virtual void GetPtrStrings(wxArrayString& strings,CIwAttrData* data) { if (m_Parent!=NULL) m_Parent->GetPtrStrings(strings,data); }
    virtual void SetupInlinePtr(CIwAttrInstance* inst,CIwAttrMember* member) { if (m_Parent!=NULL) m_Parent->SetupInlinePtr(inst,member); }
    virtual bool CheckAllowMultiple(CIwAttrInstance* inst,CIwAttrMember* member,std::vector<wxString>& argv,bool more)
    {
        return (m_Parent==NULL) ? more : m_Parent->CheckAllowMultiple(inst,member,argv,more);
    }
};

//-----------------------------------------------------------------------------
//manage the attributes
class CIwAttrDescMgr
{
    struct LineStruct
    {
        wxString              m_Line;
        wxString              m_Data;
        std::vector<wxString> m_Args;
    };
public:
    bool m_IsExporter;
    bool m_IsWizard;
    static bool s_LiveEditing;
    std::vector<CIwAttrDesc*> m_Descs;
    std::vector<wxString> m_InvalidExtraNotes;
    CIwAttrExtra* m_Extra;
    CIwAttrClass m_DefaultClass;
    float m_FrameRate;
    wxString m_BaseDir;
public:
    CIwAttrDescMgr(bool IsExporter);
    ~CIwAttrDescMgr();
    void Clear();

    bool Load(const wxString& FileName,int Slot=-1);
    CIwAttrClass* GetClass(const wxString& Name);
    bool FindAlts(std::vector<CIwAttrInstance*>& Bases,std::vector<CIwAttrData*>& Alts,CIwAttrData* Data);
    CIwAttrClass* GetTemplateClassForExt(const wxString& Name);

    CIwAttrInstance* GetFromNotes(std::vector<CIwAttrNote>& Notes,CIwAttrExportType Type,bool Interactive=true,CIwAttrFileGroup* group=NULL);
    CIwAttrInstance* GetFromNotes(std::vector<CIwAttrNote>& Notes,std::vector<CIwAttrClass*>& Classes,bool Interactive=true,CIwAttrFileGroup* group=NULL);
    CIwAttrInstance* GetFromFile(const wxString& Path,CIwAttrExportType Type,CIwAttrFileGroup* group=NULL);
    CIwAttrInstance* GetFromFile2(const wxString& Path,std::vector<CIwAttrClass*>& Classes,CIwAttrFileGroup* group=NULL);  // called by defaultin GetFromFile but does not overwrite m_Descs[1]
    CIwAttrInstance* GetFromFile(const wxString& Path,CIwAttrExportType Type,const wxString& rootName,CIwAttrFileGroup* group=NULL);    //call to load a group of insts
    CIwAttrInstance* GetFromFile(const wxString& Path,CIwAttrFileGroup* group);
    CIwAttrInstance* MakeInstance(const wxString& rootName);

    CIwAttrInstance* GetFromExtraNotes(std::vector<CIwAttrNote>& inNotes,CIwAttrExportType Type,const wxString& rootName);
    CIwAttrInstance* GetFromExtraNotes2(std::vector<CIwAttrNote>& inNotes,CIwAttrFileGroup* group);

    void IntegrateFromFile(const wxString& Path,CIwAttrInstance* Inst,bool ignoreName=false);
    void IntegrateFromFile2(CIwAttrInstance* Inst,std::vector<wxString>& lines);

    void FillDialog(CIwStyleCtrlGroup* Sizer,wxWindow* Win,CIwAttrInstance* Base,wxArrayString* ClassStrings,const wxChar** Exclude=NULL,bool readOnly=false);  //sizer is assumed to be 3 cols wide
    void FillMultiDialog(CIwStyleCtrlGroup* Sizer,wxWindow* Win,std::vector<CIwAttrInstance*>& Bases,wxArrayString* ClassStrings,const wxChar** Exclude=NULL,bool readOnly=false);  //sizer is assumed to be 3 cols wide
    void GetClasses(std::vector<CIwAttrClass*>& Classes,CIwAttrExportType Type);
    void GetDerivedClasses(std::vector<CIwAttrClass*>& Classes);
    void GetClassStrings(std::vector<CIwAttrClass*>& Classes,wxArrayString& Strings);
    void DoAddWizard(CIwAttrInstance* Inst);

    // check the members & hierarchy of this class to see if it has a useGeo field
    bool DoesClassContainUseGeo( const wxString& name );
    bool DoesClassContainUseGeo( CIwAttrClass *pClass );
    void GetAllMembersOfClass( const wxString& className, std::list<CIwAttrMember*>& members );

    //returns true and adds correct file extension to name if type is valid
    bool GetMemberFiletype(const wxString& type,wxString& name);
    CIwAttrClass* MakeContainer(CIwAttrExportType type,const wxString& rootName);
    CIwAttrClass* MakeContainer(const wxString& ext,std::vector<CIwAttrClass*>& Classes);

    wxComboBox* AddTypesSetup(CIwStyleCtrlGroup* Sizer,wxWindow* Win,CIwAttrFileGroup* group);
    void GetResTemplateStrings(wxArrayString& Strings);
    wxString FindResTemplateString(const wxString& name);
    void MakeIntoNote(CIwAttrNote& note,const wxString& string);
    wxString GetMaterials();

    bool ReadFile(const wxString& Path,std::vector<LineStruct>& lines);
    int ReadPtrArray(CIwAttrInstance* Inst,std::vector<wxString>& argv,int arg,std::vector<LineStruct>& lines,int line);

    struct ClassBaseStruct
    {
        CIwAttrClass* m_Class;
        CIwAttrClass* m_Base;
    };
    // for classes in the given slot, find the base classes in different slot
    void FindClassBases(int Slot, std::vector<ClassBaseStruct>& bases);
};


//-----------------------------------------------------------------------------
// text data control
class CIwAttrTextCombo : public wxComboBox
{
protected:
    CIwAttrData* m_Data;
    std::vector<CIwAttrData*> m_Alts;
    int m_Num;
    enum { CTRLID_COMBO };
public:
    CIwAttrTextCombo(wxWindow* Parent,CIwAttrData* Data,int Num,wxArrayString& Strings,std::vector<CIwAttrData*>& Alts) :
        wxComboBox(Parent,CTRLID_COMBO,Data->m_Items[Num].m_String,wxPoint(-1,-1),wxSize(-1,-1),Strings,0),m_Data(Data),m_Alts(Alts),m_Num(Num)
    {
        SetToolTip(wxString::Format(wxT("Change %s here"),Data->GetName().c_str()));

        SetValue(m_Data->m_Items[m_Num].m_String);
    }

    virtual void OnCombo(wxCommandEvent&)
    {
        m_Data->m_Items[m_Num].m_String=GetValue();
        for (int j=0; j<(int)m_Alts.size(); j++)
        {
            m_Alts[j]->m_Items[m_Num].m_String=GetValue();
        }

        m_Data->m_Instance->SetChanged(true);
    }
    DECLARE_EVENT_TABLE()
};

#endif

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
#ifndef IW_ASD_ITX_FILE_H
#define IW_ASD_ITX_FILE_H

class CIwASDFileReader;
class CIwASDITXClassDef;
class CIwASDITXMemberDef;
class CIwASDITXInstance;

//--------------------------------------------------------------------------------
class CIwASDFileReader
{
protected:
    struct Pos
    {
        wxTextFile*           m_File;
        int                   m_Line;
        int                   m_Offset;

        Pos() : m_File(NULL), m_Line(0),m_Offset(0) { }
        Pos(const Pos& pos) : m_File(pos.m_File),m_Line(pos.m_Line),m_Offset(pos.m_Offset) { }
        Pos(const wxString& fileName) : m_Line(-1),m_Offset(0) { m_File=new wxTextFile(fileName); }
    };
protected:
    std::vector<CIwASDITXInstance*> m_ObjectStack;
    CIwASDITXInstance* m_NextObject;

    std::vector<Pos> m_FileStack;
    Pos m_CurrFile;

    std::vector<wxString> m_Args;
public:
    CIwASDITXInstance* m_CurrObject;
public:
    CIwASDFileReader(CIwASDITXClassDef* context);
    ~CIwASDFileReader();

    void SetNextObject(CIwASDITXInstance* obj) { m_NextObject=obj; }
    void PushContext();
    void PopContext();
    void PushFile(const wxString& fileName);
    bool PopFile();

    bool FetchNextLine();
    int NumLineArgs() { return m_Args.size()-m_CurrFile.m_Offset; }
    wxString LineArg(int num) { return (num>=NumLineArgs()) ? L"" : m_Args[num+m_CurrFile.m_Offset]; }
    bool Advance(int num=1) { m_CurrFile.m_Offset+=num; return NumLineArgs()>0; }
};

//--------------------------------------------------------------------------------
enum EIwASDITXMemberType
{
    MEMBERTYPE_STRING=MODULE_TAG(FILE_MODULE_ID,0),
    MEMBERTYPE_RES
};

//--------------------------------------------------------------------------------
class CIwASDITXField        //concrete instance of CIwASDITXMemberDef
{
public:
    CIwASDITXMemberDef* m_Def;
public:
    CIwASDITXField(CIwASDITXMemberDef* def) : m_Def(def) {}
    virtual wxString ToString()=0;
};

//--------------------------------------------------------------------------------
class CIwASDITXMemberDef    //abstract definition
{
public:
    unsigned int m_Type;
    wxString m_Name;
    CIwASDITXClassDef* m_Class;
public:
    CIwASDITXMemberDef(CIwASDITXClassDef* klass,const wxString& name,unsigned int type) : m_Type(type),m_Name(name),m_Class(klass) {}
    //for each line it first finds a member where m_Name==line[0]
    //if that fails it next finds a member where IsMatch is true else it removes the line
    virtual bool IsMatch(CIwASDFileReader& reader) { return false; }
    virtual CIwASDITXField* Make(CIwASDFileReader& reader)=0;
};

//--------------------------------------------------------------------------------
class CIwASDITXStringField : public CIwASDITXField   //concrete instance of CIwASDITXMemberStringDef, holds a string param
{
public:
    wxString m_Data;
public:
    CIwASDITXStringField(CIwASDITXMemberDef* def,const wxString& data) : CIwASDITXField(def) {}
    virtual wxString ToString() { return wxString::Format(L"%s \"%s\"",m_Def->m_Name.c_str(),m_Data.c_str()); }
};

//--------------------------------------------------------------------------------
class CIwASDITXMemberStringDef : public CIwASDITXMemberDef      //abstract definition
{
public:
    CIwASDITXMemberStringDef(CIwASDITXClassDef* klass,const wxString& name) : CIwASDITXMemberDef(klass,name,MEMBERTYPE_STRING) {}
    virtual CIwASDITXField* Make(CIwASDFileReader& reader) { return (reader.NumLineArgs()>1) ? new CIwASDITXStringField(this,reader.LineArg(1)) : NULL; }
};

//--------------------------------------------------------------------------------
class CIwASDITXResField : public CIwASDITXField   //concrete instance of CIwASDITXMemberStringDef, holds a string param
{
public:
    wxString m_Data;
public:
    CIwASDITXResField(CIwASDITXMemberDef* def,const wxString& data) : CIwASDITXField(def),m_Data(data) {}
    virtual wxString ToString() { return wxString::Format(L"\"%s\"",m_Def->m_Name.c_str(),m_Data.c_str()); }
};

//--------------------------------------------------------------------------------
class CIwASDITXMemberResDef : public CIwASDITXMemberDef     //abstract definition
{
public:
    CIwASDITXMemberResDef(CIwASDITXClassDef* klass) : CIwASDITXMemberDef(klass,L"",MEMBERTYPE_RES) {}
    virtual bool IsMatch(CIwASDFileReader& reader) { return true; }
    virtual CIwASDITXField* Make(CIwASDFileReader& reader) { return (reader.NumLineArgs()>0) ? new CIwASDITXResField(this,reader.LineArg(0)) : NULL; }
};

//--------------------------------------------------------------------------------
class CIwASDITXInstance     //concrete instance of CIwASDITXClassDef
{
public:
    CIwASDITXClassDef* m_Def;
    std::vector<CIwASDITXField*> m_Fields;
public:
    CIwASDITXInstance(CIwASDITXClassDef* def) : m_Def(def) {}
    ~CIwASDITXInstance() { for (int i=0; i<(int)m_Fields.size(); i++) {delete m_Fields[i]; }}
};

//--------------------------------------------------------------------------------
class CIwASDITXClassDef     //abstract definition
{
public:
    CIwASDITXClassDef* m_Parent;
    wxString m_Name;
    std::vector<CIwASDITXMemberDef*> m_Members;
public:
    CIwASDITXClassDef(const wxString& name,CIwASDITXClassDef* parent=NULL) : m_Parent(parent),m_Name(name) { }
    ~CIwASDITXClassDef();
    virtual CIwASDITXInstance* Make(CIwASDFileReader& reader);
};

//--------------------------------------------------------------------------------
class CIwASDITX : public CIwASDFile
{
public:
    CIwASDITXInstance* m_Data;
public:
    CIwASDITX(unsigned int type=FILETYPE_ITX) : CIwASDFile(type),m_Data(NULL) {  }
    virtual void Load(int Stage,int path=0) {}
    virtual bool HasType(unsigned int type) { return (type==FILETYPE_ITX) ? true : CIwASDFile::HasType(type); }

    void LoadFile(bool force);
};

//--------------------------------------------------------------------------------
class CIwASDGroup : public CIwASDITX
{
public:
    CIwASDGroup(unsigned int type=FILETYPE_GROUP) : CIwASDITX(type) {}
    virtual void Load(int Stage,int path=0);

    virtual bool HasType(unsigned int type) { return (type==FILETYPE_GROUP) ? true : CIwASDITX::HasType(type); }
    virtual bool ExpandList(std::vector<CIwASDData*>& dataList,int toType);
};


#endif // !IW_ASD_ITX_FILE_H

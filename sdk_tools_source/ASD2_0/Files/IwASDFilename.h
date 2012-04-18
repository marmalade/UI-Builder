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
#ifndef IW_ASD_FILENAME_H
#define IW_ASD_FILENAME_H

class CIwASDFilenameRoots;
class CIwASDFile;

//--------------------------------------------------------------------------------
// CIwASDFilename
//--------------------------------------------------------------------------------
class CIwASDFilename
{
public:
    enum EIwFilenameFlags
    {
        ROOT_NOT=0,
        ROOT_MAIN=1,
        ROOT_AUX1=2,
        ROOT_AUX2=4,
        ROOT_AUX3=8,
        ROOT_AUX4=16,
        ROOT_AUX5=32,
        ROOT_AUX6=64,
        ROOT_NULL=128,

        TYPE_DIR=0,
        TYPE_FILENAME,
        TYPE_SECTION,
        TYPE_ALL=0xff,
    };
private:
    friend class CIwASDFilenameRoots;
protected:
    unsigned int m_RefCount;
    wxString m_Name;
public:
    CIwASDFilename* m_Parent;
    std::vector<CIwASDFilename*> m_Children;

    CIwASDFile* m_File;
    std::vector<CIwASDFile*> m_OtherFiles;

    unsigned char m_Root;       //type of root ROOT_NOT (0) is not a root,ROOT_MAIN (1) is main root, ROOT_AUX1-6 (2,4,...) are aux roots
                                //ROOT_NULL (128) is the root of files not yet created
    unsigned char m_Type;       //TYPE_DIR (0) is a directory, TYPE_FILENAME (1) is a file, TYPE_SECTION (2) is a sub section in a file
public:                         // so we go: dir(main_root)/dir/dir/file:section:section
    CIwASDFilename(const wxString& name,CIwASDFilename* parent) : m_RefCount(0),m_Name(name),m_Parent(parent),m_File(NULL),m_Root(ROOT_NOT),m_Type(TYPE_DIR) { }
    void IncRef() { m_RefCount++; }
    void DecRef();

    //make/find a new filename all return a new reference to the file (or NULL)
    static CIwASDFilename* GetDir(const wxString& name,CIwASDFilename* parent=NULL,unsigned char root=ROOT_NOT);
    static CIwASDFilename* GetFile(const wxString& name,CIwASDFilename* parent=NULL);
    static CIwASDFilename* GetSection(const wxString& name,CIwASDFilename* parent);

    CIwASDFilename* GetDir(const wxString& name,unsigned char root=ROOT_NOT) { return GetDir(name,this,root); }
    CIwASDFilename* GetFile(const wxString& name) { return GetFile(name,this); }
    CIwASDFilename* GetSection(const wxString& name) { return GetSection(name,this); }

    static CIwASDFilename* Find(const wxString& name,CIwASDFilename* parent=NULL);
    CIwASDFilename* Find(const wxString& name) { return Find(name,this); }

    static bool IsAbsPath(const wxString& path);

    //fills in text with whole path or up to root using sep[0] for between dirs and sep[1] for between sections
    //puts a final sep[0] of dir's
    wxString GetString(unsigned char root=ROOT_NOT,const wxString& sep=L"/:");
    wxString GetString(CIwASDFilename* root,const wxString& sep=L"/:");

    CIwASDFilename* GetFile();                              //get file from a sub section
    CIwASDFilename* GetRoot(unsigned char root=ROOT_MAIN);  //get a particular root

    wxString c_str(unsigned char root=ROOT_NOT);
    wxString c_str(bool mainRoot);
    unsigned int size() { return c_str().size(); }

    wxString getDir(bool noFinalSlash=true);
    wxString getExt();
    wxString getName(bool includeExt=true);

    CIwASDFile* GetShortestFile();
private:
    CIwASDFilename* Add(std::vector<wxString>& argv,unsigned char Type,bool DoAdd);
    ~CIwASDFilename();
};

//--------------------------------------------------------------------------------
class CIwASDFilenameRoots
{
public:
    std::vector<CIwASDFilename*> m_Roots;

    CIwASDFilename* Get(const wxString& fileName,CIwASDFilename* parent,unsigned char Type,bool DoAdd);

    void DebugPrint(int indent=0,CIwASDFilename* file=NULL);
};

#endif // !IW_ASD_FILENAME_H

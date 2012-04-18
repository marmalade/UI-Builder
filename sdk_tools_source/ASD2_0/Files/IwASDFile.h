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
#ifndef IW_ASD_FILE_H
#define IW_ASD_FILE_H

class CIwASDDir;
class CIwASDITXClassDef;

#define FILE_MODULE_ID 1
#define FILE_EXTRA_DATA_ID 0

#define FILE_TITLETYPE_NUMFILEDIRS 1

//--------------------------------------------------------------------------------
enum EIwASDFileType
{
    FILETYPE_FILE=MODULE_TAG(FILE_MODULE_ID,0),
    FILETYPE_DIR,
    FILETYPE_ITX,
    FILETYPE_GROUP,
    FILETYPE_BIN,
    FILETYPE_TEXTURE,

    FILEDATATYPE_SOURCE,
    FILEDATATYPE_ATTR,
};

//--------------------------------------------------------------------------------
class CIwASDFileData : public CIwASDData
{
public:
    CIwASDFile* m_File;

    virtual void Load(bool force=false)=0;
    virtual void SetChanged(bool changed);
    virtual void SetState(int value,int mask);

    CIwASDFileData(unsigned int type) : CIwASDData(type) {}
    virtual ~CIwASDFileData();
    virtual wxString GetName();
    virtual void CheckSave(bool force);
    virtual void Save() {}
};

struct CIwFindData
{
    bool        isDir;
    bool        isReadOnly; //dwFileAttributes
    wxDateTime  time;   //ftLastWriteTime;
    wxULongLong size;   //nFileSizeHigh/nFileSizeLow
    wxString    name;   //cFileName

    void        SetupFileAttr(const wxString& name);
};

//--------------------------------------------------------------------------------
class CIwASDFile : public CIwASDData
{
public:
    std::vector<CIwASDFilename*> m_Paths;
    CIwFindData m_File;
    int m_Generation;

    CIwASDDir* m_Parent;
    std::map<int,CIwASDFileData*> m_DataBlocks;
public:
    CIwASDFile(unsigned int type=FILETYPE_FILE);
    ~CIwASDFile() { Clear(); }

    //return -1 for failed or the index of the path added
    int AddPath(const wxString& Name,CIwASDFilename* root=NULL);
    int AddPath(const wxString& Name,CIwASDFile* root);
    int AddPath(wxString Name,std::vector<CIwASDFilename*>& roots);
    int CreatePath(const wxString& name,CIwASDFilename* path=NULL);
    //returns true if deleted itself
    bool RemovePath(CIwASDFilename* parent);
    bool SetPath(const wxString& name);
    virtual wxString GetName() { return m_Paths.empty() ? L"" : m_Paths[0]->c_str(true); }

    void AddFileData(int type,CIwASDFileData* data);
    CIwASDFileData* LoadFileData(int type,bool force=false);
    void ResetState();

    bool StartSave(bool force); //deal with Source control etc

    //override to load specific data, Stage 0,1,2 are succesive passes,Stage -1 is load now
    virtual void Load(int Stage,int path=0) {}
    virtual void Clear();
    virtual void ClearContents();

    virtual bool HasType(unsigned int type) { return (type==FILETYPE_FILE) ? true : CIwASDData::HasType(type); }
    virtual void GetOptions(std::map<wxString,wxString>& opts,wxColour& col);
    virtual void ViewSource(int value);

    static wxString FormatMemSize(wxULongLong v);
};

//--------------------------------------------------------------------------------
class CIwASDDir : public CIwASDFile
{
public:
    std::vector<CIwASDFile*> m_Files;
    std::vector<CIwASDDir*> m_Dirs;

    CIwASDDir();
    ~CIwASDDir() { Clear(); }
    virtual void Clear();
    virtual void ClearContents();

    virtual void Load(int Stage,int path=0);
    void LoadContents(int Stage,int path);

    void RemoveFile(CIwASDFile* file);
    void Update();  //remove deleted files

    CIwASDFile* FindDirFile(wxString name);

    virtual bool HasType(unsigned int type) { return (type==FILETYPE_DIR) ? true : CIwASDFile::HasType(type); }
    virtual void GetOptions(std::map<wxString,wxString>& opts,wxColour& col);

    virtual bool ExpandList(std::vector<CIwASDData*>& dataList,int toType);
};

//--------------------------------------------------------------------------------
class CIwASDBinFile : public CIwASDFile
{
public:
    CIwASDBinFile() : CIwASDFile(FILETYPE_BIN) {}
    virtual void Load(int Stage,int path=0);

    virtual bool HasType(unsigned int type) { return (type==FILETYPE_BIN) ? true : CIwASDFile::HasType(type); }

    virtual bool ExpandList(std::vector<CIwASDData*>& dataList,int toType);
};

//------------------------------------------------------------------------------
class CIwASDSubMenu : public CIwAction
{
protected:
    CIwASDFile* GetFile(CIwASDData* data);
public:
    CIwASDSubMenu(CIwModule* module) : CIwAction(module) {}
    virtual void Action(int value=1) {}

    virtual void OverrideMenu(wxMenuItem* Item,int& id)
    {
        CIwASDFile* data=GetFile((CIwASDData*)m_Context);
        if (data==NULL) return;

        Item->SetSubMenu(CIwTheFrame->MakeMenu(CIwTheFrame->m_RightMenu,L"other",data));
    }
    bool DoCreate()
    {
        CIwASDFile* data=GetFile((CIwASDData*)m_Context);
        return (data!=NULL);
    }
};

//--------------------------------------------------------------------------------
class CIwFileModule : public CIwModule
{
    struct AttrTypeElem
    {
        wxString          m_Ext;
        CIwAttrExportType m_Type;
        wxString          m_RootName;

        AttrTypeElem(const wxString& ext,CIwAttrExportType type,const wxString& rootName) : m_Ext(ext),m_Type(type),m_RootName(rootName) {}
    };
    std::vector<AttrTypeElem> m_AttrTypes;
    wxTextFile outLog;
public:
    CIwASDDir* m_Root;
    CIwASDFilenameRoots m_FileRoots;
    std::vector<wxString> m_FileNameRejects;
    std::map<unsigned int,CIwASDITXClassDef*> m_ClassDefs;
    int m_Generation;
    CIwAttrDescMgr m_GameMgrMeta;
    CIwWinSourceControl m_SourceControl;
    bool m_LogSaves;
public:
    CIwFileModule();
    virtual ~CIwFileModule();

    void Update();
    bool FileNameReject(const wxString& name);
    void DebugPrint(int indent=0,CIwASDFile* file=NULL);

    bool CheckAttrFileType(const wxString& fileName);
    CIwAttrInstance* GetAttrInstance(const wxString& fileName,CIwAttrFileGroup* group=NULL);
    void AddToSaveLog(const wxString& fileName);

    //if type==TYPE_DIR and info=="root" store a pointer to this dir, it is our root dir
    virtual CIwASDData* MakeDataObject(unsigned int type,const wxString& info=L"");
    virtual CIwLayoutElement* MakeElement(const wxString& type);
    virtual void GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<wxString>& names,int size);
protected:
    virtual void OnInit();
};

#define CIwTheFileModule ((CIwFileModule*)(CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]))
#define CIwTheRoot (((CIwFileModule*)(CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]))->m_Root)
#define CIwTheFileRoots (((CIwFileModule*)(CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]))->m_FileRoots)
#define CIwTheFileMetaMgr (((CIwFileModule*)(CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]))->m_GameMgrMeta)
#define CIwTheFileSrcCtrl (((CIwFileModule*)(CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]))->m_SourceControl)


#endif // !IW_ASD_FILE_H

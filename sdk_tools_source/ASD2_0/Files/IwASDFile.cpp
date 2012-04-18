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
#include "IwASDFileHeader.h"

//--------------------------------------------------------------------------------
CIwASDData* CIwFileModule::MakeDataObject(unsigned int type,const wxString& info)
{
    switch (type)
    {
    case FILETYPE_GROUP:
        return new CIwASDGroup();
    case FILETYPE_BIN:
        return new CIwASDBinFile();
    case FILETYPE_TEXTURE:
        return new CIwASDTextureFile();
    case FILETYPE_FILE:
        if (!info.empty())
        {
            if (info.IsSameAs(L"group",false))
                return new CIwASDGroup();

            if (info.IsSameAs(L"bin",false))
                return new CIwASDBinFile();

            if (info.IsSameAs(L"tga",false) || info.IsSameAs(L"bmp",false) || info.IsSameAs(L"png",false) || info.IsSameAs(L"gif",false))
                return new CIwASDTextureFile();  //if(info.IsSameAs(L"itx",false))

            //	return new CIwDerbhBinFile();
        }

        return new CIwASDFile();
    case FILETYPE_DIR:
        if (!info.empty() && info.IsSameAs(L"root",false))
        {
            if (m_Root!=NULL)
                delete m_Root;

            m_Root=new CIwASDDir();
            return m_Root;
        }
        else
            return new CIwASDDir();
    }

    return NULL;
}

//--------------------------------------------------------------------------------
bool CIwFileModule::CheckAttrFileType(const wxString& fileName)
{
    for (int i=0; i<(int)m_AttrTypes.size(); i++)
    {
        if (fileName.EndsWith(m_AttrTypes[i].m_Ext))
            return true;
    }
    return false;
}

//--------------------------------------------------------------------------------
CIwAttrInstance* CIwFileModule::GetAttrInstance(const wxString& fileName,CIwAttrFileGroup* group)
{
    /*	for(int i=0;i<(int)m_AttrTypes.size();i++)
            if(fileName.EndsWith(m_AttrTypes[i].m_Ext))
            {
                if(m_AttrTypes[i].m_RootName.empty())
                    return m_GameMgrMeta.GetFromFile(fileName,m_AttrTypes[i].m_Type,group);
                else
                    return m_GameMgrMeta.GetFromFile(fileName,m_AttrTypes[i].m_Type,m_AttrTypes[i].m_RootName,group);
            }

        return NULL;
     */
    return m_GameMgrMeta.GetFromFile(fileName,group);
}

//--------------------------------------------------------------------------------
void CIwFileModule::AddToSaveLog(const wxString& fileName)
{
    if (!m_LogSaves) return;

    int i;
    for (i=0; i<(int)outLog.GetLineCount(); i++)
    {
        if (outLog[i].IsSameAs(fileName,false))
            break;
    }
    if (i==(int)outLog.GetLineCount())
    {
        outLog.AddLine(fileName);
        outLog.Write();
    }
}

//--------------------------------------------------------------------------------
CIwFileModule::CIwFileModule() : CIwModule(L"file_"),outLog(wxStandardPaths::Get().GetUserDataDir()+L"/toexporter.log"),m_Root(NULL),m_Generation(0),m_GameMgrMeta(false),m_LogSaves(false)
{
    if (!wxDirExists(wxStandardPaths::Get().GetUserDataDir()))
        wxFileName::Mkdir(wxStandardPaths::Get().GetUserDataDir());

    m_FileNameRejects.push_back(L".*");
    m_FileNameRejects.push_back(L"_viewertemp*");

    m_AttrTypes.push_back(AttrTypeElem(L"mtl",EXPORTTYPE_MATERIAL,L"materials"));
    m_AttrTypes.push_back(AttrTypeElem(L"gfx",EXPORTTYPE_GFX,L"effects"));
    m_AttrTypes.push_back(AttrTypeElem(L"group",EXPORTTYPE_GROUP,L"groups"));
    m_AttrTypes.push_back(AttrTypeElem(L"ui",EXPORTTYPE_GUI,L"ui"));
    outLog.Write();
}

//--------------------------------------------------------------------------------
void CIwFileModule::OnInit()
{
    CIwTheApp->m_ExtraData[FILE_EXTRA_DATA_ID]=this;

    m_App->AddAction(L"file_info",new CIwDataAction<CIwASDFile>(this,FILETYPE_FILE));
    m_App->AddAction(L"file_dir_info",new CIwDataAction<CIwASDDir>(this,FILETYPE_DIR));
    m_App->AddAction(L"file_itx_info",new CIwDataAction<CIwASDITX>(this,FILETYPE_ITX));
    m_App->AddAction(L"file_group_info",new CIwDataAction<CIwASDGroup>(this,FILETYPE_GROUP));
    m_App->AddAction(L"file_bin_info",new CIwDataAction<CIwASDBinFile>(this,FILETYPE_BIN));
    m_App->AddAction(L"file_text_info",new CIwDataAction<CIwASDTextureFile>(this,FILETYPE_TEXTURE));
    m_App->AddAction(L"file_other",new CIwASDSubMenu(this));
    m_App->AddAction(L"file_source",new CIwDataAction<CIwASDFile>(this,FILETYPE_FILE,&CIwASDFile::ViewSource));

    CIwASDITXClassDef* klass=new CIwASDITXClassDef(L"CIwResGroup");
    klass->m_Members.push_back(new CIwASDITXMemberStringDef(klass,L"name"));
    klass->m_Members.push_back(new CIwASDITXMemberResDef(klass));   //must be last

    m_ClassDefs[FILETYPE_GROUP]=klass;
}

//--------------------------------------------------------------------------------
void CIwFileModule::DebugPrint(int indent,CIwASDFile* file)
{
#ifdef I3D_OS_WINDOWS
    int i;

    if (file==NULL)
    {
        if (m_Root==NULL) return;

        DebugPrint(indent+1,m_Root);
    }
    else
    {
        wxString s=wxString::Format(L"%s %s %x (",wxString(indent,L' '),file->m_File.name.c_str(),file->m_Type);
        for (i=0; i<(int)file->m_Paths.size(); i++)
        {
            if (i!=0) s+=L",";

            if (file->m_Paths[i]->m_Parent!=NULL)
                s+=file->m_Paths[i]->m_Parent->getName();
        }
        s+=L")\n";
        OutputDebugString(s);

        if (file->HasType(FILETYPE_DIR))
        {
            CIwASDDir* dir=(CIwASDDir*)file;
            for (i=0; i<(int)dir->m_Dirs.size(); i++)
            {
                DebugPrint(indent+1,dir->m_Dirs[i]);
            }
            for (i=0; i<(int)dir->m_Files.size(); i++)
            {
                DebugPrint(indent+1,dir->m_Files[i]);
            }
        }
    }

#endif
}

//--------------------------------------------------------------------------------
void CIwFileModule::Update()
{
    m_Generation++;
    for (int i=0; i<(int)m_Root->m_Paths.size(); i++)
    {
        m_Root->Load(-1,i);
    }
    //TODO split into stages
    m_Root->Update();
}

//--------------------------------------------------------------------------------
bool CIwFileModule::FileNameReject(const wxString& name)
{
    for (int i=0; i<(int)m_FileNameRejects.size(); i++)
    {
        if (name.Matches(m_FileNameRejects[i]))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
CIwFileModule::~CIwFileModule()
{
    if (m_Root!=NULL)
        delete m_Root;

    std::map<unsigned int,CIwASDITXClassDef*>::iterator it;
    for (it=m_ClassDefs.begin(); it!=m_ClassDefs.end(); it++)
    {
        delete it->second;
    }
}

//------------------------------------------------------------------------------
void CIwFileModule::GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<wxString>& names,int size)
{
    wxImage image;
    wxSize s(size,size);
    wxPoint p;

    if (image.LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}Grey/directory.png")))
    {
        p.x=(size-image.GetWidth())/2;
        p.y=(size-image.GetHeight())/2;
        list->Add(image.Size(s,p),wxColour(L"WHITE"));
        types.push_back(FILETYPE_DIR);
        names.push_back(L"Directory");
    }

    if (image.LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}Grey/otherfile.png")))
    {
        p.x=(size-image.GetWidth())/2;
        p.y=(size-image.GetHeight())/2;
        list->Add(image.Size(s,p),wxColour(L"WHITE"));
        types.push_back(FILETYPE_FILE);
        names.push_back(L"generic file");
    }

    if (image.LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}Grey/other.png")))
    {
        p.x=(size-image.GetWidth())/2;
        p.y=(size-image.GetHeight())/2;
        list->Add(image.Size(s,p),wxColour(L"WHITE"));
        types.push_back(FILETYPE_BIN);
        names.push_back(L"binary of group file");
    }

    if (image.LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}Grey/texture.png")))
    {
        p.x=(size-image.GetWidth())/2;
        p.y=(size-image.GetHeight())/2;
        list->Add(image.Size(s,p),wxColour(L"WHITE"));
        types.push_back(FILETYPE_TEXTURE);
        names.push_back(L"texture file");
    }

    if (image.LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}Grey/entity.png")))
    {
        p.x=(size-image.GetWidth())/2;
        p.y=(size-image.GetHeight())/2;
        list->Add(image.Size(s,p),wxColour(L"WHITE"));
        types.push_back(FILETYPE_GROUP);
        names.push_back(L"group file");
    }
}

//------------------------------------------------------------------------------
CIwLayoutElement* CIwFileModule::MakeElement(const wxString& type)
{
    if (type.IsSameAs(L"project",false))
        return new CIwASDTreeLayoutProject;

    if (type.IsSameAs(L"source",false))
        return new CIwASDSourceLayout;

    if (type.IsSameAs(L"attr",false))
        return new CIwASDAttrLayout;

    if (type.IsSameAs(L"texture",false))
        return new CIwASDTextureLayout;

    return NULL;
}

//--------------------------------------------------------------------------------
CIwASDFileData::~CIwASDFileData()
{
    Clear();
}

//--------------------------------------------------------------------------------
void CIwASDFileData::SetChanged(bool changed)
{
    CIwASDData::SetChanged(changed);
    m_File->ResetState();
}

//--------------------------------------------------------------------------------
wxString CIwASDFileData::GetName()
{
    return m_File->GetName();
}

//--------------------------------------------------------------------------------
void CIwASDFileData::CheckSave(bool force)
{
    if (!m_File->StartSave(force)) return;

    Save();
    SetState(0,STATE_CHANGED);
}

//--------------------------------------------------------------------------------
void CIwASDFileData::SetState(int value,int mask)
{
    if (mask&STATE_READWRITE)
        if (m_File->GetState(STATE_READWRITE))
            value&=~STATE_READWRITE;

    CIwASDData::SetState(value,mask);
    m_File->ResetState();
}

//--------------------------------------------------------------------------------
void CIwASDFile::ViewSource(int value)
{
    CIwLayoutData* elem=CIwTheFrame->FindDataContainer(FILETYPE_FILE,L"attr",wxString::Format(L"File: %s",m_File.name.c_str()));
    if (elem==NULL) return;

    elem->SetData(this);
}

//--------------------------------------------------------------------------------
bool CIwASDFile::StartSave(bool force)
{
    if (force || m_Paths.empty())
    {
        wxFileName name;
        if (m_Paths.empty())
        {
            if (CIwTheRoot!=NULL && !CIwTheRoot->m_Paths.empty())
                name.Assign(CIwTheRoot->m_Paths[0]->c_str(),L"");
        }
        else
            name.Assign(m_Paths[0]->c_str());

        wxFileDialog Dialog(CIwTheFrame,L"Save File...",name.GetPath(),name.GetFullName(),L"All Files (*.*)|*.*",
                            wxSAVE|wxOVERWRITE_PROMPT);

        if (Dialog.ShowModal()!=wxID_OK)
            return false;

        SetPath(Dialog.GetPath());
    }

    //TODO SC and RO controls

    return true;
}

//--------------------------------------------------------------------------------
CIwASDFile::CIwASDFile(unsigned int type) : CIwASDData(type),m_Generation(0),m_Parent(NULL)
{
}

//--------------------------------------------------------------------------------
void CIwASDFile::ResetState()
{
    int state=0;
    for (std::map<int,CIwASDFileData*>::iterator it=m_DataBlocks.begin(); it!=m_DataBlocks.end(); ++it)
    {
        state|=it->second->GetState();
    }
}

//--------------------------------------------------------------------------------
void CIwASDFile::AddFileData(int type,CIwASDFileData* data)
{
    if (m_DataBlocks.find(type)!=m_DataBlocks.end())
        delete m_DataBlocks[type];

    m_DataBlocks[type]=data;
    data->m_File=this;
}

//--------------------------------------------------------------------------------
CIwASDFileData* CIwASDFile::LoadFileData(int type,bool force)
{
    if (m_DataBlocks.find(type)==m_DataBlocks.end())
        return NULL;

    m_DataBlocks[type]->Load(force);
    return m_DataBlocks[type];
}

//--------------------------------------------------------------------------------
void CIwASDFile::ClearContents()
{
    for (CIwASDDir* parent=m_Parent; parent!=NULL; parent=parent->m_Parent)
    {
        parent->RefreshData(this,false,true);
    }

    for (int i=0; i<(int)m_Paths.size(); i++)
    {
        if (m_Paths[i]->m_File==this)
            m_Paths[i]->m_File=NULL;
        else
            for (int j=0; j<(int)m_Paths[i]->m_OtherFiles.size(); j++)
            {
                m_Paths[i]->m_OtherFiles.erase(m_Paths[i]->m_OtherFiles.begin()+j);
                break;
            }

        m_Paths[i]->DecRef();
    }
    m_Paths.clear();

    for (std::map<int,CIwASDFileData*>::iterator it=m_DataBlocks.begin(); it!=m_DataBlocks.end(); ++it)
    {
        delete it->second;
    }
    m_DataBlocks.clear();
}

//--------------------------------------------------------------------------------
void CIwASDFile::Clear()
{
    ClearContents();

    CIwASDData::Clear();
}

//--------------------------------------------------------------------------------
void CIwFindData::SetupFileAttr(const wxString& fname)
{
    wxFileName fn(fname);
    size=fn.GetSize();
    name=fn.GetName();
    fn.GetTimes(&time,NULL,NULL);

    isDir=fn.IsDir();
    if (isDir)
        isReadOnly=fn.IsDirReadable();
    else
        isReadOnly=fn.IsFileReadable();
}

//--------------------------------------------------------------------------------
bool CIwASDFile::SetPath(const wxString& name)
{
    int i;
    m_File.SetupFileAttr(name);

    std::vector<CIwASDFilename*> paths;
    for (i=1; i<(int)m_Paths.size(); i++)
    {
        paths.push_back(m_Paths[i]);
    }

    if (m_Paths.size()>0)
        m_Paths[0]->DecRef();

    m_Paths.clear();

    int res=AddPath(name);
    if (res==-1)
        res=CreatePath(name);

    for (i=0; i<(int)paths.size(); i++)
    {
        m_Paths.push_back(paths[i]);
    }

    return res!=-1;
}

//--------------------------------------------------------------------------------
int CIwASDFile::AddPath(const wxString& name,CIwASDFile* root)
{
    return AddPath(name,root->m_Paths);
}

//--------------------------------------------------------------------------------
int CIwASDFile::AddPath(const wxString& name,CIwASDFilename* root)
{
    std::vector<CIwASDFilename*> roots;
    if (root!=NULL)
        roots.push_back(root);

    return AddPath(name,roots);
}

//--------------------------------------------------------------------------------
int CIwASDFile::AddPath(wxString name,std::vector<CIwASDFilename*>& roots)
{
    CIwFindData FileData;
    CIwASDFilename* path=NULL;
    bool found=false;
    wxString abs;

    if (name.EndsWith(L"\\") || name.EndsWith(L"/"))
        name.RemoveLast();

    if (CIwASDFilename::IsAbsPath(name))
    {
        abs=name;
        found=wxFileExists(name);
    }
    else
    {
        for (int i=0; i<(int)roots.size(); i++)
        {
            wxString Line=wxString::Format(L"%s%s",roots[i]->c_str().c_str(),name.c_str());
            Line.Replace(L"\\",L"/");
            if (Line.EndsWith(L"/"))
                Line.RemoveLast();

            found=wxFileExists(Line);

            if (found)
            {
                path=roots[i];
                break;
            }

            abs=Line;
        }
    }

    if (!found)
        return -1;

    if (m_Paths.size()==0)
        m_File.SetupFileAttr(abs);

    return CreatePath(name,path);
}

//--------------------------------------------------------------------------------
int CIwASDFile::CreatePath(const wxString& name,CIwASDFilename* path)
{
    if (m_Type==FILETYPE_DIR)
        path=CIwASDFilename::GetDir(name,path);
    else
        path=CIwASDFilename::GetFile(name,path);

    m_Paths.push_back(path);
    if (path->m_File==NULL)
        path->m_File=this;
    else
        path->m_OtherFiles.push_back(this);

    return m_Paths.size()-1;
}


//--------------------------------------------------------------------------------
CIwASDDir::CIwASDDir() : CIwASDFile(FILETYPE_DIR)
{
    m_Connections[FILETYPE_FILE]=new CIwASDDataConxToList((std::vector<CIwASDData*>&)m_Files,false);
    m_Connections[FILETYPE_DIR]=new CIwASDDataConxToList((std::vector<CIwASDData*>&)m_Dirs,false);
}

//--------------------------------------------------------------------------------
wxString CIwASDFile::FormatMemSize(wxULongLong v)
{
    wxString s;
    const char* suffix="tgmk";
    int len=(int)strlen(suffix);

    for (int i=0; i<len; i++)
    {
        wxULongLong amount=(wxULongLong )pow(1024.0,len-i);
        wxULongLong v2=(v/amount);

        if (v2>0)
            s+=wxString::Format(L"%d%c ",v2.GetLo(),(wxChar)suffix[i]);

        v-=v2*amount;
    }
    s+=wxString::Format(L"%db",*((wxLongLong_t*)&v));

    return s;
}

//--------------------------------------------------------------------------------
void CIwASDFile::GetOptions(std::map<wxString,wxString>& opts,wxColour& col)
{
    opts[L"file_name"]=m_File.name;
    wxString s;
    for (int i=0; i<(int)m_Paths.size(); i++)
    {
        if (i!=0) s+=L";";

        s+=m_Paths[i]->GetRoot()->getName();
    }
    opts[L"file_roots"]=s;
    opts[L"file_size"]=FormatMemSize(m_File.size);

    opts[L"file_path"]=L"???";
    if (m_Paths.size()>0 && m_Paths[0]->m_Parent!=NULL)
        opts[L"file_path"]=m_Paths[0]->m_Parent->c_str(true);
}

//--------------------------------------------------------------------------------
bool CIwASDFile::RemovePath(CIwASDFilename* parent)
{
    int i;
    for (i=0; i<(int)m_Paths.size(); i++)
    {
        if (m_Paths[i]->m_Parent==parent)
            break;
    }
    if (i==(int)m_Paths.size()) return false;

    m_Paths[i]->DecRef();
    m_Paths.erase(m_Paths.begin()+i);
    if (m_Paths.size()>0)
        return false;

    delete this;
    return true;
}

//--------------------------------------------------------------------------------
void CIwASDDir::GetOptions(std::map<wxString,wxString>& opts,wxColour& col)
{
    if (m_Paths.empty())
        col=wxColour(L"GREY");

    opts[L"dir_num"]=wxString::Format(L"(%d)",m_Files.size()+m_Dirs.size());
    CIwASDFile::GetOptions(opts,col);
}

//--------------------------------------------------------------------------------
void CIwASDDir::LoadContents(int Stage,int path)
{
    CIwFindData FileData;
    //CIwASDFilename* Path=NULL;
    int i;

    if (path>=(int)m_Paths.size()) return;

    wxString Line=m_Paths[path]->c_str();
    wxDir dir(Line);
    bool found=dir.GetFirst(&Line);
    while (found)
    {
        FileData.SetupFileAttr(Line);
        if (!CIwTheFileModule->FileNameReject(FileData.name))
        {
            CIwASDFile* file=NULL;

            if (FileData.isDir)
            {
                for (i=0; i<(int)m_Dirs.size(); i++)
                {
                    if (m_Dirs[i]->m_File.name==FileData.name)
                    {
                        file=m_Dirs[i];
                        break;
                    }
                }
            }
            else
            {
                for (i=0; i<(int)m_Files.size(); i++)
                {
                    if (m_Files[i]->m_File.name==FileData.name)
                    {
                        file=m_Files[i];
                        break;
                    }
                }
            }

            if (file==NULL)
            {
                if (FileData.isDir)
                {
                    file=(CIwASDFile*)CIwTheApp->MakeDataObject(FILETYPE_DIR,FileData.name.AfterLast('.'));
                    m_Dirs.push_back((CIwASDDir*)file);
                }
                else
                {
                    file=(CIwASDFile*)CIwTheApp->MakeDataObject(FILETYPE_FILE,FileData.name.AfterLast('.'));
                    m_Files.push_back((CIwASDFile*)file);
                }

                file->m_Parent=this;
            }

            file->m_Generation=CIwTheFileModule->m_Generation;

            CIwASDFilename* fileName=CIwASDFilename::Find(FileData.name,m_Paths[path]);
            for (i=0; i<(int)file->m_Paths.size(); i++)
            {
                if (file->m_Paths[i]==fileName)
                    break;
            }

            int num=-1;
            if (i==(int)file->m_Paths.size())
                num=file->AddPath(FileData.name,m_Paths[path]);
            else if (FileData.isDir)
                num=i;
            else if (FileData.time!=file->m_File.time)
                num=i;

            if (num!=-1)
                file->Load(Stage,num);
        }

        found=dir.GetNext(&Line);
    }
}

//--------------------------------------------------------------------------------
void CIwASDDir::Update()
{
    int i;

    for (i=0; i<(int)m_Dirs.size(); )
    {
        if (m_Dirs[i]->m_Generation!=CIwTheFileModule->m_Generation)
        {
            delete m_Dirs[i];
            m_Dirs.erase(m_Dirs.begin()+i);
        }
        else
        {
            m_Dirs[i]->Update();
            i++;
        }
    }

    for (i=0; i<(int)m_Files.size(); i++)
    {
        if (m_Files[i]->m_Generation!=CIwTheFileModule->m_Generation)
        {
            delete m_Files[i];
            m_Files.erase(m_Files.begin()+i);
        }
        else i++;
    }
}

//--------------------------------------------------------------------------------
void CIwASDDir::Load(int Stage,int path)
{
    int i;

    if (Stage==0 || Stage==-1)
    {
        LoadContents(Stage,path);
        return;
    }

    for (i=0; i<(int)m_Files.size(); i++)
    {
        m_Files[i]->Load(Stage);
    }

    for (i=0; i<(int)m_Dirs.size(); i++)
    {
        m_Dirs[i]->Load(Stage);
    }
}

//--------------------------------------------------------------------------------
void CIwASDDir::ClearContents()
{
    int i;
    for (i=0; i<(int)m_Files.size(); i++)
    {
        delete m_Files[i];
    }
    m_Files.clear();

    for (i=0; i<(int)m_Dirs.size(); i++)
    {
        delete m_Dirs[i];
    }
    m_Dirs.clear();
}

//--------------------------------------------------------------------------------
void CIwASDDir::Clear()
{
    ClearContents();

    CIwASDFile::Clear();
}

//--------------------------------------------------------------------------------
bool CIwASDDir::ExpandList(std::vector<CIwASDData*>& dataList,int toType)
{
    int j;
    if (toType!=FILETYPE_FILE) return false;

    for (j=0; j<(int)m_Files.size(); j++)
    {
        dataList.push_back(m_Files[j]);
    }

    for (j=0; j<(int)m_Dirs.size(); j++)
    {
        dataList.push_back(m_Dirs[j]);
    }

    return true;
}


//--------------------------------------------------------------------------------
CIwASDFile* CIwASDDir::FindDirFile(wxString name)
{
    int i;
    name.Replace(L"\\",L"/");
    name.MakeLower();

    for (i=0; i<(int)m_Paths.size(); i++)
    {
        CIwASDFilename* path=CIwASDFilename::Find(name,m_Paths[i]);
        if (path!=NULL)
            return path->GetShortestFile();
    }

    for (i=0; i<(int)m_Paths.size(); i++)
    {
        wxString prefix=L"";
        if (m_Paths[i]->m_Parent!=NULL)
            prefix=m_Paths[i]->m_Parent->c_str(true)+m_Paths[i]->getName()+L"/";

        prefix.MakeLower();

        if (name.StartsWith(prefix))
        {
            CIwASDFile* file=FindDirFile(name.Mid(prefix.size()));
            if (file!=NULL)
                return file;
        }
    }

    return NULL;
}

//--------------------------------------------------------------------------------
CIwASDFile* CIwASDSubMenu::GetFile(CIwASDData* data)
{
    int type=-1;
    if (data->HasType(FILETYPE_GROUP))
        type=FILETYPE_BIN;
    else if (data->HasType(FILETYPE_BIN))
        type=FILETYPE_GROUP;
    else
        return NULL;

    if (data->m_Connections.find(type)==data->m_Connections.end())
        return NULL;

    if (data->m_Connections[type]->size()<1)
        return NULL;

    return (CIwASDFile*)(*data->m_Connections[type])[0];
}

//--------------------------------------------------------------------------------
void CIwASDGroup::Load(int Stage,int path)
{
    if (m_Connections.find(FILETYPE_BIN)==m_Connections.end())
        m_Connections[FILETYPE_BIN]=new CIwASDDataConxList(true);

    for (int i=0; i<(int)m_Parent->m_Files.size(); i++)
    {
        if (m_Parent->m_Files[i]->HasType(FILETYPE_BIN))
        {
            if (wxString(m_Parent->m_Files[i]->m_File.name).IsSameAs(wxString(m_File.name)+L".bin",false))
            {
                int j;
                for (j=0; j<(int)m_Connections[FILETYPE_BIN]->size(); j++)
                {
                    if ((*m_Connections[FILETYPE_BIN])[j]==m_Parent->m_Files[i])
                        break;
                }
                if (j==(int)m_Connections[FILETYPE_BIN]->size())
                {
                    m_Connections[FILETYPE_BIN]->push_back(m_Parent->m_Files[i]);
                    m_Parent->m_Files[i]->Load(Stage);
                }

                break;
            }
        }
    }

    CIwASDITX::Load(Stage,path);
}

//--------------------------------------------------------------------------------
void CIwASDBinFile::Load(int Stage,int pathnum)
{
    if (m_Connections.find(FILETYPE_GROUP)==m_Connections.end())
        m_Connections[FILETYPE_GROUP]=new CIwASDDataConxList(true);

    for (int i=0; i<(int)m_Parent->m_Files.size(); i++)
    {
        if (m_Parent->m_Files[i]->HasType(FILETYPE_GROUP))
        {
            if (wxString(m_File.name).IsSameAs(wxString(m_Parent->m_Files[i]->m_File.name)+L".bin",false))
            {
                int j;
                for (j=0; j<(int)m_Connections[FILETYPE_GROUP]->size(); j++)
                {
                    if ((*m_Connections[FILETYPE_GROUP])[j]==m_Parent->m_Files[i])
                        break;
                }
                if (j==(int)m_Connections[FILETYPE_GROUP]->size())
                {
                    m_Connections[FILETYPE_GROUP]->push_back(m_Parent->m_Files[i]);
                    m_Parent->m_Files[i]->Load(Stage);
                }

                break;
            }
        }
    }
}

//--------------------------------------------------------------------------------
bool CIwASDBinFile::ExpandList(std::vector<CIwASDData*>& dataList,int toType)
{
    if (m_Connections.find(FILETYPE_GROUP)==m_Connections.end() || m_Connections[FILETYPE_GROUP]->size()<1)
        return false;

    CIwASDGroup* group=(CIwASDGroup*)(*m_Connections[FILETYPE_GROUP])[0];
    if (group==NULL) return false;

    if (toType==FILETYPE_BIN)
    {
        dataList.push_back(group);
        return true;
    }
    else
        return group->ExpandList(dataList,toType);
}

//--------------------------------------------------------------------------------
bool CIwASDGroup::ExpandList(std::vector<CIwASDData*>& dataList,int toType)
{
    int k;

    if (toType==FILETYPE_BIN)
    {
        if (m_Connections.find(FILETYPE_BIN)==m_Connections.end() || m_Connections[FILETYPE_BIN]->size()<1)
        {
            CIwASDFile* file2=(CIwASDFile*)CIwTheApp->MakeDataObject(FILETYPE_FILE,L"bin");
            file2->m_File.name=m_File.name+L".bin";
            file2->m_Parent=m_Parent;

            m_Parent->m_Files.push_back((CIwASDFile*)file2);
            file2->Load(-1);
        }

        if (m_Connections.find(FILETYPE_BIN)==m_Connections.end() || m_Connections[FILETYPE_BIN]->size()<1)
            return false;

        CIwASDBinFile* bin=(CIwASDBinFile*)(*m_Connections[FILETYPE_BIN])[0];
        if (bin==NULL) return false;

        dataList.push_back(bin);
        return true;
    }

    LoadFile(false);
    if (m_Data==NULL) return false;

    for (int j=0; j<(int)m_Data->m_Fields.size(); j++)
    {
        if (m_Data->m_Fields[j]->m_Def->m_Type==MEMBERTYPE_RES)
        {
            CIwASDITXResField* res=(CIwASDITXResField*)m_Data->m_Fields[j];

            CIwASDFile* file=NULL;
            if (res->m_Data[0]=='.' && (res->m_Data[1]=='\\' || res->m_Data[1]=='/'))
            {
                for (k=0; k<(int)m_Paths.size(); k++)
                {
                    CIwASDFilename* path=CIwASDFilename::GetFile(res->m_Data.Mid(2),m_Paths[k]);
                    if (path!=NULL)
                    {
                        file=path->GetShortestFile();
                        break;
                    }
                }
            }
            else
                file=CIwTheRoot->FindDirFile(res->m_Data);

            if (file==NULL) continue;

            if (file->m_Type!=FILETYPE_GROUP) continue;

            dataList.push_back(file);
        }
    }
    return false;
}

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
#include "wxInclude.h"
#include "IwProject.h"

#ifdef I3D_OS_WINDOWS
#include <windows.h>
#include <shlobj.h>
#include <io.h>
#endif

#define REGISTRY_KEY wxT("HKEY_CURRENT_USER\\Software\\IdeaWorks3D\\ProjectManager")
#define REGISTRY_VALUE wxT("ProjectDir")

#define GLOBAL wxT("Global")
#define SHARED wxT("Shared")
#define COMP wxT("Company")
#define LOCAL wxT("Local")
#define EXT wxT(".txt")

//-----------------------------------------------------------------------------
// helper function split string into argv array
int SplitLong(char *line, char *argv[],const char list[])
{
    int i,j;
    bool zerod=true,found;
    int len=(int)strlen(line);
    int listlen=(int)strlen(list);

    int argc=0;
    argv[0]=line;

    for (i=0; i<len; i++)
    {
        found=false;
        for (j=0; j<listlen; j++)
        {
            if (line[i]==list[j])
                found=true;
        }

        if (found&&!zerod)
        {
            line[i]=0;
            zerod=true;
        }

        if (!found&&zerod)
        {
            argv[argc++]=&line[i];
            zerod=false;
        }
    }
    return argc;
}

//--------------------------------------------------------------------------------
bool CIwProject::HasPath()
{
    return m_Path.size()>0;
}

//--------------------------------------------------------------------------------
CIwProject::CIwProject() : m_Callback(NULL)
{
#ifdef I3D_OS_WINDOWS
    bool old=wxLog::EnableLogging(false);

    wxRegKey key(REGISTRY_KEY);

    if (!key.QueryValue(REGISTRY_VALUE,m_Path) || !wxDir::Exists(m_Path))
        m_Path=wxGetCwd();

    wxChar path[256];
    SHGetSpecialFolderPath(NULL,path,CSIDL_PERSONAL,0);
    m_UserPath=path;
    m_UserPath+=wxT("/Marmalade Director/");

    if (!wxDir::Exists(m_UserPath))
        wxFileName::Mkdir(m_UserPath);

    wxGetEnv(L"S3E_DIR",&m_s3eDir);

    wxLog::EnableLogging(old);
#endif
}

//--------------------------------------------------------------------------------
//find directories
void CIwProject::Setup(const wxString& fileName)
{
    bool old=wxLog::EnableLogging(false);

    m_Path=wxGetCwd();
    m_UserPath=wxStandardPaths::Get().GetUserDataDir()+L"/";

    wxTextFile fp(fileName);
    if (fp.Exists() && fp.Open())
    {
        for (int i=0; i<(int)fp.GetLineCount(); i++)
        {
            wxString rest;
            if (fp[i].StartsWith(L"path",&rest))
                m_Path=rest.Trim(false);

            if (fp[i].StartsWith(L"userpath",&rest))
                m_UserPath=rest.Trim(false);
        }
    }

    wxFileName name(m_Path,L"");
    while (name.GetDirCount()>0) {
        wxString dir=name.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR)+L"s3e";
        if (wxFileName::DirExists(dir))
        {
            m_s3eDir=dir;
            break;
        }

        name.RemoveLastDir();
    }

    if (!wxDir::Exists(m_UserPath))
        wxFileName::Mkdir(m_UserPath);

    wxLog::EnableLogging(old);
}

//--------------------------------------------------------------------------------
void CIwProject::SetPath(const wxString& path)
{
    m_Path=path;
}

//--------------------------------------------------------------------------------
//write our registry key
bool CIwProject::WriteReg()
{
#ifdef I3D_OS_WINDOWS
    wxRegKey key(REGISTRY_KEY);

    if (!key.Exists())
        key.Create();

    key.SetValue(REGISTRY_VALUE,m_Path);
#endif

    return true;
}


//--------------------------------------------------------------------------------
//read all global settings
bool CIwProject::ReadGlobals()
{
    Clear(true);

    wxString Line=wxString::Format(wxT("%s") GLOBAL LOCAL EXT,m_UserPath.c_str());
    if (!wxFileName::FileExists(Line))
        Line=wxString::Format(wxT("%s") GLOBAL LOCAL EXT,m_Path.c_str());

    Read(Line,PROJECT_LOCAL);

    Line=wxString::Format(wxT("%s") GLOBAL COMP EXT,m_Path.c_str());
    Read(Line,PROJECT_COMPANY);

    Line=wxString::Format(wxT("%s") GLOBAL SHARED EXT,m_Path.c_str());
    return Read(Line,PROJECT_SHARED);
}

//--------------------------------------------------------------------------------
//read project settings
bool CIwProject::Read(const wxString& Project)
{
    Clear(false);

    wxString Line=wxString::Format(wxT("%s%s") LOCAL EXT,m_UserPath.c_str(),Project.c_str());
    if (!wxFileName::FileExists(Line))
        Line=wxString::Format(wxT("%s%s") LOCAL EXT,m_Path.c_str(),Project.c_str());

    bool Local=Read(Line,(CIwProjectMode)(PROJECT_PROJECT|PROJECT_LOCAL));

    Line=wxString::Format(wxT("%s%s") SHARED EXT,m_Path.c_str(),Project.c_str());
    return Read(Line,(CIwProjectMode)(PROJECT_PROJECT|PROJECT_SHARED)) || Local;
}

//--------------------------------------------------------------------------------
void CIwProject::SaveLocal(const wxString& Project,bool Shared)
{
    if (Shared)
        Save(wxString::Format(wxT("%s%s") SHARED EXT,m_Path.c_str(),Project.c_str()),(CIwProjectMode)(PROJECT_PROJECT|PROJECT_SHARED));

    Save(wxString::Format(wxT("%s%s") LOCAL EXT,m_UserPath.c_str(),Project.c_str()),(CIwProjectMode)(PROJECT_PROJECT|PROJECT_LOCAL));
}

//--------------------------------------------------------------------------------
//save some of our settings
void CIwProject::Save(bool Proj,bool Shared)
{
    wxString Project;

    if (!Proj)
    {
        if (Shared)
        {
            Save(wxString::Format(wxT("%s") GLOBAL SHARED EXT,m_Path.c_str()),PROJECT_SHARED);
            Save(wxString::Format(wxT("%s") GLOBAL COMP EXT,m_Path.c_str()),PROJECT_COMPANY);
        }

        Save(wxString::Format(wxT("%s") GLOBAL LOCAL EXT,m_UserPath.c_str()),PROJECT_LOCAL);
    }
    else if (Get(L"currentproject",Project))
    {
        if (Shared)
            Save(wxString::Format(wxT("%s%s") SHARED EXT,m_Path.c_str(),Project.c_str()),(CIwProjectMode)(PROJECT_PROJECT|PROJECT_SHARED));

        Save(wxString::Format(wxT("%s%s") LOCAL EXT,m_UserPath.c_str(),Project.c_str()),(CIwProjectMode)(PROJECT_PROJECT|PROJECT_LOCAL));
    }
}

void CIwProject::DeleteSettingsByName(const wxString& Type)
{
    for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); )
    {
        if ( it->Type.IsSameAs(Type,false))
            it=m_Settings.erase(it);
        else
            ++it;
    }
}

void CIwProject::DeleteSettingsByMode(int Mode)
{
    for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); )
    {
        if ( it->Mode == Mode )
            it=m_Settings.erase(it);
        else
            ++it;
    }
}


//--------------------------------------------------------------------------------
//clear out (some) of our settings
void CIwProject::Clear(bool IncludeGlobals)
{
    if (IncludeGlobals)
        m_Settings.clear();

    else
        for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); )
        {
            if (it->Mode&PROJECT_PROJECT)
                it=m_Settings.erase(it);
            else ++it;
        }
}

//--------------------------------------------------------------------------------
//save file (internal)
void CIwProject::Save(const wxString& FileName,CIwProjectMode Mode)
{
    wxTextFile fp(FileName);

    for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); ++it)
    {
        if (it->Mode==Mode)
            fp.AddLine(wxString::Format(wxT("%s\t%s"),it->Type.c_str(),it->Data.c_str()));
    }

    fp.Write();
}

//--------------------------------------------------------------------------------
//read (internal)
bool CIwProject::Read(const wxString& FileName,CIwProjectMode Mode)
{
    wxTextFile fp(FileName);
    if (!fp.Exists())
        return false;

    fp.Open();

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        fp[i].Replace(wxT("\t"),wxT("    "));

        unsigned char indent=0;
        for (; indent<fp[i].size(); indent++)
        {
            if (fp[i][(int)indent]!=wxT(' '))
                break;
        }

        fp[i].Trim();
        fp[i].Trim(false);

        if (fp[i].size()<1) continue;

        if (fp[i].Find(wxT(' '))==wxNOT_FOUND)
            Add(fp[i],wxT(""),Mode,indent);
        else
            Add(fp[i].BeforeFirst(wxT(' ')),fp[i].AfterFirst(wxT(' ')).Trim(false),Mode,indent);
    }
    return true;
}

//--------------------------------------------------------------------------------
//add new setting
void CIwProject::Add(const wxString& Type,const wxString& Data,bool Project,bool Shared,unsigned char Indent)
{
    Add(Type,Data,(CIwProjectMode)((Project ? PROJECT_PROJECT : PROJECT_LOCAL)|(Shared ? PROJECT_SHARED : PROJECT_LOCAL)),Indent);
}

//--------------------------------------------------------------------------------
void CIwProject::SetOrAdd(const wxString& Type,const wxString& Data,bool Project,bool Shared)
{
    CIwProjectMode mode=(CIwProjectMode)((Project ? PROJECT_PROJECT : PROJECT_LOCAL)|(Shared ? PROJECT_SHARED : PROJECT_LOCAL));
    CIwProjectSetting *Setting=Get(Type,0);
    if (Setting!=NULL && (Setting->Mode&(PROJECT_PROJECT|PROJECT_LOCAL|PROJECT_SHARED))==mode)
        Set(Type,Data);
    else
    {
        CIwProjectSetting Setting;

        Setting.Mode=mode;
        Setting.Type=Type;
        Setting.Data=Data;
        Setting.Indent=0;

        m_Settings.insert(m_Settings.begin(),Setting);
    }
}

//--------------------------------------------------------------------------------
//add new setting
void CIwProject::Add(const wxString& Type,const wxString& Data,CIwProjectMode Mode,unsigned char Indent)
{
    m_Settings.resize(m_Settings.size()+1);
    CIwProjectSetting& Setting=m_Settings.back();

    Setting.Mode=Mode;
    Setting.Type=Type;
    Setting.Data=Data;
    Setting.Indent=Indent;
}

//--------------------------------------------------------------------------------
//add temporary setting (not saved)
void CIwProject::AddTemp(const wxString& Type,const wxString& Data,bool Project,unsigned char Indent)
{
    m_Settings.resize(m_Settings.size()+1);
    CIwProjectSetting& Setting=m_Settings.back();

    Setting.Mode=(CIwProjectMode)((Project ? PROJECT_PROJECT|PROJECT_LOCAL : PROJECT_LOCAL)|PROJECT_TEMP);
    Setting.Type=Type;
    Setting.Data=Data;
    Setting.Indent=Indent;
}

//--------------------------------------------------------------------------------
//get absolute path from setting
bool CIwProject::GetFile(const wxString& Type,wxString& Data,int Item)
{
    wxString Root;
    wxString Line;

    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    int len=Setting->Data.Find(wxT(':'),true);
    if (len!=wxNOT_FOUND && len!=1)
    {
        if (!wxGetEnv(Setting->Data.BeforeLast(wxT(':')),&Line) && Setting->Data.BeforeLast(wxT(':')).IsSameAs(L"S3E_DIR",false))
            Line=m_s3eDir;

        Line+=Setting->Data.AfterLast(wxT(':'));
    }
    else
        Line=Setting->Data;

    if (Get(wxT("localroot"),Root) && (Line.size()>2) && ((Line[1]!=wxT(':')) && (Line[0]!=wxT('\\')) && (Line[0]!=wxT('/'))))
        Data=wxString::Format(wxT("%s/%s"),Root.c_str(),Line.c_str());
    else
        Data=Line;

    Data.Replace(L"\\",L"/");
    wxFileName name(Data);
    name.Normalize(wxPATH_NORM_ALL);
    Data=name.GetFullPath();

    if (!Data.EndsWith(L"\\") && !Data.EndsWith(L"/"))
        Data+=L"/";

    return true;
}

//--------------------------------------------------------------------------------
//get setting (internal)
CIwProjectSetting *CIwProject::Get(const wxString& Type,int Item)
{
    std::vector<wxString> argv;
    if (SuperSplit(Type,argv,wxT("."))<1)
        return NULL;

    int i=0;
    int Indent=-1;

    for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); ++it)
    {
        if (it->Type.IsSameAs(argv[i],false))
        {
            if (Item==0)
            {
                if (Indent>=it->Indent)
                    return NULL;

                if (i+1==(int)argv.size())
                    return &(*it);
                else
                {
                    i++;
                    Indent=it->Indent;
                }
            }
            else
                Item--;
        }
    }
    return NULL;
}

//--------------------------------------------------------------------------------
//get a setting, for the i'th value Item=i returns NULL if not found
bool CIwProject::Get(const wxString& Type,wxString& Data,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    Data=Setting->Data;
    return true;
}

//--------------------------------------------------------------------------------
//set a setting, for the i'th value Item=i
bool CIwProject::Set(const wxString& Type,const wxString& Data,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    if ((m_Callback==NULL) || m_Callback(Type,Data,m_CallbackData))
        Setting->Data=Data;

    return true;
}

//--------------------------------------------------------------------------------
//does it have a setting
bool CIwProject::Has(const wxString& Type,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    return Setting!=NULL;
}
//--------------------------------------------------------------------------------
//is it a project setting
bool CIwProject::IsProject(const wxString& Type,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    return (Setting->Mode&PROJECT_PROJECT)==PROJECT_PROJECT;
}
//--------------------------------------------------------------------------------
//is it a shared setting
bool CIwProject::IsShared(const wxString& Type,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    return (Setting->Mode&PROJECT_SHARED)==PROJECT_SHARED;
}
//--------------------------------------------------------------------------------
//is it a temporary setting
bool CIwProject::IsTemp(const wxString& Type,int Item)
{
    CIwProjectSetting *Setting=Get(Type,Item);
    if (!Setting) return false;

    return (Setting->Mode&PROJECT_TEMP)==PROJECT_TEMP;
}

//--------------------------------------------------------------------------------
//setup a callback that is called whenever a setting is changed
void CIwProject::SetupCallback(ProjectModifyFn Fn,void *Data)
{
    m_Callback=Fn;
    m_CallbackData=Data;
}

//--------------------------------------------------------------------------------
//change project name
void CIwProject::ChangeProjectName(const wxString& NewName,wxString& OldName)
{
    CIwProjectSetting* Setting=NULL;
    if (OldName.empty())
    {
        Setting=Get(wxT("currentproject"),0);
        if (!Setting) return;

        OldName=Setting->Data;
    }

    CIwProjectSetting* Setting2=Get(wxT("project"),0);
    for (int i=1; Setting2!=NULL; i++)
    {
        if (Setting2->Data.IsSameAs(OldName,false))
        {
            Setting2->Data=NewName;
            break;
        }

        Setting2=Get(wxT("project"),i);
    }

    if (Setting!=NULL)
        Setting->Data=NewName;
}

//--------------------------------------------------------------------------------
//change project name
//void CIwProject::DeleteProject()
//{
//
//	CIwProjectSetting* Setting=Get("currentproject",0);
//	if(!Setting) return;
//
//	for(std::list<CIwProjectSetting>::iterator it=m_Settings.begin();it!=m_Settings.end();++it)
//	{
//		CIwProjectSetting &Temp=*it;
//
//		if(!stricmp(Temp.Type.c_str(),"project"))
//			if(!stricmp(Temp.Data.c_str(),Setting->Data.c_str()))
//			{
//				m_Settings.erase(it);
//				break;
//			}
//	}
//	// remove the project local file
//	char Line[256]="";
//	sprintf(Line,"%s%s" LOCAL EXT,m_UserPath,Setting->Data.c_str());
//	if(_access(Line,0)==-1)
//		sprintf(Line,"%s%s" LOCAL EXT,m_Path,Setting->Data.c_str());
//
//	::wxRemoveFile(Line);
//
//}

void CIwProject::WriteSettingsToControl(wxTextCtrl* txtCtrl,int mode)
{
    if (txtCtrl==NULL) return;

    for (std::list<CIwProjectSetting>::iterator it=m_Settings.begin(); it!=m_Settings.end(); ++it)
    {
        CIwProjectSetting &Temp=*it;
        if (Temp.Mode==mode)
            txtCtrl->AppendText(wxString::Format(wxT("%s  %s\n"),Temp.Type.c_str(),Temp.Data.c_str()));
    }
    txtCtrl->ShowPosition(0);
}

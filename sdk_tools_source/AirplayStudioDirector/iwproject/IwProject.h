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
//-----------------------------------------------------------------------------
//project settings management
//--------------------------------------------------------------------------------
#if !defined(IWPROJECT_H)
#define IWPROJECT_H

#define PATH_LENGTH 256

#include "wx/string.h"
#include <list>

class WXDLLEXPORT wxTextCtrl;

typedef bool (*ProjectModifyFn)(const wxString& Type,const wxString& Data,void* UserData);

//--------------------------------------------------------------------------------
//types of settings
enum CIwProjectMode
{
    PROJECT_LOCAL=1,
    PROJECT_SHARED=2,
    PROJECT_PROJECT=4,
    PROJECT_TEMP=8,
    PROJECT_COMPANY=16,
};

//--------------------------------------------------------------------------------
//a setting
class CIwProjectSetting
{
public:
    wxString Type;
    wxString Data;
    CIwProjectMode Mode;
    //bool Shared;
    //bool Project;
    //bool Temp;
    unsigned char Indent;
};

//--------------------------------------------------------------------------------
//project settings handler
class CIwProject
{
public:
    CIwProject();
    void Setup(const wxString& fileName);
    bool HasPath();
    void SetPath(const wxString& Path);
    wxString GetPath() { return m_Path; }
    wxString GetUserPath() { return m_UserPath; }
    bool WriteReg();    //should only be done in the project manager

    bool ReadGlobals();
    bool Read(const wxString& Project);

    void Save(bool Project,bool Shared=false);
    void SaveLocal(const wxString& Project,bool Shared=false);

    bool Get(const wxString& Type,wxString& Data,int Item=0);
    bool GetFile(const wxString& Type,wxString& Data,int Item=0);
    bool Set(const wxString& Type,const wxString& Data,int Item=0);
    void Add(const wxString& Type,const wxString& Data,CIwProjectMode Mode,unsigned char Indent=0);
    void AddTemp(const wxString& Type,const wxString& Data,bool Project,unsigned char Indent=0);
    void Add(const wxString& Type,const wxString& Data,bool Project,bool Shared,unsigned char Indent=0);
    void SetOrAdd(const wxString& Type,const wxString& Data,bool Project,bool Shared);

    bool Has(const wxString& Type,int Item=0);
    bool IsProject(const wxString& Type,int Item=0);
    bool IsShared(const wxString& Type,int Item=0);
    bool IsTemp(const wxString& Type,int Item=0);

    void SetupCallback(ProjectModifyFn Fn,void *Data);
    void ChangeProjectName(const wxString& NewName,wxString& OldName);
    //	void DeleteProject();
    void DeleteSettingsByName(const wxString& Type);        // delete all settings
    void DeleteSettingsByMode(int Mode);

    int  NumProjectSettings(){ return m_Settings.size(); }

    void WriteSettingsToControl( wxTextCtrl* txtCtrl, int mode );
private:
    bool Read(const wxString& FileName,CIwProjectMode Mode);
    void Clear(bool IncludeGlobals);
    void Save(const wxString& FileName,CIwProjectMode Mode);
    CIwProjectSetting *Get(const wxString& Type,int Item);

    wxString m_Path;
    std::list<CIwProjectSetting> m_Settings;

    ProjectModifyFn m_Callback;
    void* m_CallbackData;
public:
    wxString m_UserPath;
    wxString m_s3eDir;
};

extern int SplitLong(char *line, char *argv[],const char list[]);

#endif // !defined(IWPROJECT_H)

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
#if !defined(IwWinSourceControl_H)
#define IwWinSourceControl_H

#ifndef IwWinAppBase
#define IwWinAppBase
//--------------------------------------------------------------------------------
//base for app class to add SC tick handling
class CIwWinAppBase : public wxApp
{
public:
    virtual void ChangeTimer(bool Add)=0;
    virtual void LogProcess(const wxString& Line,const wxString& Name,bool Final)=0;
};
#endif

//--------------------------------------------------------------------------------
//types of result returned
enum EIwWinSourceControlResultType
{
    SC_RESULT_STRING,
    SC_RESULT_INT,
    SC_RESULT_BOOL,
};

//--------------------------------------------------------------------------------
//result definition
struct CIwWinSourceControlResultDef
{
    wxString                      m_Name;
    EIwWinSourceControlResultType m_Type;
    wxString                      m_Prefix;
};

//--------------------------------------------------------------------------------
//value from the result
struct CIwWinSourceControlResultValue
{
    CIwWinSourceControlResultDef *m_Def;
    wxString                      m_String;
    int                           m_Int;
};

//--------------------------------------------------------------------------------
//result of an SC command
class CIwWinSourceControlResult
{
public:
    std::vector<CIwWinSourceControlResultValue> m_Results;
public:
    bool GetBool(const wxString& Name);
    wxString GetString(const wxString& Name);
    int GetInt(const wxString& Name);
};

//--------------------------------------------------------------------------------
//definition of an SC command
struct CIwWinSourceControlCommand
{
    wxString                                   m_Name;
    wxString                                   m_Command;
    std::vector<CIwWinSourceControlResultDef*> m_Results;

    ~CIwWinSourceControlCommand();
    bool                                       ProcessResult(const wxString& Line,CIwWinSourceControlResultValue* Result);
};

//--------------------------------------------------------------------------------
//base class for SC done callback
class CIwWinSourceControlDoneCommand
{
public:
    virtual void DoneCommand(CIwWinSourceControlResult* Result)=0;
};

//base class for SC continuing callback
class CIwWinSourceControlGotAsyncResult
{
public:
    virtual void GotString(const wxString& Type,const wxString& String)=0;
    virtual void GotInt(const wxString& Type,int Value)=0;
    virtual void GotBool(const wxString& Type)=0;
    virtual void Done()=0;
};

class CIwSCFile
{
public:
    virtual wxString Expand(const wxString& text)=0;
};

class CIwProjSourceControl;
class CIwProjSourceControlMultiple;

//--------------------------------------------------------------------------------
//generic source control interface system
class CIwWinSourceControl
{
public:
    wxString m_BaseCommand;
    wxString m_DepotRoot;
    wxString m_LocalRoot;
    wxString m_User;
    wxString m_Client;
    wxString m_CurrChange;
    wxString m_ChangeMessage;
    bool m_Active,m_Backup,m_ChangeList;
    std::vector<CIwWinSourceControlCommand*> m_Commands;
    std::list<CIwProjSourceControl*> m_Processes;
    std::list<CIwProjSourceControlMultiple*> m_MultProcesses;
public:
    CIwWinSourceControl() : m_CurrChange(wxT("default")),m_Active(false),m_Backup(true),m_ChangeList(false) {}
    ~CIwWinSourceControl();
    bool Load(const wxString& Filename,bool NoSettings=false);
    CIwWinSourceControlResult* Run(const wxString& Command,CIwSCFile* File=NULL);
    CIwWinSourceControlResult* Run(const wxString& Command,const wxString& File,bool Quiet=false);
    bool RunAsync(const wxString& Command,CIwWinSourceControlDoneCommand* DoneCommand,CIwSCFile* File=NULL);
    bool RunMultipleAsync(const wxString& Command,CIwWinSourceControlGotAsyncResult* Result,CIwSCFile* File=NULL);
    void Reset();
    bool RunNoWX(const wxString& Command,const wxString& File);
    bool IsUnderLocalRoot(const wxString& path);
protected:
    CIwWinSourceControlCommand* GetCmd(const wxString& Command);
    void Expand(wxString& text);
    wxString GetCmdLine(CIwWinSourceControlCommand* Cmd,CIwSCFile* File);
    wxString GetCmdLine(CIwWinSourceControlCommand* Cmd,const wxString& File);
};

//--------------------------------------------------------------------------------
//process of an async SC command
class CIwProjSourceControl : public wxProcess
{
public:
    CIwProjSourceControl(CIwWinSourceControl* Parent,CIwWinSourceControlCommand* Cmd,CIwWinSourceControlDoneCommand* Done) :
        wxProcess(wxPROCESS_REDIRECT),m_Parent(Parent),m_Cmd(Cmd),m_Done(Done),m_PID(-1),m_Working(true) {}
    virtual ~CIwProjSourceControl();
    virtual void OnTerminate(int pid,int status);
private:
    CIwWinSourceControl* m_Parent;
    CIwWinSourceControlCommand* m_Cmd;
    CIwWinSourceControlDoneCommand* m_Done;
public:
    long m_PID;
    bool m_Working;
};

//--------------------------------------------------------------------------------
//process of a continuing SC command
class CIwProjSourceControlMultiple : public wxProcess
{
public:
    CIwProjSourceControlMultiple(CIwWinSourceControl* Parent,CIwWinSourceControlCommand* Cmd,CIwWinSourceControlGotAsyncResult* DoResult) :
        wxProcess(wxPROCESS_REDIRECT),m_Parent(Parent),m_Cmd(Cmd),m_DoResult(DoResult),m_PID(-1) {}
    virtual ~CIwProjSourceControlMultiple();
    virtual void OnTerminate(int pid,int status);

    void Process(bool Final=false);
    void Process(const wxString& Line);
private:
    CIwWinSourceControl* m_Parent;
    CIwWinSourceControlCommand* m_Cmd;
public:
    CIwWinSourceControlGotAsyncResult* m_DoResult;
    long m_PID;
};

#endif

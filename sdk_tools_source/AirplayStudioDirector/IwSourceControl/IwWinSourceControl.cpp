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
//--------------------------------------------------------------------------------
// interface to source control (i.e. perforce)
//--------------------------------------------------------------------------------
#include "wxInclude.h"
#include "IwWinSourceControl.h"

//--------------------------------------------------------------------------------
CIwWinSourceControlCommand::~CIwWinSourceControlCommand()
{
    for (std::vector<CIwWinSourceControlResultDef*>::iterator it=m_Results.begin(); it!=m_Results.end(); ++it)
    {
        delete *it;
    }
}

//--------------------------------------------------------------------------------
CIwWinSourceControl::~CIwWinSourceControl()
{
    if (m_CurrChange!=wxT("default"))
        Run(wxT("changelistdelete"));

    for (std::vector<CIwWinSourceControlCommand*>::iterator it=m_Commands.begin(); it!=m_Commands.end(); ++it)
    {
        delete *it;
    }
    for (std::list<CIwProjSourceControl*>::iterator sit=m_Processes.begin(); sit!=m_Processes.end(); ++sit)
    {
        delete *sit;
    }
    for (std::list<CIwProjSourceControlMultiple*>::iterator mit=m_MultProcesses.begin(); mit!=m_MultProcesses.end(); ++mit)
    {
        delete *mit;
    }
}

//--------------------------------------------------------------------------------
//stop interacting with SC
void CIwWinSourceControl::Reset()
{
    for (std::list<CIwProjSourceControl*>::iterator sit=m_Processes.begin(); sit!=m_Processes.end(); ++sit)
    {
        (*sit)->m_Working=false;
    }
    //delete *sit;
    for (std::list<CIwProjSourceControlMultiple*>::iterator mit=m_MultProcesses.begin(); mit!=m_MultProcesses.end(); ++mit)
    {
        (*mit)->m_DoResult=NULL;
    }
    //delete *mit;
    m_Processes.clear();
    m_MultProcesses.clear();
}

//--------------------------------------------------------------------------------
//get bool from results
bool CIwWinSourceControlResult::GetBool(const wxString& Name)
{
    for (std::vector<CIwWinSourceControlResultValue>::iterator it=m_Results.begin(); it!=m_Results.end(); ++it)
    {
        CIwWinSourceControlResultValue& Val=*it;
        if (!Val.m_Def->m_Name.IsSameAs(Name,false))
            continue;

        return Val.m_Def->m_Type==SC_RESULT_BOOL;
    }
    return false;
}

//--------------------------------------------------------------------------------
//get string from results
wxString CIwWinSourceControlResult::GetString(const wxString& Name)
{
    for (std::vector<CIwWinSourceControlResultValue>::iterator it=m_Results.begin(); it!=m_Results.end(); ++it)
    {
        CIwWinSourceControlResultValue& Val=*it;
        if (!Val.m_Def->m_Name.IsSameAs(Name,false))
            continue;

        if (Val.m_Def->m_Type==SC_RESULT_STRING)
            return Val.m_String.c_str();
        else
            return wxT("");
    }
    return wxT("");
}

//--------------------------------------------------------------------------------
//get int from results
int CIwWinSourceControlResult::GetInt(const wxString& Name)
{
    for (std::vector<CIwWinSourceControlResultValue>::iterator it=m_Results.begin(); it!=m_Results.end(); ++it)
    {
        CIwWinSourceControlResultValue& Val=*it;
        if (!Val.m_Def->m_Name.IsSameAs(Name,false))
            continue;

        if (Val.m_Def->m_Type==SC_RESULT_INT)
            return Val.m_Int;
        else
            return -2;
    }
    return -1;
}

//--------------------------------------------------------------------------------
//get SC command template (internal)
CIwWinSourceControlCommand* CIwWinSourceControl::GetCmd(const wxString& Command)
{
    if (!m_Active) return NULL;

    for (std::vector<CIwWinSourceControlCommand*>::iterator it=m_Commands.begin(); it!=m_Commands.end(); ++it)
    {
        CIwWinSourceControlCommand* Cmd=*it;

        if (Cmd->m_Name.IsSameAs(Command,false))
            return Cmd;
    }
    return NULL;
}

//--------------------------------------------------------------------------------
void CIwWinSourceControl::Expand(wxString& text)
{
    text.Replace(wxT("%c"),m_Client);
    text.Replace(wxT("%u"),m_User);
    text.Replace(wxT("%r"),m_LocalRoot);
    text.Replace(wxT("%h"),m_CurrChange);
    text.Replace(wxT("%d"),m_DepotRoot);
    text.Replace(wxT("%t"),wxDateTime::Now().Format());
    text.Replace(wxT("%m"),m_ChangeMessage);

    text.Replace(wxT("#"),wxT("%23"));
}

//--------------------------------------------------------------------------------
//replace command template with values (internal)
wxString CIwWinSourceControl::GetCmdLine(CIwWinSourceControlCommand* Cmd,CIwSCFile* File)
{
    wxString result=m_BaseCommand+wxT(" ");

    if (File!=NULL)
        result+=File->Expand(Cmd->m_Command);
    else
        result+=Cmd->m_Command;

    Expand(result);
    return result;
}

//--------------------------------------------------------------------------------
//replace command template with values (internal)
wxString CIwWinSourceControl::GetCmdLine(CIwWinSourceControlCommand* Cmd,const wxString& File)
{
    wxString result=m_BaseCommand+wxT(" ")+Cmd->m_Command;

    result.Replace(wxT("%p"),File);

    Expand(result);
    return result;
}

//--------------------------------------------------------------------------------
//extract values from a line of result of SC command (internal)
bool CIwWinSourceControlCommand::ProcessResult(const wxString& Line,CIwWinSourceControlResultValue* Value)
{
    for (std::vector<CIwWinSourceControlResultDef*>::iterator it=m_Results.begin(); it!=m_Results.end(); ++it)
    {
        CIwWinSourceControlResultDef* Def=(*it);

        wxString rest;
        if (Line.Lower().StartsWith(Def->m_Prefix.Lower(),&rest))
        {
            long res;
            Value->m_Def=Def;
            switch (Def->m_Type)
            {
            case SC_RESULT_STRING:
                Value->m_String=rest;
                break;
            case SC_RESULT_INT:
                rest.ToLong(&res);
                Value->m_Int=res;
                break;
            default:
                break;
            }
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------
//run an SC command (syncroniously)
CIwWinSourceControlResult* CIwWinSourceControl::Run(const wxString& Command,CIwSCFile* File)
{
    CIwWinSourceControlResult* Result=NULL;
    CIwWinSourceControlCommand* Cmd=GetCmd(Command);
    if (Cmd==NULL) return NULL;

    wxString Line=GetCmdLine(Cmd,File);

    wxArrayString Output;
    wxArrayString Error;
    wxExecute(Line,Output,Error);

    if (Error.size()>0)
    {
        for (int i=0; i<(int)Error.size(); i++)
        {
            Error[i].Replace(wxT("%23"),wxT("#"));
            Error[i].Replace(wxT("\n"),wxT(""));
            if (Error[i].size()>0)
                wxLogMessage(wxT("Source Control Warning: %s"),Error[i].c_str());
        }
        return NULL;
    }

    Result=new CIwWinSourceControlResult();

    for (wxArrayString::iterator it=Output.begin(); it!=Output.end(); ++it)
    {
        CIwWinSourceControlResultValue Value;
        if (Cmd->ProcessResult((*it),&Value))
            Result->m_Results.push_back(Value);
    }

    return Result;
}

//--------------------------------------------------------------------------------
//run an SC command (syncroniously)
CIwWinSourceControlResult* CIwWinSourceControl::Run(const wxString& Command,const wxString& File,bool Quiet)
{
    CIwWinSourceControlResult* Result;
    CIwWinSourceControlCommand* Cmd=GetCmd(Command);
    if (Cmd==NULL) return NULL;

    wxString Line=GetCmdLine(Cmd,File);

    wxArrayString Output;
    wxArrayString Error;
    wxExecute(Line,Output,Error);

    if (Error.size()>0)
    {
        if (Quiet) return NULL;

        for (int i=0; i<(int)Error.size(); i++)
        {
            Error[i].Replace(wxT("%23"),wxT("#"));
            Error[i].Replace(wxT("\n"),wxT(""));
            if (Error[i].size()>0)
                wxLogMessage(wxT("Source Control Warning: %s"),Error[i].c_str());
        }
        return NULL;
    }

    Result=new CIwWinSourceControlResult();

    for (wxArrayString::iterator it=Output.begin(); it!=Output.end(); ++it)
    {
        CIwWinSourceControlResultValue Value;
        if (Cmd->ProcessResult((*it),&Value))
            Result->m_Results.push_back(Value);
    }

    return Result;
}

//--------------------------------------------------------------------------------
//run SC command when in batch mode (no results returned)
bool CIwWinSourceControl::RunNoWX(const wxString& Command,const wxString& File) /*
                                                                                   CIwWinSourceControlResult* Result;
                                                                                   CIwWinSourceControlCommand* Cmd=GetCmd(Command);
                                                                                   if(Cmd==NULL) return false;

                                                                                   wxString Line=GetCmdLine(Cmd,File);

                                                                                   STARTUPINFO si;
                                                                                   PROCESS_INFORMATION pi;
                                                                                   wxZeroMemory(si);
                                                                                   si.cb = sizeof(si);

                                                                                   return CreateProcess(NULL,Line.c_str(),NULL,NULL,FALSE,0,NULL,NULL,&si,&pi);*/
{return false;
}

//--------------------------------------------------------------------------------
CIwProjSourceControl::~CIwProjSourceControl()
{
    if (m_PID!=-1)
        wxProcess::Kill(m_PID,wxSIGKILL);
}

//--------------------------------------------------------------------------------
//SC async command as finished
void CIwProjSourceControl::OnTerminate(int,int)
{
    if (m_Working)
    {
        if (IsErrorAvailable())
        {
            wxTextInputStream ts(*GetErrorStream());
            while (IsErrorAvailable())
            {
                wxString Line=ts.ReadLine();
                bool ReadFinal=!IsErrorAvailable();

                ((CIwWinAppBase*)wxTheApp)->LogProcess(Line,wxT("Source Control"),ReadFinal);
            }
            if (m_Done!=NULL)
                m_Done->DoneCommand(NULL);
        }
        else
        {
            CIwWinSourceControlResult Result;
            wxTextInputStream ts(*GetInputStream());

            while (IsInputAvailable())
            {
                wxString line=ts.ReadLine();
                CIwWinSourceControlResultValue Value;
                if (m_Cmd->ProcessResult(line,&Value))
                    Result.m_Results.push_back(Value);
            }

            if (m_Done!=NULL)
            {
                m_Done->DoneCommand(&Result);   //send processed results to callback
            }
        }
    }

    m_Parent->m_Processes.remove(this);
    m_PID=-1;
    delete this;
}

//--------------------------------------------------------------------------------
//get results for ongoing SC command (internal)
void CIwProjSourceControlMultiple::Process(const wxString& Ptr)
{
    CIwWinSourceControlResultValue Value;

    if (!m_DoResult)
        return;

    if (!m_Cmd->ProcessResult(Ptr,&Value))
        return;

    switch (Value.m_Def->m_Type)
    {
    case SC_RESULT_STRING:
        m_DoResult->GotString(Value.m_Def->m_Name,Value.m_String);
        break;
    case SC_RESULT_INT:
        m_DoResult->GotInt(Value.m_Def->m_Name,Value.m_Int);
        break;
    case SC_RESULT_BOOL:
        m_DoResult->GotBool(Value.m_Def->m_Name);
        break;
    }
}

//--------------------------------------------------------------------------------
//get results for an ongoing SC command
void CIwProjSourceControlMultiple::Process(bool Final)
{
    if (IsErrorAvailable())
    {
        wxTextInputStream ts(*GetErrorStream());
        while (IsErrorAvailable())
        {
            wxString Line=ts.ReadLine();
            bool ReadFinal=!IsErrorAvailable();

            ((CIwWinAppBase*)wxTheApp)->LogProcess(Line,wxT("Source Control"),ReadFinal);
        }
    }
    else
    {
        while (IsInputAvailable())
        {
            wxTextInputStream ts(*GetInputStream());

            while (IsInputAvailable())
            {
                Process(ts.ReadLine());
            }
        }
    }
}

//--------------------------------------------------------------------------------
//finished an ongoing SC command
void CIwProjSourceControlMultiple::OnTerminate(int,int)
{
    Process(true);

    if (m_DoResult)
        m_DoResult->Done();

    m_Parent->m_MultProcesses.remove(this);

    ((CIwWinAppBase*)wxTheApp)->ChangeTimer(false);

    m_PID=-1;
    delete this;
}

CIwProjSourceControlMultiple::~CIwProjSourceControlMultiple()
{
    if (m_PID!=-1)
        wxProcess::Kill(m_PID,wxSIGKILL);
}

//--------------------------------------------------------------------------------
//run SC command async
bool CIwWinSourceControl::RunAsync(const wxString& Command,CIwWinSourceControlDoneCommand* DoneCommand,CIwSCFile* File)
{
    CIwWinSourceControlCommand* Cmd=GetCmd(Command);
    if (Cmd==NULL)
    {
        delete DoneCommand;
        return false;
    }

    wxString Line=GetCmdLine(Cmd,File);

    CIwProjSourceControl* Process=new CIwProjSourceControl(this,Cmd,DoneCommand);
    m_Processes.push_back(Process);
    Process->m_PID=wxExecute(Line.BeforeFirst(wxT('\n')),wxEXEC_ASYNC,Process);

    if (Line.Find(wxT('\n'))!=wxNOT_FOUND)
    {
        wxString rest=Line.AfterFirst(wxT('\n'));
        Process->GetOutputStream()->Write(rest.mb_str(),rest.size());
    }

    return true;
}

//--------------------------------------------------------------------------------
//run SC command async, with ongoing results output
bool CIwWinSourceControl::RunMultipleAsync(const wxString& Command,CIwWinSourceControlGotAsyncResult* Result,CIwSCFile* File)
{
    CIwWinSourceControlCommand* Cmd=GetCmd(Command);
    if (Cmd==NULL) return false;

    wxString Line=GetCmdLine(Cmd,File);
    //*
    CIwProjSourceControlMultiple* Process=new CIwProjSourceControlMultiple(this,Cmd,Result);
    m_MultProcesses.push_back(Process);


    Process->m_PID=wxExecute(Line,wxEXEC_ASYNC,Process);

    ((CIwWinAppBase*)wxTheApp)->ChangeTimer(true);
    /*/

       CIwWinSourceControlResultValue Value;
       wxArrayString Out,Err;
       wxExecute(Line,Out,Err);

       int i;
       for(i=0;i<Err.size();i++)
        ((CIwWinAppBase*)wxTheApp)->LogProcess(Line,"Source Control",Err[i].c_str());

       for(i=0;i<Out.size();i++)
       {
        if(Cmd->ProcessResult(Out[i].c_str(),&Value))
            switch(Value.m_Def->m_Type)
            {
                case SC_RESULT_STRING:
                    Result->GotString(Value.m_Def->m_Name.c_str(),Value.m_String.c_str());
                    break;
                case SC_RESULT_INT:
                    Result->GotInt(Value.m_Def->m_Name.c_str(),Value.m_Int);
                    break;
                case SC_RESULT_BOOL:
                    Result->GotBool(Value.m_Def->m_Name.c_str());
                    break;
            }
       }
       //*/

    return true;
}

//--------------------------------------------------------------------------------
//load SC description file
bool CIwWinSourceControl::Load(const wxString& Filename,bool NoSettings)
{
    wxTextFile fp(Filename);
    if (!fp.Exists() || !fp.Open())
    {
        m_Active=false;
        return false;
    }

    m_Active=true;

    CIwWinSourceControlCommand* Cmd=NULL;
    int CmdIndent=256;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> argv;
        if (SuperSplit(fp[i],argv,wxT(" \t\n"))<2) continue;

        int Indent=0;
        for (; Indent<(int)fp[i].size(); Indent++)
        {
            if (fp[i][Indent]!=' ' && fp[i][Indent]!='\t')
                break;
        }

        if (Indent>CmdIndent)    //result definition
        {
            if (argv.size()<3) continue;

            CIwWinSourceControlResultDef* Result=new CIwWinSourceControlResultDef();
            Cmd->m_Results.push_back(Result);

            Result->m_Name=argv[0];
            Result->m_Prefix=argv[2];

            if (argv[1].IsSameAs(wxT("string"),false))
                Result->m_Type=SC_RESULT_STRING;
            else if (argv[1].IsSameAs(wxT("int"),false))
                Result->m_Type=SC_RESULT_INT;
            else //if(!stricmp(argv[1],"bool")) //set to bool as default
                Result->m_Type=SC_RESULT_BOOL;
        }
        else
        {
            if (argv[0].IsSameAs(wxT("basecommand"),false)) //exe to call
                m_BaseCommand=argv[1];
            else if (argv[0].IsSameAs(wxT("depotroot"),false)) //base of depot
                m_DepotRoot=argv[1];
            else        //command
            {
                Cmd=new CIwWinSourceControlCommand();
                m_Commands.push_back(Cmd);

                Cmd->m_Name=argv[0];
                Cmd->m_Command=argv[1];
                CmdIndent=Indent;

                Cmd->m_Command.Replace(wxT("\\n"),wxT("\n"));
                Cmd->m_Command.Replace(wxT("\\z"),wxT("\x1A"));
            }
        }
    }

    if (NoSettings) return true;

    wxLogNull logNull;

    CIwWinSourceControlResult* Result=Run(wxT("settings"));
    if (Result!=NULL)
    {
        m_LocalRoot=Result->GetString(wxT("localroot"));
        m_User=Result->GetString(wxT("user"));
        m_Client=Result->GetString(wxT("client"));
        delete Result;
    }
    else
        m_Active=false;

    return true;
}

//--------------------------------------------------------------------------------
bool CIwWinSourceControl::IsUnderLocalRoot(const wxString& path)
{
    if (!m_Active) return false;

    for (int i=0; i<(int)m_LocalRoot.size(); i++)
    {
        if (path[i]=='\\' && m_LocalRoot[i]=='/')
            continue;

        if (path[i]=='/' && m_LocalRoot[i]=='\\')
            continue;

        if (tolower(path[i])!=tolower(m_LocalRoot[i]))
            return false;
    }
    return true;
}

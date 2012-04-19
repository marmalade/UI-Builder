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
#include "IwASDBase.h"

wxString CIwASDLogWindow::m_PreLogFile;

//--------------------------------------------------------------------------------
//log window has started, add stored logs
void CIwASDLogWindow::Setup(wxTextCtrl *Text,const wxString& fileName)
{
    if (!fileName.empty())
#ifdef I3D_OS_WINDOWS
        m_File=_wfopen(fileName.c_str(),L"w");

#else
        m_File=fopen(fileName.mb_str(),"w");
#endif
    else if (m_File!=NULL)
    {
        fclose(m_File);
        m_File=NULL;
    }
    m_Text=Text;
    if (m_Text==NULL) return;

    for (std::vector<wxString>::iterator it=Strings.begin(); it!=Strings.end(); ++it)
    {
        m_Text->AppendText(*it);
    }
    Strings.clear();

    if (m_PreLogFile.size()==0) return;

    wxTextFile fp(m_PreLogFile);
    m_PreLogFile.clear();

    if (!fp.Exists() || !fp.Open()) return;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        if (fp[i].empty()) continue;

        wxTextPos Len=m_Text->GetLastPosition();
        m_Text->SetSelection(Len,Len);

        wxTextAttr existingColour = m_Text->GetDefaultStyle();
        wxString rest;
        if (fp[i].Lower().StartsWith(L"success: ",&rest))
        {
            m_Text->SetDefaultStyle( wxTextAttr( *wxGREEN ) );
            m_Text->AppendText(rest);
        }
        else if (fp[i].Lower().StartsWith(L"warning: ",&rest))
        {
            m_Text->SetDefaultStyle( wxTextAttr( wxColour(64,64,127) ) );
            m_Text->AppendText(rest);
        }
        else if (fp[i].Lower().StartsWith(L"error: ",&rest))
        {
            m_Text->SetDefaultStyle( wxTextAttr( *wxRED ) );
            m_Text->AppendText(rest);
        }
        else if (fp[i].Lower().StartsWith(L"message: ",&rest))
        {
            wxString rest2;
            if (rest.StartsWith(L"writing file ",&rest2))
            {
                rest2.Replace(L"\n",L"");
                m_FileList.push_back(rest2);
            }

            m_Text->AppendText(rest);
        }
        else
            m_Text->AppendText(fp[i]);

        m_Text->AppendText(L"\n");

        m_Text->SetDefaultStyle( existingColour );
    }
    if (!m_Callback.empty())
        m_Callback();
}

//--------------------------------------------------------------------------------
void CIwASDLogWindow::SetupFileList(FastDelegate0<> callback)
{
    m_Callback=callback;
    if (!m_Callback.empty())
        m_Callback();
}

//--------------------------------------------------------------------------------
//logging from wx (do we log?)
void CIwASDLogWindow::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
    switch (level)
    {
    case wxLOG_Status:
        if (!wxIsEmpty(szString))
        {
            wxString str;
            str<<L"Status: "<<szString;
            DoLogString(str,t);
        }

        break;
    case wxLOG_Trace:
        break;
    case wxLOG_Error:
        if (m_Text!=NULL)
        {
            // set colour, do the log, set it back
            wxTextAttr existingColour = m_Text->GetDefaultStyle();
            m_Text->SetDefaultStyle( wxTextAttr( *wxRED ) );
            wxLog::DoLog(level,szString,t);
            m_Text->SetDefaultStyle( existingColour );
        }
        else
            wxLog::DoLog(level,szString,t);

        break;
    case wxLOG_Warning:
        if (m_Text!=NULL)
        {
            // set colour, do the log, set it back
            wxTextAttr existingColour = m_Text->GetDefaultStyle();
            m_Text->SetDefaultStyle( wxTextAttr( wxColour(64,64,127) ) );
            wxLog::DoLog(level,szString,t);
            m_Text->SetDefaultStyle( existingColour );
        }
        else
            wxLog::DoLog(level,szString,t);

        break;
    default:
        wxLog::DoLog(level,szString,t);
        break;
    }
}

//--------------------------------------------------------------------------------
//logging from wx
void CIwASDLogWindow::DoLogString(const wxChar *szString, time_t t)
{
    wxString msg;

    msg<<szString;

    if (m_File!=NULL)
    {
        fprintf(m_File,"%s\n",msg.mb_str().data());
        fflush(m_File);
    }

    if (m_Text==NULL)
    {
#ifdef WIN32
        OutputDebugString(szString);
        OutputDebugString(L"\n");
#endif
        msg<<wxT("\n");
        Strings.push_back(msg);
        return;
    }

    wxString rest2,rest(szString);
    if (rest.StartsWith(L"writing file ",&rest2))
    {
        rest2.Replace(L"\n",L"");
        m_FileList.push_back(rest2);

        if (!m_Callback.empty())
            m_Callback();
    }

    wxTextPos Len=m_Text->GetLastPosition();
    m_Text->SetSelection(Len,Len);

    m_Text->AppendText(msg);
    m_Text->AppendText(L"\n");
}

//------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDApp, wxApp)
    EVT_IDLE(CIwASDApp::OnIdle)
    //	EVT_TIMER(CTRLID_TIMER,CIwASDApp::OnTick)
    //	EVT_TIMER(CTRLID_VIEWERTIMER,CIwASDApp::OnViewTick)
END_EVENT_TABLE()

//#ifndef ASD_AS_DLL
IMPLEMENT_APP(CIwASDApp)
//#else
//extern "C" {
//	EXP_DLL void ASDMain(int argc,char** argv);
//}

//IMPLEMENT_APP_NO_MAIN(CIwASDApp)

//#endif

//-------------------------------------------------------------------------------
//run the application
void ASDMain(int argc,char** argv)
{
    wxFileName name(wxString(argv[0],wxConvLibc));
#ifdef I3D_OS_OSX
    name.RemoveLastDir();
    name.AppendDir(L"Resources");
#endif
    wxSetWorkingDirectory(name.GetPath());

    wxEntry(argc, argv);
    wxEntryCleanup();
}

//------------------------------------------------------------------------------
void CIwProcessDelegate::OnTerminate(int pid, int status)
{
    if (m_Delegate==NULL) return;

    CIwTheApp->m_Defered.push_back(m_Delegate);
    wxWakeUpIdle();
    //m_Delegate();
}

//------------------------------------------------------------------------------
void CIwProcessLoggingDelegate::DoPoll()
{
    CIwTheApp->m_Defered.push_back(MakeDelegate(this,&CIwProcessLoggingDelegate::Poll));
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
void CIwProcessLoggingDelegate::Poll()
{
    if (IsErrorAvailable())
    {
        wxTextInputStream ts(*GetErrorStream());
        while (IsErrorAvailable())
        {
            wxString Line=ts.ReadLine();

            wxLogMessage(L"%s",Line.c_str());
        }
    }
    else
    {
        if (IsInputAvailable())
        {
            wxTextInputStream ts(*GetInputStream());

            while (IsInputAvailable())
            {
                wxString Line=ts.ReadLine();

                wxLogMessage(L"%s",Line.c_str());
            }
        }
    }
}

//------------------------------------------------------------------------------
void CIwASDApp::OnIdle(wxIdleEvent& event)
{
    wxApp::OnIdle(event);

    if (m_Frame!=NULL)
        m_Frame->m_Elements->CheckForReset();

    static bool processing=false;

    if (processing) return;

    processing=true;
    for (int i=0; i<(int)m_Defered.size(); i++)
    {
        m_Defered[i]();
    }
    m_Defered.clear();
    processing=false;
}

//------------------------------------------------------------------------------
bool CIwASDApp::OnInit()
{
    SetAppName(IwGetSDKIdent()+L"Tools");
    InitAllImageHandlers();

    for (int i=0; i<MAX_EXTRA_DATA; i++)
    {
        m_ExtraData[i]=NULL;
    }

    if (!wxFileExists(wxGetCwd()+L"/data/VLayoutShared.svl"))
    {
        wxFileName name(argv[0]);
        wxSetWorkingDirectory(name.GetPath(false));
    }

    //start logging
    m_Log=new CIwASDLogWindow();
    wxLog::SetActiveTarget(m_Log);

    //setup default path (modules could override)
    m_DirList[L"resource"]=wxStandardPaths::Get().GetResourcesDir();
    m_DirList[L"viewer"]=wxStandardPaths::Get().GetResourcesDir()+L"/data/";
    m_DirList[L"layout"]=L"{viewer}VLayoutShared.svl";

    m_DirList[L"user"]=wxStandardPaths::Get().GetUserDataDir();

    m_DirList[L"currproject"]=L"Layout";
    m_DirList[L"projectdetails"]=L"";

    //add modules
    RegisterModules(this);
    if (m_StartupProblem) return false;

    //create main window
    m_Frame=new CIwLayoutFrame();

    //create the frame
    m_Frame->Reset();

    // refresh update here but might work elsewhere
    m_Frame->Show(true);
    m_Frame->Refresh();
    m_Frame->Update();

    PostWindowInit();

    //m_Frame->ShowModal();
    //delete m_Frame;
    //m_Frame=NULL;

    //return false;
    return true;
}

//------------------------------------------------------------------------------
void CIwASDApp::PostWindowInit()
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        m_Modules[i]->PostWindowInit();
    }
}

//------------------------------------------------------------------------------
void CIwASDApp::Exit()
{
    if (m_Frame!=NULL)
        m_Frame->Close();
    else
        m_StartupProblem=true;
}

//------------------------------------------------------------------------------
CIwASDApp::~CIwASDApp()
{
    for (int i=(int)m_Modules.size()-1; i>=0; i--)
    {
        delete m_Modules[i];
    }

    if (m_RootData!=NULL)
        delete m_RootData;
}

//------------------------------------------------------------------------------
void CIwASDApp::SetProjectName(const wxString& name)
{
    m_DirList[L"currproject"]=name;

    if (m_Frame==NULL) return;

    m_Frame->SetMainTitle();
}

//------------------------------------------------------------------------------
void CIwASDApp::SetProjectDetails(const wxString& details)
{
    m_DirList[L"projectdetails"]=details;

    if (m_Frame==NULL) return;

    m_Frame->SetMainTitle();
}

//------------------------------------------------------------------------------
void CIwASDApp::AddModule(unsigned char ID,CIwModule* Module)
{
    m_Modules.push_back(Module);
    Module->Setup(this,ID);
}

//------------------------------------------------------------------------------
CIwASDData* CIwASDApp::MakeDataObject(unsigned int type,const wxString& info)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        CIwASDData* data=m_Modules[i]->MakeDataObject(type,info);

        if (data!=NULL)
            return data;
    }
    return NULL;
}

//------------------------------------------------------------------------------
CIwASDData* CIwASDApp::CloneDataObject(unsigned int type,CIwASDData* from,CIwASDData* root,bool copy)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        CIwASDData* data=m_Modules[i]->CloneDataObject(type,from,root,copy);

        if (data!=NULL)
            return data;
    }
    return NULL;
}

//------------------------------------------------------------------------------
void CIwASDApp::GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<wxString>& names,int size)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        m_Modules[i]->GetFileTypeInfo(list,types,names,size);
    }
}

//------------------------------------------------------------------------------
CIwLayoutElement* CIwASDApp::MakeElement(const wxString& type)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        CIwLayoutElement* res=m_Modules[i]->MakeElement(type);
        if (res!=NULL)
        {
            res->m_Type=type;
            return res;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
CIwModule* CIwASDApp::FindTagsModule(unsigned int tag)
{
    if (tag==UNKNOWN_TAG) return NULL;

    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        if (m_Modules[i]->m_ModuleNum==GET_MODULE_FROM_TAG(tag))
            return m_Modules[i];
    }

    return NULL;
}

//------------------------------------------------------------------------------
void CIwASDApp::AddAction(const wxString& name,CIwAction* action)
{
    if (m_Actions.find(name)!=m_Actions.end())
        m_Actions[name]->DeleteThis();

    m_Actions[name]=action;
}

//------------------------------------------------------------------------------
void CIwASDApp::DeleteAction(const wxString& name)
{
    if (m_Actions.find(name)==m_Actions.end())
        return;

    m_Actions[name]->DeleteThis();
    m_Actions.erase(name);
}

//------------------------------------------------------------------------------
CIwAction* CIwASDApp::FindAction(const wxString& name)
{
    if (m_Actions.find(name)==m_Actions.end())
        return NULL;

    return m_Actions[name];
}

//------------------------------------------------------------------------------
void CIwASDApp::SetOpenState(CIwASDData* data,bool opening)
{
    int i;
    if (m_RootData==data)
    {
        if (!opening)
            m_RootData=NULL;

        return;
    }

    for (i=0; i<(int)m_OpenData.size(); i++)
    {
        if (m_OpenData[i]==data)
        {
            if (!opening)
                m_OpenData.erase(m_OpenData.begin()+i);

            break;
        }
    }
    if (opening && i==(int)m_OpenData.size())
        m_OpenData.push_back(data);
}

//------------------------------------------------------------------------------
void CIwASDApp::SetRoot(CIwASDData* data)
{
    for (int i=0; i<(int)m_OpenData.size(); i++)
    {
        if (m_OpenData[i]==data)
        {
            m_OpenData.erase(m_OpenData.begin()+i);
            break;
        }
    }
    m_RootData=data;
}

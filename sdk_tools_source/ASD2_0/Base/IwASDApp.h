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
#ifndef IW_ASD_APP_H
#define IW_ASD_APP_H

#ifdef I3D_OS_WINDOWS
#define EXP_DLL extern "C" __declspec(dllexport)
#else
#define EXP_DLL extern "C" __attribute__((visibility("default")))
#endif

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
// CIwASDApp

//--------------------------------------------------------------------------------
class CIwASDLogWindow : public wxLog
{
    wxTextCtrl *m_Text;
    std::vector<wxString> Strings;
    FILE* m_File;
public:
    static wxString m_PreLogFile;
    std::vector<wxString> m_FileList;
    FastDelegate0<> m_Callback;
public:
    CIwASDLogWindow() : m_Text(NULL),m_File(NULL) {}
    ~CIwASDLogWindow() { if (m_File!=NULL) fclose(m_File); }
    void Setup(wxTextCtrl *Text,const wxString& fileName=L"");
    void Clear() { if (m_Text) m_Text->Clear(); }
    void SetupFileList(FastDelegate0<> callback);
protected:
    virtual void DoLog(wxLogLevel level, const wxChar *szString, time_t t);
    virtual void DoLogString(const wxChar *szString, time_t t);
    virtual void OnFrameDelete(wxFrame *frame)
    {
        wxLog::SetActiveTarget(NULL);
    }
};

//------------------------------------------------------------------------------
class CIwProcessDelegate : public wxProcess
{
protected:
    FastDelegate0<> m_Delegate;
public:
    //example: new CIwProcessDelegate(this,MakeDelegate(this,class::fn));
    CIwProcessDelegate(FastDelegate0<> delegate) : wxProcess(),m_Delegate(delegate) {}
    virtual void OnTerminate(int pid, int status);
};

//------------------------------------------------------------------------------
class CIwProcessLoggingDelegate : public CIwProcessDelegate
{
    class Timer : public wxTimer
    {
        CIwProcessLoggingDelegate* m_Parent;
    public:
        Timer(CIwProcessLoggingDelegate* parent) : m_Parent(parent) {}
        virtual void Notify() { m_Parent->DoPoll(); }
    };
    Timer m_Timer;
public:
    //example: new CIwProcessLoggingDelegate(this,MakeDelegate(this,class::fn));
    CIwProcessLoggingDelegate(FastDelegate0<> delegate) : CIwProcessDelegate(delegate),m_Timer(this) { Redirect(); m_Timer.Start(500); }
    ~CIwProcessLoggingDelegate() { m_Timer.Stop(); }
    void DoPoll();
    void Poll();
};

#define MAX_EXTRA_DATA 32

#ifdef ASD_AS_DLL
EXP_DLL void ASDMain(int argc,char** argv);
#endif

//--------------------------------------------------------------------------------
class CIwASDApp : public CIwWinAppBase
{
public:
    CIwLayoutFrame* m_Frame;
    std::map<wxString,wxString> m_DirList;
    CIwASDLogWindow* m_Log;

    void* m_ExtraData[MAX_EXTRA_DATA];
    std::vector<FastDelegate0<> > m_Defered;
    std::vector<CIwASDData*> m_OpenData;
    CIwASDData* m_RootData;
protected:
    std::vector<CIwModule*> m_Modules;
    std::map<wxString,CIwAction*> m_Actions;
    bool m_StartupProblem;
public:
    CIwASDApp() : CIwWinAppBase(),m_Frame(NULL),m_RootData(NULL),m_StartupProblem(false) {}
    ~CIwASDApp();

    virtual bool OnInit();
    void PostWindowInit();
    void AddModule(unsigned char ID,CIwModule* Module);

    void Exit();

    CIwASDData* MakeDataObject(unsigned int type,const wxString& info=L"");
    CIwASDData* CloneDataObject(unsigned int type,CIwASDData* from,CIwASDData* root,bool copy);
    void GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<wxString>& names,int size);

    CIwModule *FindTagsModule(unsigned int tag);

    void AddAction(const wxString& name,CIwAction* action);
    void DeleteAction(const wxString& name);
    CIwAction* FindAction(const wxString& name);

    void SetOpenState(CIwASDData* data,bool opening);
    CIwASDData* GetRoot() { return m_RootData; }
    void SetRoot(CIwASDData* data);

    CIwLayoutElement* MakeElement(const wxString& type);
    wxString MakeAbsoluteFilename(const wxString& Filename) { return ReplaceOptions(Filename,m_DirList); }
    void SetProjectName(const wxString& name);
    void SetProjectDetails(const wxString& details);

    void OnIdle(wxIdleEvent& event);
    virtual int FilterEvent(wxEvent& event) { if (event.GetEventType()==wxEVT_MOUSE_CAPTURE_LOST) return TRUE; else return -1; }

    virtual void ChangeTimer(bool Add) {}
    virtual void LogProcess(const wxString& Line,const wxString& Name,bool Final) {}
private:
    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
#ifndef NO_CIWAPP
DECLARE_APP(CIwASDApp)
#endif
//todo plugin version of this define
#define CIwTheApp ((CIwASDApp *)wxApp::GetInstance())
#define CIwTheFrame (CIwTheApp->m_Frame)

#endif // !IW_ASD_APP_H

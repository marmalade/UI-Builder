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
#ifndef HOST_H
#define HOST_H

#ifndef _UINT32
typedef unsigned long  int  uint32;
#define _UINT32
#endif

typedef int (*MainFn)(wxDllType hInstance, wxDllType hPrevInstance, const char* lpCmdLine, int nCmdShow);

#ifdef I3D_OS_WINDOWS
typedef LRESULT (CALLBACK *s3eWin32UIProcessMessageFn)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

typedef void(*s3eExtWin32SurfaceAttachFn)(void* handle, uint32 width, uint32 height);
typedef int (*s3eExtGetFn)(const char* extension, void* functions, uint32 functionsLen);
typedef int (*s3eConfigGetIntFn)(const char* group, const char* name, int* value);
typedef void (*s3eDebugOutputStringLogFn)(const char* string);
typedef void (*s3eExtWin32AttachLogFn)(s3eDebugOutputStringLogFn fn);

class CIwHostUIConsoleFrame;

#ifdef I3D_OS_WINDOWS
#define DllExport   __declspec( dllexport )
#else
#define DllExport
#endif
extern "C" DllExport void LinkFromLauncherUI( CIwViewerUI* ui );

class CIwUIEdFrame;

//------------------------------------------------------------------------------
class CIwHost : wxThread
{
public:
    static bool s_Active;

#ifdef I3D_OS_WINDOWS
    HWND m_HWND;
#endif

    wxString m_Cwd;
    wxString m_CmdLine;
    wxString m_ViaFile;
    wxString m_AltGroupFile;
    wxString m_LibFile;
    int m_CmdShow;
    bool m_Restarting;
    int m_KeyDown;

#ifdef I3D_OS_WINDOWS
    s3eWin32UIProcessMessageFn m_ProcessFn;
#endif
    s3eExtWin32SurfaceAttachFn m_AttachFn;
    s3eExtWin32AttachLogFn m_AttachLogFn;

    CIwViewerUI* m_Link;

    CUIEdAttrShared m_Shared;
    CIwUIEdFrame* m_Frame;
    bool m_ShowAssertBox;
    wxSemaphore m_ShutdownSemaphore;
    bool m_InViewer;
    std::vector<wxString> m_SelList;

public:
    CIwHost();
    ~CIwHost();
    void Setup(const wxString& viaFile,const wxString& group,const wxString& dir,CIwUIEdFrame* frame);
    void SetupVia(const wxString& viaFile,const wxString& group,const wxString& dir);
    void Start();
    void Run();
    bool End(bool restart);
    wxThread::ExitCode Entry();

    //void RestartNow();

    void ReplaceViaLine(const wxString& starting, const wxString& newline);

#ifdef I3D_OS_WINDOWS
    int ProcessMessage(UINT uMsg,UINT wParam,UINT lParam);
#endif

    void SetAssertBox(bool value);
    bool GetAssertBox() { return m_ShowAssertBox; }
    wxString ChooseBestFile(wxString a,wxString b);
    bool StartsWith(const wxString& fileName,const wxString& dir);
};

#endif

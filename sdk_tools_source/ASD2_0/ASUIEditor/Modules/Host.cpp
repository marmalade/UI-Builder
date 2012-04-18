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
#include "IwUIEdHeader.h"

bool CIwHost::s_Active=false;

//------------------------------------------------------------------------------
#ifdef I3D_OS_WINDOWS
CIwHost::CIwHost() : m_HWND(NULL),m_CmdShow(SW_SHOWNORMAL),m_Frame(NULL),m_KeyDown(0),
    m_AttachLogFn(NULL),m_AttachFn(NULL),m_Link(NULL),m_Shared(this),m_Restarting(true),m_ShutdownSemaphore(0,1),m_InViewer(true),wxThread(wxTHREAD_JOINABLE)
#else
CIwHost::CIwHost() : m_CmdShow(0),m_Restarting(true),m_KeyDown(0),
    m_AttachFn(NULL),m_AttachLogFn(NULL),m_Link(NULL),m_Shared(this),m_Frame(NULL),m_ShutdownSemaphore(0,1),m_InViewer(true),wxThread(wxTHREAD_JOINABLE)
#endif
{
    s_Active=true;
    m_Cwd=wxGetCwd();

    wxString Data;
    m_ShowAssertBox=false;
    Create();
}

//------------------------------------------------------------------------------
CIwHost::~CIwHost()
{
    s_Active=false;
}

//------------------------------------------------------------------------------
bool CIwHost::StartsWith(const wxString& f,const wxString& d)
{
    wxString mainDir=d;
    wxString file=f;
    mainDir.MakeLower().Replace(L"/",L"\\");
    file.MakeLower().Replace(L"/",L"\\");

    return file.StartsWith(mainDir);
}

//------------------------------------------------------------------------------
void CIwHost::SetupVia(const wxString& viaFile,const wxString& group,const wxString& dir)
{
    wxString ViewerDir;
    wxString ViewerPath;
    wxString Path;
    wxString Data2;
    wxTextFile fp(viaFile);

    // For the moment, ignore 'dll_gl' setting and use relative path
    ViewerDir.Printf(L"%s/data", wxGetCwd().c_str());
    ViewerPath.Printf(L"%s/Viewer/viewer.s86", ViewerDir.BeforeLast('/').c_str());

    // Use .s86 specified in GlobalShared.txt
    // Project.Get(L"dll_gl", ViewerPath);

    //wxLogMessage(L"Running: %s with options:",Buffer);
    if (!ViewerPath.Right(3).IsSameAs(L"exe",false))
        fp.AddLine(wxString::Format(L"-dll: \"%s\"",ViewerPath.c_str()));

    fp.AddLine(wxString::Format(L"-data: \"%s\"",ViewerDir.c_str()));

    fp.AddLine(wxString::Format(L"-viewerpath:\"%s\"",dir.c_str()));

    Path.Printf(L"%s/app.icf",ViewerDir.c_str());
    fp.AddLine(wxString::Format(L"-app-icf1:\"%s\"",Path.c_str()));

    fp.AddLine(wxString::Format(L" -checks3edir:\"0\" "));
    fp.AddLine(L" -SurfaceZoomX:1.0 -SurfaceZoomY:1.0 ");
    fp.AddLine(L"-BacklightTimeout:0");

    if (!m_Shared.m_Project.m_ResTemplate.empty())
        fp.AddLine(wxString::Format(L"-restemplate:\"%s\"",m_Shared.m_Project.m_ResTemplate.c_str()));

    int HWSWMode=m_Shared.m_Project.m_HWSWMode;

    if (HWSWMode==0 || HWSWMode==5)
        fp.AddLine(wxString::Format(L"--gx:Transform=HW --gx:Lighting=HW --gx:Rasterisation=HW "));
    else if (HWSWMode==2)
        fp.AddLine(wxString::Format(L"--gx:Transform=HWSW --gx:Lighting=HWSW --gx:Rasterisation=HWSW "));
    else
        fp.AddLine(wxString::Format(L"--gx:Transform=SW --gx:Lighting=SW --gx:Rasterisation=SW "));

    // fix gl offscreen problem
    fp.AddLine(wxString::Format(L"--GLUseOffscreenBuffer=false"));

    if (HWSWMode==3)
        fp.AddLine(L" -scanline:\"true\" ");

    if (HWSWMode==4)
        fp.AddLine(L" -zbuffer:\"true\" ");

    if (HWSWMode==5)
        fp.AddLine(L" -GLAPI:\"GLES 2.0 from Imagination POWERVR(TM)\" ");
    else if (HWSWMode==0 || HWSWMode==2)
        fp.AddLine(L" -GLAPI:\"GLES 1.1 Common Profile from Imagination POWERVR(TM)\" ");

    fp.AddLine(L"-gui");
    fp.AddLine(L"--noskin=1");

    wxSize Size(m_Shared.m_Project.m_Width,m_Shared.m_Project.m_Height);
    fp.AddLine(wxString::Format(L"-SurfaceWidth:%d -SurfaceHeight:%d",Size.x,Size.y));

    fp.AddLine(wxString::Format(L"-viewerfile:\"%s\"",group.c_str()));

    Data2=m_Shared.m_Project.m_Palette.m_FileName.Mid(m_Shared.m_Project.m_RootDir.size());
    fp.AddLine(wxString::Format(L"-palettefile:\"%s\"",Data2.c_str()));

    fp.Write();
}
//------------------------------------------------------------------------------
wxString CIwHost::ChooseBestFile(wxString a,wxString b)
{
    wxFileName fa(a);
    wxFileName fb(b);
    fa.MakeAbsolute();
    fb.MakeAbsolute();

    a=fa.GetFullPath();
    b=fb.GetFullPath();

    bool hasA=wxFileExists(a);
    bool hasB=wxFileExists(b);

    if (!hasA && !hasB)
        return a;

    if (hasA && !hasB)
        return a;

    if (!hasA && hasB)
        return b;

    if (wxFileModificationTime(a)>=wxFileModificationTime(b))
        return a;
    else
        return b;
}

//------------------------------------------------------------------------------
void CIwHost::Setup(const wxString& viaFile,const wxString& group,const wxString& dir,CIwUIEdFrame* frame)
{
    if (!m_InViewer) return;

    wxString s3eDir;
    wxGetEnv(L"S3E_DIR",&s3eDir);

    m_Frame=frame;

    m_ViaFile=viaFile;
    m_CmdLine=wxString::Format(L"-via:\"%s\"",viaFile.c_str());
#ifdef I3D_OS_WINDOWS
    m_HWND=(HWND)m_Frame->GetHWND();
#endif
    int num=0;

    if (!m_Shared.m_Project.m_GroupData->SaveFiltered(m_AltGroupFile,num))
        SetupVia(viaFile,group,dir);
    else
        SetupVia(viaFile,m_AltGroupFile,dir);

    m_LibFile.Printf(L"%s/Viewer/s3e_simulator_debug.dll", wxGetCwd().c_str());
    if (!wxFileExists(m_LibFile))
        m_LibFile=s3eDir+L"/win32/s3e_simulator_debug.dll";

    if (m_Shared.m_Project.m_Width%4) m_Shared.m_Project.m_Width=m_Shared.m_Project.m_Width-(m_Shared.m_Project.m_Width%4)+4;
}

//------------------------------------------------------------------------------
wxThread::ExitCode CIwHost::Entry()
{
    Run();

    return 0;
}

//------------------------------------------------------------------------------
bool CIwHost::End(bool restart)
{
    if (m_Link==NULL)
        return true;

    if (!m_InViewer)
        return false;

    m_Restarting=restart;

    SetupVia(m_ViaFile,m_Shared.m_Project.m_Group,m_Shared.m_Project.m_RootDir);

    m_Link->SetData(CIwViewerUI::SHUTDOWN);
    m_Link=NULL;

    return true;
}

//------------------------------------------------------------------------------
void CIwHost::Start()
{
    wxThread::Run();
}

//------------------------------------------------------------------------------
void LogFn(const char* message)
{
    wxString str(message,wxConvUTF8);
    if (str.empty()) return;

    if (str[str.size()-1]=='\r' || str[str.size()-1]=='\n')
        str.RemoveLast();

    wxLogMessage(str);
}

//------------------------------------------------------------------------------
bool LogErrorFn(const char* message)
{
    if (CIwTheApp && CIwTheApp->m_ExtraData[UIED_EXTRA_DATA_ID])
    {
        bool showAssertBox = CIwTheHost.m_ShowAssertBox;

        wxString str(message,wxConvUTF8);
        if (str.empty())
            return CIwTheHost.m_ShowAssertBox;

        if (str[str.size()-1]=='\r' || str[str.size()-1]=='\n')
            str.RemoveLast();

        wxLogError(str);

        return showAssertBox;
    }

    return false;
}

//------------------------------------------------------------------------------
void CIwHost::SetAssertBox(bool value)
{
    m_ShowAssertBox=value;
    if (value)
        Project.SetOrAdd(L"assertbox",L"true",false,false);
    else
        Project.SetOrAdd(L"assertbox",L"false",false,false);

    Project.Save(false);
}

//------------------------------------------------------------------------------
void ConsoleLogFn(const char* message)
{
    wxString str(message,wxConvUTF8);
    if (str.empty()) return;

    if (str[str.size()-1]=='\r' || str[str.size()-1]=='\n')
        str.RemoveLast();

    wxLogMessage(str);
}

//------------------------------------------------------------------------------
void RecvFn(const char* message)
{
    wxString str(message,wxConvUTF8);
    if (str.empty())
        return;

    if (str.IsSameAs(L"started"))
        wxLogMessage(L"Loaded Application");
}

//------------------------------------------------------------------------------
void CIwHost::Run()
{
    wxDynamicLibrary Lib;
    MainFn EntryPoint;

    while (m_Restarting)
    {
        m_Restarting=false;

        if (m_LibFile.empty())
            return;

        Lib.Load(m_LibFile);
        if (!Lib.IsLoaded())
            return;

        if (!Lib.HasSymbol(L"Main"))
            return;

        EntryPoint=(MainFn)Lib.GetSymbol(L"Main");

        if (Lib.HasSymbol(L"s3eExtGet"))
        {
            s3eExtGetFn pGetFn = (s3eExtGetFn)Lib.GetSymbol(L"s3eExtGet");

#ifdef I3D_OS_WINDOWS
            if (m_HWND!=NULL)
            {
                m_Frame->SetMinSize(wxSize(m_Shared.m_Project.m_Width,m_Shared.m_Project.m_Height));
                m_Frame->SetSize(m_Shared.m_Project.m_Width,m_Shared.m_Project.m_Height);
                m_Frame->GetParent()->GetSizer()->Layout();
                pGetFn("Win32Surface",&m_AttachFn,sizeof(m_AttachFn));
                pGetFn("Win32UIProcessMessage",&m_ProcessFn,sizeof(m_ProcessFn));

                if (m_AttachFn)
                    m_AttachFn(m_HWND,m_Shared.m_Project.m_Width,m_Shared.m_Project.m_Height);
            }

            pGetFn("Win32AttachLog",&m_AttachLogFn,sizeof(m_AttachLogFn));

            if (m_AttachLogFn)
                m_AttachLogFn(LogFn);

#endif
        }

        m_InViewer=false;
        EntryPoint(Lib.GetLibHandle(),NULL,m_CmdLine.mb_str(),m_CmdShow);
#ifdef I3D_OS_WINDOWS
        InvalidateRect(m_HWND,NULL,TRUE);
#endif
        Lib.Unload();
    }
}

//ATOM                MyRegisterClass(HINSTANCE hInstance);
//BOOL                InitInstance(HINSTANCE, int);
//LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

/*
   BOOL RestartProcess(const wxString& cmdline)
   {
    wxChar szAppPath[MAX_PATH] = {0};
    ::GetModuleFileName(NULL, szAppPath, MAX_PATH);
    STARTUPINFO                si = {0};
    PROCESS_INFORMATION        pi = {0};
    si.cb = sizeof(STARTUPINFO);
    wxStrcat(szAppPath, L" -via:\"C:\\temp\\data\\_viewertemp.via\" ");
    if(cmdline)
        wxStrcat(szAppPath, cmdline.c_str());
    return CreateProcess(NULL, szAppPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
   }

   void RestartNow(const wxString& cmdline)
   {
    if(RestartProcess(cmdline))
        exit(0);
   }*/

//------------------------------------------------------------------------------
void CIwHost::ReplaceViaLine(const wxString& starting, const wxString& newline)
{
    // Save back viewer size in via file
    wxTextFile fp(m_ViaFile);
    if (!fp.Exists() || !fp.Open())
        return;

    bool found=false;
    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        int offset=fp[i].Find(starting);
        if (offset!=wxNOT_FOUND)
        {
            fp[i]=fp[i].Left(offset)+newline;
            found=true;
        }
    }
    if (!found)
        fp.AddLine(newline);

    fp.Write();
}

//------------------------------------------------------------------------------
#ifdef I3D_OS_WINDOWS
int CIwHost::ProcessMessage(UINT uMsg, UINT wParam, UINT lParam)
{
    if (m_ProcessFn)
        return m_ProcessFn(m_HWND,uMsg,wParam,lParam);

    return 1;
}

//CIwHostLink* CIwHost::s_Link=NULL;

//--------------------------------------------------------------------------------
BOOL CALLBACK FindGameWindow(HWND hwnd,HWND* prevWindow)
{
    wxChar Line[256];

    GetWindowText(hwnd,Line,256);
    wxString Str(Line);

    if (Str.Contains(L"- x86 Simulator"))
    {
        (*prevWindow)=hwnd;
        return FALSE;
    }

    return TRUE;
}

//------------------------------------------------------------------------------
int IwUIProcessMessage(UINT uMsg, const char* text)
{
    int wParam,lParam;
    sscanf(text,"%d|%d",&wParam,&lParam);
    return CIwTheHost.ProcessMessage(uMsg,wParam,lParam);
}
#endif
//------------------------------------------------------------------------------
int IwViewerProcessMessage(UINT uMsg, const char* line)
{
    if (!CIwHost::s_Active) return -1;

    if (uMsg==CIwViewerUI::CREATE_SELECTION)
    {
        CIwTheHost.m_Shared.m_Project.SyncFlags(false);
        CIwTheHost.m_Shared.m_Project.SyncLocalisation();
        CIwTheHost.m_Shared.m_Project.SyncStyleSheet();

        CIwTheHost.m_Shared.SetSelection(NULL,SELSOURCE_START|SELSOURCE_SCHEDULE);

        CIwTheHost.m_InViewer=true;

        wxLogMessage(L"Ready.");
    }
    else if (uMsg==CIwViewerUI::IS_DEAD)
    {
        CIwTheHost.m_ShutdownSemaphore.Post();
        CIwTheHost.m_Link = NULL;
        //wxLogMessage(L"View Closed.");
    }
    else if (uMsg==CIwViewerUI::ADDED_ELEMENT)
    {
        short pos[2];
        char* argv[32];
        SuperSplit((char*)line,argv,"|");
        pos[0]=atoi(argv[1]);
        pos[1]=atoi(argv[2]);

        CIwAttrInstance* inst=CIwTheHost.m_Shared.m_DragInst;
        CIwAttrInstance* parent=CIwTheHost.m_Shared.m_CurrUI->m_Group.m_Inst;
        if (inst==NULL) return 1;

        bool foundName=false,foundPos=false;
        std::vector<CIwAttrNote> notes;
        inst->FillNotes(notes);
        for (int i=0; i<(int)notes.size(); i++)
        {
            if (notes[i].m_Name.IsSameAs(L"name",false))
            {
                notes[i].m_Data=wxString(argv[0],wxConvUTF8);
                foundName=true;
            }

            if (notes[i].m_Name.IsSameAs(L"pos",false))
            {
                notes[i].m_Data=wxString::Format(L"{ %d %d }",pos[0],pos[1]);
                foundPos=true;
            }
        }

        if (!foundName)
        {
            CIwAttrNote note;
            note.m_Name=L"name";
            note.m_Data=wxString(argv[0],wxConvUTF8);
            note.m_Info=note.m_ID=0;
            notes.push_back(note);
        }

        if (!foundPos)
        {
            CIwAttrNote note;
            note.m_Name=L"pos";
            note.m_Data=wxString::Format(L"{ %d %d }",pos[0],pos[1]);
            note.m_Info=note.m_ID=0;
            notes.push_back(note);
        }

        inst=CIwTheFileMetaMgr.GetFromNotes(notes,EXPORTTYPE_GUI,true);

        CIwAttrData* data=new CIwAttrData;
        data->m_Mgr=parent->m_Mgr;
        data->m_Instance=parent;
        data->m_Member=parent->m_Class->GetClassMember(inst->m_Class);
        data->m_Info=-1;
        data->m_Group=NULL;
        data->SetDefault();
        parent->m_Data.push_back(data);
        inst->m_Parent=data;

        data->m_Items.resize(1);
        data->m_Items[0].m_Flags=ATTRITEM_ALLOCED_F;
        data->m_Items[0].m_Inst=inst;

        CIwTheHost.m_Shared.SetSelected(inst,true);
    }
    else if (uMsg==CIwViewerUI::MOVED_ELEMENT)
    {
        short move[4];
        char* argv[32];
        SuperSplit((char*)line,argv,"|");
        move[0]=atoi(argv[1]);
        move[1]=atoi(argv[2]);
        move[2]=atoi(argv[3]);
        move[3]=atoi(argv[4]);

        CIwAttrInstance* inst=CIwTheHost.m_Shared.m_CurrUI->m_Group.m_Inst;
        if (inst!=NULL)
        {
            inst=inst->FindChild(wxString(argv[0],wxConvUTF8));
            if (inst!=NULL)
            {
                CIwAttrData* pos=inst->FindData(L"pos");
                CIwAttrData* size=inst->FindData(L"size");

                if (pos!=NULL)
                {
                    pos->m_Items[0].m_Int=move[0];
                    pos->m_Items[1].m_Int=move[1];
                }

                if (size!=NULL)
                {
                    size->m_Items[0].m_Int=move[2];
                    size->m_Items[1].m_Int=move[3];
                }

                if (inst!=NULL && CIwTheHost.m_Shared.m_SelElem==inst)
                    CIwTheHost.m_Shared.SetSelected(inst,false);
            }
        }
    }
    else if (uMsg==CIwViewerUI::SELECT_ELEMENT)
    {
        wxString str=wxString(line,wxConvUTF8);

        CIwAttrInstance* inst=CIwTheHost.m_Shared.GetFromFullName(str);
        if (inst!=NULL)
            CIwTheHost.m_Shared.SetSelection(inst,SELSOURCE_VIEWER|SELSOURCE_SCHEDULE);
    }
    else if (uMsg==CIwViewerUI::UPDATE_POS && CIwTheHost.m_Frame!=NULL && CIwTheHost.m_Frame->m_Focussed)
    {
        wxPoint pos=wxGetMousePosition();
        wxMouseState& state=wxGetMouseState();

        if (!CIwHost::s_Active)
            return -1;

        pos=CIwTheHost.m_Frame->ScreenToClient(pos);

        int mode=128;

        if (!CIwTheHost.m_Frame->m_OnDrag)
        {
            if (state.LeftDown())
                mode|=1;

            if (state.MiddleDown())
                mode|=2;

            if (state.RightDown())
                mode|=4;

            if (state.ShiftDown())
                mode|=8;

            if (state.ControlDown())
                mode|=16;

            if (state.MetaDown())
                mode|=32;
        }

        char text[256];
        sprintf(text,"%d|%d|%d|%d",mode,pos.x,pos.y,CIwTheHost.m_KeyDown);
        CIwTheHost.m_Link->SetData(CIwViewerUI::MOUSE_POS,text);
    }
    else if (uMsg==CIwViewerUI::UPDATED_ELEMENTS)
    {
        char* argv[32];
        if (SuperSplit((char*)line,argv,"|")>1)
        {
            wxString name=wxString(argv[0],wxConvLibc);
            wxString file=wxString(argv[1],wxConvLibc);

            CIwTheHost.m_Shared.UpdateFromViewer(name,file,CIwTheHost.m_SelList);
            CIwTheHost.m_SelList.clear();
        }
    }
    else if (uMsg==CIwViewerUI::ADD_SELECTION)
        CIwTheHost.m_SelList.push_back(wxString(line,wxConvUTF8));

    return 1;
}

//------------------------------------------------------------------------------
extern "C" DllExport void LinkFromLauncherUI( CIwViewerUI* ui )
{
    CIwTheHost.m_Link=ui;
    if (CIwTheHost.m_Link)
    {
        wxLogMessage(L"Loading...");
#ifdef I3D_OS_WINDOWS
        CIwTheHost.m_Link->m_UIMessage        = IwUIProcessMessage; // Sending windows commands to s86
        CIwTheHost.m_Link->m_ViewerMessage    = IwViewerProcessMessage; // Receiving commands from s86
#endif
        ui->m_LogFn            = LogFn;
        ui->m_LogErrorFn    = LogErrorFn;
    }
}

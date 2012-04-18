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
#ifndef TOOLS_HEADER_H
#define TOOLS_HEADER_H

#ifdef _MSC_VER
#define  _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#ifndef I3D_ARCH_AMD64
#define _USE_32BIT_TIME_T
#endif

#include <wx/wxprec.h>

#ifndef _CRT_SECURE_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif

#pragma warning ( disable : 4786 )
#pragma warning ( push,1 )
#endif

#include <string>
#include <deque>
#include <vector>
#include <set>
#include <algorithm>
#include <list>
#include <map>
#include <float.h>

#ifdef _MSC_VER
#pragma warning ( pop )

//#include <winsock2.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include <io.h>
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/treectrl.h"
#include "wx/image.h"
#include "wx/bitmap.h"
#include "wx/toolbar.h"
#include "wx/process.h"
#include "wx/listctrl.h"
#include "wx/laywin.h"
#include "wx/grid.h"
#include "wx/dataobj.h"
#include "wx/dnd.h"
#include "wx/datetime.h"
#include "wx/tglbtn.h"
#include "wx/dynlib.h"
#include "wx/stc/stc.h"
#include "wx/colordlg.h"
#include "wx/splash.h"
#include "wx/colordlg.h"
#include "wx/gbsizer.h"
#include "wx/html/htmlwin.h"
#include "wx/statline.h"
#include "wx/progdlg.h"
#include "wx/tooltip.h"
#include "wx/notebook.h"
#include "wx/textfile.h"
#include "wx/dir.h"
#include "wx/txtstrm.h"
#include "wx/imaglist.h"

#ifdef _MSC_VER
#include "wx/msw/registry.h"
#endif

#ifndef I3D_OS_WINDOWS
#define stricmp strcasecmp
#define strnicmp strncasecmp

#define isnanf(x) isnan(x)
#define _access(x,y) access(x,y)
#endif

#include "wx/stdpaths.h"
#include "wx/spinctrl.h"
#include "wx/fontdlg.h"
#include "wx/richtext/richtextctrl.h"
#include "wx/splitter.h"
#include "wx/ffile.h"

int SuperSplit(const wxString& s,std::vector<wxString>& args,const wxString& list,char quote='\"');
int SuperSplit(char *line,char *argv[],const char list[]);

//Supersplit without " detection
int Split(const wxString& s,std::vector<wxString>& args,const wxString& list);

//--------------------------------------------------------------------------------
struct ReplaceOptionsCallback
{
    virtual ~ReplaceOptionsCallback() {}
    virtual wxString Replace(wxString tag)=0;
};

struct MapReplaceOptionsCallback : public ReplaceOptionsCallback
{
    std::map<wxString,wxString>& m_Options;

    MapReplaceOptionsCallback(std::map<wxString,wxString>& options) : m_Options(options) {}
    virtual wxString Replace(wxString tag);
};

wxString ReplaceOptions(const wxString& Input,ReplaceOptionsCallback& callback);
wxString ReplaceOptions(const wxString& Input,std::map<wxString,wxString>& Options);

// bring the currnet app to the front/focus
void BringToFront();

//call instead of wxInitAllImageHandlers for correct tga ordering
void InitAllImageHandlers();

//std::string to wxString
#define SSTOWXS(x) (wxString((x).c_str(),wxConvUTF8))
#define WXSTOSS(x) (std::string((x).mb_str()))

//--------------------------------------------------------------------------------
class CIwTextFile : public wxTextFile {
    static std::vector<wxString> m_Flags;
public:
    static void AddFlag(const wxString& flag) { m_Flags.push_back(flag); }
    static void ClearFlags() { m_Flags.clear(); }
public:
    CIwTextFile() { }
    CIwTextFile(const wxString& strFileName) : wxTextFile(strFileName) {}
protected:
    virtual bool OnRead(const wxMBConv& conv);
};

wxString IwGetS3EDir(wxString root=L"");
wxString IwGetSharedResourceDir();


#endif

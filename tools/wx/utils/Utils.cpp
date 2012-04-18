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

#ifdef __WXMAC__
#include "Carbon/Carbon.h"
#endif

void BringToFront()
{
    #ifdef __WXMAC__
//    ProcessSerialNumber process;
//    if (GetCurrentProcess(&process) == noErr)
//              SetFrontProcess(&process);
    #endif
}

//--------------------------------------------------------------------------------
int Split(const wxString& s,std::vector<wxString>& args,const wxString& list)
{
    wxString temp;
    int mode=0;
    args.clear();

    bool trans=false;
    for (int i=0;i<(int)s.size();i++)
    {
        bool end=false,force=false;
        switch(mode)
        {
            case 0:
                if (s[i]==wxT('_'))
                {
                    mode=4;
                    temp.append(1,wxT('_'));
                } else if (list.Find(s[i])!=-1)
                {
                    end=true;
                    mode=3;
                } else
                    temp.append(s[i]);
                break;
            case 3:
                if (s[i]==wxT('_'))
                {
                    mode=4;
                    temp.append(1,wxT('_'));
                } else if (list.Find(s[i])==-1)
                {
                    temp.append(s[i]);
                    mode=0;
                }
                break;
            case 4:
                if (list.Find(s[i])!=-1)
                {
                    end=true;
                    mode=3;
                } else {
                    temp.append(s[i]);
                    mode=0;
                }
                break;

        }
        if ((end && !temp.empty()) || force)
        {
            if (trans)
                args.push_back(wxGetTranslation(temp));
            else
                args.push_back(temp);
            temp.clear();
            trans=false;
        }
    }
    if (!temp.empty())
    {
        if (trans)
            args.push_back(wxGetTranslation(temp));
        else
            args.push_back(temp);
    }

    return args.size();
}

//--------------------------------------------------------------------------------
int SuperSplit(const wxString& s,std::vector<wxString>& args,const wxString& list,char quote)
{
    wxString temp;
    int mode=0;
    args.clear();

    bool trans=false;
    for (int i=0;i<(int)s.size();i++)
    {
        bool end=false,force=false;
        switch(mode)
        {
            case 0:
                if (s[i]==wxT('_'))
                {
                    mode=4;
                    temp.append(1,wxT('_'));
                } else if (s[i]==quote)
                    mode=1;
                else if (list.Find(s[i])!=-1)
                {
                    end=true;
                    mode=3;
                } else
                    temp.append(s[i]);
                break;
            case 1:
                if (s[i]==wxT('\\'))
                    mode=2;
                else if (s[i]==quote)
                {
                    force=true;
                    mode=3;
                } else
                    temp.append(s[i]);
                break;
            case 2:
                if (s[i]!=quote)
                    temp.append(1,wxT('\\'));
                temp.append(s[i]);
                mode=1;
                break;
            case 3:
                if (s[i]==wxT('_'))
                {
                    mode=4;
                    temp.append(1,wxT('_'));
                } else if (s[i]==quote)
                    mode=1;
                else if (list.Find(s[i])==-1)
                {
                    temp.append(s[i]);
                    mode=0;
                }
                break;
            case 4:
                if (s[i]==quote)
                {
                    temp.clear();
                    mode=1;
                    trans=true;
                } else {
                    if (list.Find(s[i])!=-1)
                    {
                        end=true;
                        mode=3;
                    } else {
                        temp.append(s[i]);
                        mode=0;
                    }
                }
                break;

        }
        if ((end && !temp.empty()) || force)
        {
            if (trans)
                args.push_back(wxGetTranslation(temp));
            else
                args.push_back(temp);
            temp.clear();
            trans=false;
        }
    }
    if (!temp.empty())
    {
        if (trans)
            args.push_back(wxGetTranslation(temp));
        else
            args.push_back(temp);
    }

    return args.size();
}
//--------------------------------------------------------------------------------
wxString MapReplaceOptionsCallback::Replace(wxString tag)
{
    char repchar=0;

    if (tag[0]=='/' || tag[0]=='\\' || tag[0]=='\"' || tag[0]=='%' || tag[0]=='\'')
    {
        repchar=tag[0];
        tag=tag.Mid(1);
    }
    wxString Value;
    if (m_Options.find(tag)!=m_Options.end())
        Value=ReplaceOptions(m_Options[tag],*this);

    int j;
    wxString Value2;

    switch(repchar)
    {
        case '/':
            for (j=0;j<(int)Value.size();j++)
                if (Value[j]=='\\')
                    Value[j]='/';
            break;
        case '\\':
            for (j=0;j<(int)Value.size();j++)
                if (Value[j]=='/')
                    Value[j]='\\';
            break;
        case '\"':
            for (j=0;j<(int)Value.size();j++)
            {
                if (j<(int)Value.size()-1 && Value[j]=='\\' && Value[j+1]=='\\')
                    Value2.append(wxT("\\"));
                else if (Value[j]=='\\')
                    Value2.append(wxT("\\\\"));
                else if (Value[j]=='\"')
                    Value2.append(wxT("\\\""));
                else
                    Value2.append(1,Value[j]);
            }
            Value=Value2;
            break;
        case '%':
            wxGetEnv(tag,&Value);
            break;
        case '\'':
            Value=wxString::Format(wxT("\"%s\""),Value.c_str());
            break;
    }
    return Value;
}

//--------------------------------------------------------------------------------
wxString ReplaceOptions(const wxString& Input,ReplaceOptionsCallback& callback)
{
    wxString Output;
    wxString Tag;

    int Mode=0;
    for (int i=0;i<(int)Input.size();i++)
    {
        switch(Mode)
        {
            case 0:
                if (Input[i]=='{')
                {
                    Mode=1;
                    Tag=wxT("");
                } else
                    Output.append(1,Input[i]);
                break;
            case 1:
                if (Input[i]=='}')
                {
                    Output.append(callback.Replace(Tag));
                    Mode=0;
                } else
                    Tag.append(1,Input[i]);
                break;
        }
    }
    return Output;
}

/*
//--------------------------------------------------------------------------------
wxString ReplaceOptions(const wxString Input,std::map<wxString,wxString>& Options)
{
    MapReplaceOptionsCallback callback(Options);
    return ReplaceOptions(Input,callback);
}
/*/
//--------------------------------------------------------------------------------
wxString ReplaceOptions(const wxString& Input,std::map<wxString,wxString>& Options)
{
    wxString Output;
    wxString Tag;

    int Mode=0;
    int RepChar=0;
    for (int i=0;i<(int)Input.size();i++)
    {
        switch(Mode)
        {
            case 0:
                if (Input[i]=='{')
                {
                    Mode=1;
                    Tag=wxT("");
                    RepChar=0;
                } else
                    Output.append(1,Input[i]);
                break;
            case 1:
                if (Input[i]=='}')
                {
                    if (Options.find(Tag)!=Options.end())
                        Output.append(ReplaceOptions(Options[Tag],Options));
                    Mode=0;
                } else if (Input[i]=='/')
                {
                    RepChar='/';
                    Mode=2;
                } else if (Input[i]=='\\')
                {
                    RepChar='\\';
                    Mode=2;
                } else if (Input[i]=='\"')
                {
                    RepChar='\"';
                    Mode=2;
                } else if (Input[i]=='%')
                {
                    RepChar='%';
                    Mode=2;
                } else if (Input[i]=='\'')
                {
                    RepChar='\'';
                    Mode=2;
                } else {
                    Tag.append(1,Input[i]);
                    Mode=2;
                }
                break;
            case 2:
                if (Input[i]=='}')
                {
                    if (Options.find(Tag)!=Options.end())
                    {
                        wxString Value = ReplaceOptions(Options[Tag],Options);

                        if (RepChar=='\\')
                        {
                            for (int j=0;j<(int)Value.size();j++)
                                if (Value[j]=='/')
                                    Value[j]='\\';
                        }
                        if (RepChar=='/')
                        {
                            for (int j=0;j<(int)Value.size();j++)
                                if (Value[j]=='\\')
                                    Value[j]='/';
                        }
                        if (RepChar=='\"')
                        {
                            wxString Value2;
                            for (int j=0;j<(int)Value.size();j++)
                            {
                                if (j<(int)Value.size()-1 && Value[j]=='\\' && Value[j+1]=='\\')
                                    Value2.append(wxT("\\"));
                                else if (Value[j]=='\\')
                                    Value2.append(wxT("\\\\"));
                                else if (Value[j]=='\"')
                                    Value2.append(wxT("\\\""));
                                else
                                    Value2.append(1,Value[j]);
                            }
                            Value=Value2;
                        }
                        if (RepChar=='\'')
                        {
                            Value=wxString::Format(wxT("\"%s\""),Value.c_str());
                        }
                        Output.append(Value);

                    } else if (RepChar=='%')
                    {
                        wxString Val;
                        wxGetEnv(Tag,&Val);
                        Output.append(Val);
                    }

                    Mode=0;
                } else Tag.append(1,Input[i]);
                break;
        }
    }
    return Output;
}
//*/

//--------------------------------------------------------------------------------
//used by SuperSplit
static void RemoveChars(char *line,int offset,int num,int *len)
{
    int i;
    (*len)-=num;
    for (i=offset;i<(*len);i++)
        line[i]=line[i+num];
    line[i]=0;
}

//--------------------------------------------------------------------------------
//remove chars in list, returns remainder split up (strings surrounded by " are returned as one item)
int SuperSplit(char *line,char *argv[],const char list[])
{
    int i,j,argc=0;
    bool zerod=true,found,string=false,startstring;
    int len=(int)strlen(line);
    int listlen=(int)strlen(list);

    argv[0]=line;

    for (i=0;i<len;i++)
    {
        found=false;
        startstring=false;

        if (string&&(line[i]=='\\'))
        {
            if ((line[i+1]=='\"') || (line[i+1]=='\\'))
                RemoveChars(line,i,1,&len);
        } else {
            if (string)
                while ((line[i]=='"')&&(line[i+1]=='"'))
                    RemoveChars(line,i,2,&len);
            if (line[i]=='"')
                startstring=true;
        }

        if (!string)
            for (j=0;j<listlen;j++)
                if (line[i]==list[j])
                    found=true;
        if (startstring)
        {
            found=true;
            string=!string;
            if (string)
            {
                argv[argc++]=&line[i+1];
                zerod=false;
            }
        }

        if (found&&(!zerod)&&(!string))
        {
            line[i]=0;
            zerod=true;
        } else if ((!found)&&zerod)
        {
            argv[argc++]=&line[i];
            zerod=false;
        }
    }
    return argc;
}

void InitAllImageHandlers()
{
#if wxUSE_LIBPNG
  wxImage::AddHandler( new wxPNGHandler );
#endif
#if wxUSE_LIBJPEG
  wxImage::AddHandler( new wxJPEGHandler );
#endif
#if wxUSE_LIBTIFF
  wxImage::AddHandler( new wxTIFFHandler );
#endif
#if wxUSE_GIF
  wxImage::AddHandler( new wxGIFHandler );
#endif
#if wxUSE_PNM
  wxImage::AddHandler( new wxPNMHandler );
#endif
#if wxUSE_PCX
  wxImage::AddHandler( new wxPCXHandler );
#endif
#if wxUSE_IFF
  wxImage::AddHandler( new wxIFFHandler );
#endif
#if wxUSE_TGA
  wxImage::AddHandler( new wxTGAHandler );
#endif
#if wxUSE_XPM
  wxImage::AddHandler( new wxXPMHandler );
#endif
#if wxUSE_ICO_CUR
  wxImage::AddHandler( new wxICOHandler );
  wxImage::AddHandler( new wxCURHandler );
  wxImage::AddHandler( new wxANIHandler );
#endif
}

std::vector<wxString> CIwTextFile::m_Flags;

//--------------------------------------------------------------------------------
bool CIwTextFile::OnRead(const wxMBConv& conv)
{
    if (!wxTextFile::OnRead(conv))
        return false;
    std::vector<int> mode;

    bool use=true;
    int i,j;
    for (i=0;i<(int)GetLineCount();)
    {
        std::vector<wxString> args;
        SuperSplit(GetLine(i),args,wxT(" \t\n"));

        if (args.size()<1)
            i++;
        else if (args[0][0]=='#')
        {
            if (args[0].IsSameAs(wxT("#ifdef"),false))
            {
                if (!use || args.size()<2)
                    mode.push_back(-1);
                else {
                    for (j=0;j<(int)m_Flags.size();j++)
                        if (m_Flags[j].IsSameAs(args[1],false))
                            break;
                    if (j==(int)m_Flags.size()) {
                        mode.push_back(0);
                        use=false;
                    } else
                        mode.push_back(1);
                }
            } else if (args[0].IsSameAs(wxT("#ifndef"),false))
            {
                if (!use || args.size()<2)
                    mode.push_back(-1);
                else {
                    for (j=0;j<(int)m_Flags.size();j++)
                        if (m_Flags[j].IsSameAs(args[1],false))
                            break;
                    if (j!=(int)m_Flags.size()) {
                        mode.push_back(0);
                        use=false;
                    } else
                        mode.push_back(1);
                }
            } else if (args[0].IsSameAs(wxT("#else"),false))
            {
                if (mode.back()==0)
                {
                    mode.back()=1;
                    use=true;
                } else if (mode.back()==1)
                {
                    mode.back()=0;
                    use=false;
                }
            } else if (args[0].IsSameAs(wxT("#endif"),false))
            {
                mode.pop_back();
                if (mode.empty())
                    use=true;
                else if (mode.back()==0)
                    use=false;
                else if (mode.back()==1)
                    use=true;
            } else {
                i++;
                continue;
            }
            RemoveLine(i);
        } else if (use)
            i++;
        else
            RemoveLine(i);
    }
    return true;
}

wxString m_ResDirCached;
wxString m_SharedResDirCached;

wxString IwGetResourceDir()
{
    if (m_ResDirCached.empty()) {
        #ifdef I3D_OS_LINUX
        m_ResDirCached = wxStandardPaths::Get().GetExecutablePath();
        printf("resdir: %s\n", (const char*)m_ResDirCached.ToUTF8());
        m_ResDirCached = wxFileName(m_ResDirCached).GetPath();
        printf("resdir: %s\n", (const char*)m_ResDirCached.ToUTF8());
        #else
        m_ResDirCached=wxStandardPaths::Get().GetResourcesDir();
        #endif
        if (m_ResDirCached.Last()!='\\' && m_ResDirCached.Last()!='/')
            m_ResDirCached+=L"/";
    }
    return m_ResDirCached;
}

wxString IwGetSharedResourceDir()
{
    if (m_SharedResDirCached.empty()) {
        wxFileName name(IwGetResourceDir(),L"");

        while (name.GetDirCount()>0) {
            if (wxFileName::DirExists(name.GetPath(true)+L"s3e")) {
                name.AppendDir(L"s3e");
                name.AppendDir(L"brd");
                break;
            }
            name.RemoveLastDir();
        }

        if (name.GetDirCount()>0 && wxFileName::DirExists(name.GetFullPath()))
            m_SharedResDirCached=name.GetFullPath();
        else
            m_SharedResDirCached=IwGetResourceDir();
    }
    return m_SharedResDirCached;
}

void IwSetResourceDir(const wxString& dir)
{
    m_ResDirCached=dir;
}

wxString IwGetS3EDir(wxString root)
{
    if (root.empty())
    {
        root=IwGetResourceDir();
    }
    wxFileName name(root,L"");
    name.Normalize(wxPATH_NORM_DOTS);
    while (name.GetDirCount()>0) {
        if (wxFileName::DirExists(name.GetPath(true)+L"s3e")) {
            name.AppendDir(L"s3e");

            return name.GetFullPath();
        }
        name.RemoveLastDir();
    }
    wxString s3eDir;
    wxGetEnv(L"S3E_DIR",&s3eDir);

    return s3eDir;
}


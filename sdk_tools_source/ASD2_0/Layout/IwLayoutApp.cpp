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
//------------------------------------------------------------------------------
/*!
    \file IwLAyoutApp.cpp
 */
//------------------------------------------------------------------------------

#include "IwASDBase.h"

//--------------------------------------------------------------------------------
//used by SuperSplit
void RemoveChars(char *line,int offset,int num,int *len)
{
    int i;
    (*len)-=num;
    for (i=offset; i<(*len); i++)
    {
        line[i]=line[i+num];
    }
    line[i]=0;
}

//--------------------------------------------------------------------------------
//remove chars in list, returns remainder split up (strings surrounded by " are returned as one item)
int SuperSplit(char *line,char *argv[],char list[])
{
    int i,j,argc=0;
    bool zerod=true,found,string=false,startstring;
    int len=(int)strlen(line);
    int listlen=(int)strlen(list);

    argv[0]=line;

    for (i=0; i<len; i++)
    {
        found=false;
        startstring=false;

        if (string&&(line[i]=='\\'))
        {
            if ((line[i+1]=='\"') || (line[i+1]=='\\'))
                RemoveChars(line,i,1,&len);
        }
        else
        {
            if (string)
                while ((line[i]=='"')&&(line[i+1]=='"'))
                    RemoveChars(line,i,2,&len);

            if (line[i]=='"')
                startstring=true;
        }

        if (!string)
            for (j=0; j<listlen; j++)
            {
                if (line[i]==list[j])
                    found=true;
            }

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
        }
        else if ((!found)&&zerod)
        {
            argv[argc++]=&line[i];
            zerod=false;
        }
    }
    return argc;
}

//--------------------------------------------------------------------------------
std::string ReplaceOptions(const char* Input,std::map<std::string,std::string>& Options)
{
    std::string Output;
    std::string Tag;

    int Mode=0;
    int RepChar=0;
    for (int i=0; i<(int)strlen(Input); i++)
    {
        switch (Mode)
        {
        case 0:
            if (Input[i]=='{')
            {
                Mode=1;
                Tag="";
                RepChar=0;
            }
            else
                Output.append(1,Input[i]);

            break;
        case 1:
            if (Input[i]=='}')
            {
                if (Options.find(Tag)!=Options.end())
                    Output.append(ReplaceOptions(Options[Tag].c_str(),Options));

                Mode=0;
            }
            else if (Input[i]=='/')
            {
                RepChar='/';
                Mode=2;
            }
            else if (Input[i]=='\\')
            {
                RepChar='\\';
                Mode=2;
            }
            else if (Input[i]=='\"')
            {
                RepChar='\"';
                Mode=2;
            }
            else if (Input[i]=='%')
            {
                RepChar='%';
                Mode=2;
            }
            else if (Input[i]=='\'')
            {
                RepChar='\'';
                Mode=2;
            }
            else
            {
                Tag.append(1,Input[i]);
                Mode=2;
            }

            break;
        case 2:
            if (Input[i]=='}')
            {
                if (Options.find(Tag)!=Options.end())
                {
                    std::string Value=ReplaceOptions(Options[Tag].c_str(),Options);

                    if (RepChar=='\\')
                    {
                        for (int j=0; j<(int)Value.size(); j++)
                        {
                            if (Value[j]=='/')
                                Value[j]='\\';
                        }
                    }

                    if (RepChar=='/')
                    {
                        for (int j=0; j<(int)Value.size(); j++)
                        {
                            if (Value[j]=='\\')
                                Value[j]='/';
                        }
                    }

                    if (RepChar=='\"')
                    {
                        std::string Value2;
                        for (int j=0; j<(int)Value.size(); j++)
                        {
                            if (Value[j]=='\\')
                                Value2.append("\\\\");
                            else if (Value[j]=='\"')
                                Value2.append("\\\"");
                            else
                                Value2.append(1,Value[j]);
                        }
                        Value=Value2;
                    }

                    if (RepChar=='\'')
                    {
                        char Line[256];
                        sprintf(Line,"\"%s\"",Value.c_str());
                        Value=Line;
                    }

                    Output.append(Value);

                }
                else if (RepChar=='%')
                {
                    wxString Val;
                    wxGetEnv(Tag.c_str(),&Val);
                    Output.append(Val.c_str());
                }

                Mode=0;
            }
            else Tag.append(1,Input[i]);

            break;
        }
    }
    return Output;
}

//--------------------------------------------------------------------------------
//log window has started, add stored logs
void CIwLayoutLogWindow::Setup(wxTextCtrl *Text)
{
    m_Text=Text;
    if (m_Text==NULL) return;

    for (std::vector<wxString>::iterator it=Strings.begin(); it!=Strings.end(); ++it)
    {
        m_Text->AppendText(*it);
    }
    Strings.clear();
}

//--------------------------------------------------------------------------------
//logging from wx (do we log?)
void CIwLayoutLogWindow::DoLog(wxLogLevel level, const wxChar *szString, time_t t)
{
    switch (level)
    {
    case wxLOG_Status:
        if (!wxIsEmpty(szString))
        {
            wxString str;
            str<<"Status: "<<szString;
            DoLogString(str,t);
        }

        break;
    case wxLOG_Trace:
        break;
    case wxLOG_Warning:
        if (m_Text!=NULL)
        {
            // set colour, do the log, set it back
            wxTextAttr existingColour = m_Text->GetDefaultStyle();
            m_Text->SetDefaultStyle( wxTextAttr( wxColour("ORANGE") ) );
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
void CIwLayoutLogWindow::DoLogString(const wxChar *szString, time_t t)
{
    wxString msg;
    TimeStamp(&msg);
    msg<<szString; //<<wxT('\n');

    if (m_Text==NULL)
    {
#ifdef WIN32
        OutputDebugStr(szString);
        OutputDebugStr("\n");
#endif
        Strings.push_back(msg);
        return;
    }

    wxTextPos Len=m_Text->GetLastPosition();
    m_Text->SetSelection(Len,Len);

    m_Text->AppendText(msg);
    m_Text->AppendText("\n");
}

//------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwASDApp, wxApp)
    //	EVT_IDLE(CIwASDApp::OnIdle)
    //	EVT_TIMER(CTRLID_TIMER,CIwASDApp::OnTick)
    //	EVT_TIMER(CTRLID_VIEWERTIMER,CIwASDApp::OnViewTick)
END_EVENT_TABLE()

IMPLEMENT_APP(CIwASDApp)

//------------------------------------------------------------------------------
bool CIwASDApp::OnInit()
{
    char Line[256];

    wxInitAllImageHandlers();

    for (int i=0; i<MAX_EXTRA_DATA; i++)
    {
        m_ExtraData[i]=NULL;
    }

    //setup default path (modules could override)
    if (getcwd(Line,256))
    {
        strcat(Line,"\\data\\");
        m_DirList["viewer"]=Line;
    }

    m_DirList["layout"]="{viewer}VLayoutShared.svl";

    SHGetSpecialFolderPath(NULL,Line,CSIDL_PERSONAL,0);
    strcat(Line,"\\Director\\");
    m_DirList["user"]=Line;
    m_DirList["currproject"]="Layout";

    //start logging
    m_Log=new CIwLayoutLogWindow();
    wxLog::SetActiveTarget(m_Log);

    //add modules
    RegisterModules(this);

    //create main window
    m_Frame=new CIwLayoutFrame();

    //create the frame
    m_Frame->Reset();

    // refresh update here but might work elsewhere
    m_Frame->Show(true);
    m_Frame->Refresh();
    m_Frame->Update();

    return true;
}

//------------------------------------------------------------------------------
CIwASDApp::~CIwASDApp()
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        delete m_Modules[i];
    }
}

//------------------------------------------------------------------------------
void CIwASDApp::SetProjectName(const char* name)
{
    m_DirList["currproject"]=name;

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
CIwASDData* CIwASDApp::MakeDataObject(unsigned int type,const char* info)
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
void CIwASDApp::GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<std::string>& names,int size)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        m_Modules[i]->GetFileTypeInfo(list,types,names,size);
    }
}

//------------------------------------------------------------------------------
unsigned int CIwASDApp::FindTag(const char* tag,unsigned int type)
{
    unsigned int res=UNKNOWN_TAG;
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        int Offset=m_Modules[i]->IsPrefix(tag,type);
        if (Offset==UNKNOWN_TAG) continue;

        res=m_Modules[i]->FindTag(tag+Offset,type);
        if (res!=UNKNOWN_TAG)
            break;
    }
    return res;
}

//------------------------------------------------------------------------------
void CIwASDApp::FillTagNames(wxArrayString& array,unsigned int type,std::vector<unsigned int>& subTypeList)
{
    for (int i=0; i<(int)m_Modules.size(); i++)
    {
        for (int k=0; k<(int)m_Modules[i]->m_TagDefTypes.size(); k++)
        {
            if (m_Modules[i]->m_TagDefTypes[k]==type)
            {
                for (int j=0; m_Modules[i]->m_TagDefs[k][j].m_Name!=NULL; j++)
                {
                    char Line[256];
                    sprintf(Line,"%s%s",m_Modules[i]->m_Prefix.c_str(),m_Modules[i]->m_TagDefs[k][j].m_Name);
                    array.Add(Line);

                    if (m_Modules[i]->GetSubActionType(m_Modules[i]->m_TagDefs[k][j].m_ID,type)!=type)
                        subTypeList.push_back(m_Modules[i]->m_TagDefs[k][j].m_ID);
                }
                break;
            }
        }
    }
}

//------------------------------------------------------------------------------
CIwLayoutElement* CIwASDApp::MakeElement(const char* type)
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

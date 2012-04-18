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
#include "IwASDFileHeader.h"

//--------------------------------------------------------------------------------
// CIwASDFilenameRoots
//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilenameRoots::Get(const wxString& fileName,CIwASDFilename* parent,unsigned char Type,bool DoAdd)
{
    std::vector<wxString> argv;
    std::vector<wxString> argv2;
    int i;

    int argc=SuperSplit(fileName,argv,L"\\/");
    if (argc<1) return NULL;

    for (i=1; i<argc; i++)
    {
        argv2.push_back(argv[i]);
    }

    bool AbsPath=CIwASDFilename::IsAbsPath(fileName);

    if (!AbsPath)
    {
        if (parent==NULL)
            return NULL;

        return parent->Add(argv,Type,DoAdd);
    }

    for (i=0; i<(int)m_Roots.size(); i++)
    {
        if (argv[0].IsSameAs(m_Roots[i]->getName(),false))
            return m_Roots[i]->Add(argv2,Type,DoAdd);
    }

    if (!DoAdd) return NULL;

    CIwASDFilename* file=new CIwASDFilename(argv[0],NULL);
    m_Roots.push_back(file);
    if (argc==1)
    {
        file->IncRef();
        file->m_Type=Type;
        return file;
    }
    else
        return file->Add(argv2,Type,DoAdd);
}

//--------------------------------------------------------------------------------
void CIwASDFilenameRoots::DebugPrint(int indent,CIwASDFilename* file)
{
#ifdef I3D_OS_WINDOWS
    if (file==NULL)
        for (int i=0; i<(int)m_Roots.size(); i++)
        {
            DebugPrint(indent+1,m_Roots[i]);
        }
    else
    {
        OutputDebugString(wxString::Format(L"%s %s %d r%d\n",wxString(indent,L' '),file->m_Name,file->m_Root,file->m_RefCount).c_str());

        for (int i=0; i<(int)file->m_Children.size(); i++)
        {
            DebugPrint(indent+1,file->m_Children[i]);
        }
    }

#endif
}

//--------------------------------------------------------------------------------
// CIwASDFilename
//--------------------------------------------------------------------------------
bool CIwASDFilename::IsAbsPath(const wxString& path)
{
#ifdef I3D_OS_WINDOWS
    //PC paths
    if (path[1]==':' && (path[2]=='\\' || path[2]=='/' || path.size()==2))
        return true;

    if (path[0]=='\\' && path[1]=='\\')
        return true;

#else
    if (path[0]=='/')
        return true;

#endif
    return false;
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::GetDir(const wxString& name,CIwASDFilename* parent,unsigned char root)
{
    CIwASDFilename* file=CIwTheFileRoots.Get(name,parent,TYPE_DIR,true);

    if (file!=NULL)
        file->m_Root|=root;

    return file;
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::GetFile(const wxString& name,CIwASDFilename* parent)
{
    return CIwTheFileRoots.Get(name,parent,TYPE_FILENAME,true);
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::GetSection(const wxString& name,CIwASDFilename* parent)
{
    return CIwTheFileRoots.Get(name,parent,TYPE_SECTION,true);
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::Find(const wxString& name,CIwASDFilename* parent)
{
    return CIwTheFileRoots.Get(name,parent,TYPE_ALL,false);
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::Add(std::vector<wxString>& argv,unsigned char Type,bool DoAdd)
{
    if (argv.size()==0)
    {
        if (m_Type!=Type && Type!=TYPE_ALL)
            return NULL;

        if (DoAdd)
            IncRef();

        return this;
    }

    std::vector<wxString> argv2;
    int i;

    for (i=1; i<(int)argv.size(); i++)
    {
        argv2.push_back(argv[i]);
    }

    if (argv[0].IsSameAs(L"..",false))
    {
        if (m_Parent==NULL)
            return NULL;

        return m_Parent->Add(argv2,Type,DoAdd);
    }

    for (i=0; i<(int)m_Children.size(); i++)
    {
        if (m_Children[i]->m_Name.IsSameAs(argv[0],false))
            return m_Children[i]->Add(argv2,Type,DoAdd);
    }

    if (!DoAdd) return NULL;

    CIwASDFilename* file=new CIwASDFilename(argv[0],this);
    m_Children.push_back(file);
    IncRef();

    if (argv.size()==1)
    {
        file->IncRef();
        file->m_Type=Type;
        return file;
    }
    else
        return file->Add(argv2,Type,true);
}

//--------------------------------------------------------------------------------
CIwASDFilename::~CIwASDFilename()
{
    if (m_Parent!=NULL)
    {
        for (int i=0; i<(int)m_Parent->m_Children.size(); i++)
        {
            if (m_Parent->m_Children[i]==this)
            {
                m_Parent->m_Children.erase(m_Parent->m_Children.begin()+i);
                break;
            }
        }
        m_Parent->DecRef();
    }
}

//--------------------------------------------------------------------------------
void CIwASDFilename::DecRef()
{
    m_RefCount--;
    if (m_RefCount==0)
    {
        if (m_Parent==NULL)
        {
            for (int i=0; i<(int)CIwTheFileRoots.m_Roots.size(); i++)
            {
                if (CIwTheFileRoots.m_Roots[i]==this)
                {
                    CIwTheFileRoots.m_Roots.erase(CIwTheFileRoots.m_Roots.begin()+i);
                    break;
                }
            }
        }

        delete this;
    }
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::GetString(unsigned char root,const wxString& sep)
{
    wxString text;

    if (m_Root!=ROOT_NOT && root!=ROOT_NOT && (m_Root&root)==root)
        return L"";

    if (m_Parent!=NULL)
    {
        text=m_Parent->GetString(root,sep);

        if (m_Type==TYPE_SECTION)
            text+=sep[1];
    }

    text+=m_Name;

    if (m_Type==TYPE_DIR)
        text+=sep[0];

    return text;
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::GetString(CIwASDFilename* root,const wxString& sep)
{
    wxString text;
    int num=0;

    for (CIwASDFilename* root2=root; root2!=NULL; root2=root2->m_Parent)
    {
        if (root2==this)
        {
            for (int i=0; i<num; i++)
            {
                text+=wxString::Format(L"..%c",sep[0]);
            }
            if (num==0 && m_Type==TYPE_DIR)
                text+=sep[0];

            return text;
        }

        num++;
    }

    if (m_Parent!=NULL)
    {
        text=m_Parent->GetString(root,sep);

        if (m_Type==TYPE_SECTION)
            text+=sep[1];
    }

    text+=m_Name;

    if (m_Type==TYPE_DIR)
        text+=sep[0];

    return text;
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::GetFile()
{
    if (m_Type==TYPE_FILENAME)
        return this;

    if (m_Parent==NULL)
        return NULL;

    return m_Parent->GetFile();
}

//--------------------------------------------------------------------------------
CIwASDFilename* CIwASDFilename::GetRoot(unsigned char root)
{
    if ((m_Root&root)==root)
        return this;

    if (m_Parent==NULL)
        return NULL;

    return m_Parent->GetRoot(root);
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::c_str(unsigned char root)
{
    return GetString(root);
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::c_str(bool mainRoot)
{
    return GetString(mainRoot ? ROOT_MAIN : ROOT_NOT);
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::getDir(bool noFinalSlash)
{
    if (m_Type!=TYPE_DIR)
    {
        if (m_Parent==NULL)
            return L"";
        else
            return m_Parent->getDir(noFinalSlash);
    }

    wxString text=GetString();
    if (text.EndsWith(L"/") && noFinalSlash)
        text.RemoveLast();

    return text;
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::getExt()
{
    if (m_Type!=TYPE_FILENAME)
        return L"";

    if (m_Name.Find('.')==wxNOT_FOUND)
        return L"";
    else
        return m_Name.AfterLast('.');
}

//--------------------------------------------------------------------------------
wxString CIwASDFilename::getName(bool includeExt)
{
    if (m_Type==TYPE_FILENAME && !includeExt)
        return m_Name.BeforeLast('.');

    return m_Name;
}

//--------------------------------------------------------------------------------
CIwASDFile* CIwASDFilename::GetShortestFile()
{
    CIwASDFile* file,*file2=m_File;
    int max=1000,j;

    if (file2==NULL) return NULL;

    for (max=0,file=file2; file!=NULL; file=file->m_Parent)
    {
        max++;
    }

    for (int k=0; k<(int)m_OtherFiles.size(); k++)
    {
        for (j=0,file=m_OtherFiles[k]; file!=NULL; file=file->m_Parent)
        {
            j++;
        }
        if (j<max)
        {
            max=j;
            file2=m_OtherFiles[k];
        }
    }
    return file2;
}

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

//------------------------------------------------------------------------------
CIwASDITXClassDef::~CIwASDITXClassDef()
{
    for (int i=0; i<(int)m_Members.size(); i++)
    {
        delete m_Members[i];
    }
}

//------------------------------------------------------------------------------
CIwASDITXInstance* CIwASDITXClassDef::Make(CIwASDFileReader& reader)
{
    return new CIwASDITXInstance(this);
}

//------------------------------------------------------------------------------
CIwASDFileReader::CIwASDFileReader(CIwASDITXClassDef* context) : m_NextObject(NULL)
{
    m_CurrObject=context->Make(*this);
}

//------------------------------------------------------------------------------
CIwASDFileReader::~CIwASDFileReader()
{
    if (m_ObjectStack.empty()) return;

    m_CurrObject=m_ObjectStack.back();
    m_ObjectStack.pop_back();
}

//------------------------------------------------------------------------------
void CIwASDFileReader::PushContext()
{
    if (m_NextObject==NULL) return;

    m_ObjectStack.push_back(m_CurrObject);
    m_CurrObject=m_NextObject;
    m_NextObject=NULL;
}

//------------------------------------------------------------------------------
void CIwASDFileReader::PopContext()
{
}

//------------------------------------------------------------------------------
void CIwASDFileReader::PushFile(const wxString& fileName)
{
    m_FileStack.push_back(m_CurrFile);
    m_CurrFile=Pos(fileName);

    if (!m_CurrFile.m_File->Exists() || !m_CurrFile.m_File->Open())
        PopFile();

    int start=-1;
    for (int i=0; i<(int)m_CurrFile.m_File->GetLineCount(); i++)
    {
        for (int j=0; j<(int)(*m_CurrFile.m_File)[i].size()-1; j++)
        {
            if ((*m_CurrFile.m_File)[i][j]=='#' && start==-1)
            {
                (*m_CurrFile.m_File)[i]=(*m_CurrFile.m_File)[i].Mid(0,j);
                break;
            }

            if ((*m_CurrFile.m_File)[i][j]=='/' && start==-1)
            {
                if ((*m_CurrFile.m_File)[i][j+1]=='/')
                {
                    (*m_CurrFile.m_File)[i]=(*m_CurrFile.m_File)[i].Mid(0,j);
                    break;
                }

                if ((*m_CurrFile.m_File)[i][j+1]=='*')
                    start=j;
            }

            if (start!=-1 && (*m_CurrFile.m_File)[i][j]=='*' && (*m_CurrFile.m_File)[i][j+1]=='/')
            {
                (*m_CurrFile.m_File)[i]=(*m_CurrFile.m_File)[i].Mid(0,start)+(*m_CurrFile.m_File)[i].Mid(j+2);
                j=start+1;
                start=-1;
            }
        }
        if (start!=-1)
            (*m_CurrFile.m_File)[i]=(*m_CurrFile.m_File)[i].Mid(0,start);
    }
}

//------------------------------------------------------------------------------
bool CIwASDFileReader::PopFile()
{
    if (m_FileStack.empty()) return false;

    if (m_CurrFile.m_File!=NULL) delete m_CurrFile.m_File;

    m_CurrFile=m_FileStack.back();
    m_FileStack.pop_back();

    return (m_CurrFile.m_File!=NULL);
}

//------------------------------------------------------------------------------
bool CIwASDFileReader::FetchNextLine()
{
    while (true)
    {
        if (m_CurrFile.m_File==NULL) return false;

        m_CurrFile.m_Line++;
        if (m_CurrFile.m_Line>=(int)m_CurrFile.m_File->GetLineCount())
            if (!PopFile()) return false;

        if (SuperSplit((*m_CurrFile.m_File)[m_CurrFile.m_Line],m_Args,L" \t\n\r")>0)
            break;
    }
    m_CurrFile.m_Offset=0;
    return true;
}

//------------------------------------------------------------------------------
void CIwASDITX::LoadFile(bool force)
{
    int i;
    bool topLevel=true;

    if (m_Data!=NULL)
    {
        if (!force) return;

        delete m_Data;
        m_Data=NULL;
    }

    if (m_Paths.size()<1) return;

    if (CIwTheFileModule->m_ClassDefs.find(m_Type)==CIwTheFileModule->m_ClassDefs.end()) return;

    CIwASDFileReader reader(CIwTheFileModule->m_ClassDefs[m_Type]);

    reader.PushFile(m_Paths[0]->c_str());

    while (reader.FetchNextLine())
    {
        if (reader.LineArg(0).IsSameAs(L"{"))
        {
            if (topLevel)
                m_Data=reader.m_CurrObject;

            topLevel=false;
            reader.PushContext();
            continue;
        }

        if (reader.LineArg(0).IsSameAs(L"}"))
        {
            reader.PopContext();
            continue;
        }

        CIwASDITXClassDef* def=reader.m_CurrObject->m_Def;

        for (i=0; i<(int)def->m_Members.size(); i++)
        {
            if (reader.LineArg(0).IsSameAs(def->m_Members[i]->m_Name,false))
            {
                CIwASDITXField* field=def->m_Members[i]->Make(reader);
                if (field!=NULL)
                    reader.m_CurrObject->m_Fields.push_back(field);

                break;
            }
        }
        if (i<(int)def->m_Members.size()) continue;

        if (topLevel && reader.LineArg(0).IsSameAs(def->m_Name,false))
        {
            reader.SetNextObject(reader.m_CurrObject);
            continue;
        }

        for (i=0; i<(int)def->m_Members.size(); i++)
        {
            if (def->m_Members[i]->IsMatch(reader))
            {
                CIwASDITXField* field=def->m_Members[i]->Make(reader);
                if (field!=NULL)
                    reader.m_CurrObject->m_Fields.push_back(field);

                break;
            }
        }
    }
}

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
// Standard includes
#include "IwConfig.h"

// Project includes
#include "s3eConfig.h"

// Library includes
#include <sstream>

//-----------------------------------------------------------------------------

CIwConfig::CIwConfig()
{
    _ReadConfig();
}

const char* CIwConfig::GetConfig(const char* pName) const
{
    ConfigMap::const_iterator it = m_Config.find(pName);
    if (it != m_Config.end())
        return it->second.c_str();

    return NULL;
}

bool CIwConfig::GetOption(const char* pName) const
{
    OptionsMap::const_iterator it = m_Options.find(pName);
    if (it != m_Options.end())
        return it->second;

    return false;
}

void CIwConfig::_ReadConfig()
{
    const char* pOptions[] =
    {
        "viewerpath",
        "viewerfile",
        "palettefile",
        "restemplate",      // TODO, Pass to CIwTextParserITX::ParseFile
        "viewertemplate",   // TODO, Pass to CIwTextParserITX::ParseFile
        "data",

        "scanline",
        "zbuffer",
        "GLAPI",
        "ResBuildStyle"     // TODO, Pass to CIwResManager::SetBuildStyle
    };
    const int numOptions = sizeof(pOptions) / sizeof(pOptions[0]);

    for (int i=0; i<numOptions; ++i)
    {
        _ReadCommandLineOption(pOptions[i]);
    }

    int count = 1;
    while (true)
    {
        if (!_ReadCommandLineArgument(count))
            break;
    }
}

bool CIwConfig::_ReadCommandLineOption(const char* pName)
{
    char option[S3E_CONFIG_STRING_MAX];
    if (s3eConfigGetString("commandline", pName, option) == S3E_RESULT_SUCCESS)
    {
        m_Config[pName] = option;
        return true;
    }

    return false;
}

bool CIwConfig::_ReadCommandLineArgument(int& index, const char* pName)
{
    std::ostringstream argvstr;
    argvstr << "argv[" << index << "]";
    ++index;

    char option[S3E_CONFIG_STRING_MAX];
    if (s3eConfigGetString("commandline", argvstr.str().c_str(), option) ==
        S3E_RESULT_SUCCESS)
    {
        const char* pStr = option;
        while (*pStr == '\r' || *pStr == '\n' || *pStr == '-')
        {
            ++pStr;
        }

        int len = strlen(pStr);
        if (len)
        {
            if (pName)
                m_Config[pName] = pStr;
            else if (pStr[len-1] == ':')
                _ReadCommandLineArgument(index, pStr);
            else
                m_Options[pStr] = true;

            return true;
        }
    }

    return false;
}

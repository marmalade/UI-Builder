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
#ifndef IW_CONFIG_H
#define IW_CONFIG_H

// Library includes
#include <map>
#include <string>

//-----------------------------------------------------------------------------

class CIwConfig
{
public:
    CIwConfig();

    const char* GetConfig(const char* pName) const;
    bool GetOption(const char* pName) const;

private:
    // Private utils
    void _ReadConfig();

    bool _ReadCommandLineOption(const char* pName);
    bool _ReadCommandLineArgument(int& index, const char* pName = NULL);

    // Member data
    typedef std::map<std::string, std::string> ConfigMap;
    typedef std::map<std::string, bool> OptionsMap;

    ConfigMap m_Config;
    OptionsMap m_Options;
};

#endif

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
#include "Localise.h"

// Library includes
#include "IwLocalise.h"
#include "IwUIFuncs.h"
#include "IwUIPropertySet.h"
#include <string>

// Static data
static CLocalise* s_Localise;

//-----------------------------------------------------------------------------

CLocalise::CLocalise()
{
    IwLocaliseInit();

    s_Localise = this;
}

CLocalise::~CLocalise()
{
    s_Localise = NULL;

    IwLocaliseTerminate();
}

void CLocalise::SetLocalisation(const char* pLocaliseDat)
{
    Clear();

    CIwStringL pathonly, fileonly;
    CIwStringS ext;
    IwGetResManager()->SplitPathName(pLocaliseDat, pathonly, fileonly, ext);

    const char* pHashesFile = "LocaliseHashes";

    if (ext == "dat" && !(fileonly == pHashesFile))
    {
        std::string hashes = pathonly.c_str();
        hashes += "/";
        hashes += pHashesFile;
        hashes += ".dat";

        const char* pHashesDat = hashes.c_str();

        if (s3eFileCheckExists(pLocaliseDat) && s3eFileCheckExists(pHashesDat))
        {
            IwLocaliseSetLanguageFile(pLocaliseDat);

            IwLocaliseSetHashLookupFile(pHashesDat);

            IwUISetLocaliseCallback(_LocaliseCallback);
        }
        else
            IwAssertMsg(VIEWER, false,
                        ("Could not find %s or %s", pLocaliseDat, pHashesDat));
    }
    else
        IwAssertMsg(VIEWER, false,
                    ("Expected .dat extension for localisation file %s", pLocaliseDat));
}

void CLocalise::Clear()
{
    IwLocaliseInit();

    IwUISetLocaliseCallback(NULL);
}

const char* CLocalise::_Localise(const CIwPropertySet& propertySet)
{
    uint32 hash = 0;
    if (propertySet.GetProperty("localiseCaption", hash, true))
        if (hash)
            return IwLocaliseHashed(hash);

    return NULL;
}

const char* CLocalise::_LocaliseCallback(const CIwPropertySet& propertySet)
{
    return s_Localise ? s_Localise->_Localise(propertySet) : NULL;
}

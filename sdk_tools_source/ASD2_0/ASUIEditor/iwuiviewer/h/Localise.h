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
#ifndef LOCALISE_H
#define LOCALISE_H

// Forward declarations
class CIwPropertySet;

//-----------------------------------------------------------------------------

class CLocalise
{
public:
    CLocalise();
    ~CLocalise();

    void SetLocalisation(const char* pLocaliseDat);
    void Clear();

private:
    // Private utils
    const char* _Localise(const CIwPropertySet& propertySet);
    static const char* _LocaliseCallback(const CIwPropertySet& propertySet);
};

#endif

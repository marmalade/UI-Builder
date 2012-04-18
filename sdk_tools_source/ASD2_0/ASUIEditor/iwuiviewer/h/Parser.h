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
#ifndef PARSER_H
#define PARSER_H

// Library includes
#include "IwTextParserITX.h"

// Forward declarations
class CIwUIElement;

//-----------------------------------------------------------------------------

namespace Parser
{

void* ParseFile(const char* pFile);

void ParseContents(CIwManaged* pObject, const char* pFile);

CIwUIElement* ParseElement(const char* pItxFile);

class CTextParserAttributes : public CIwTextParserITX, CIwManaged
{
public:
    static void ParseAttributes(CIwManaged* pObject, const char* pData,
                                bool callParseOpenAndClose = true);

private:
    CTextParserAttributes(CIwManaged* pObject, bool callParseOpenAndClose);
    ~CTextParserAttributes();

    // IwManaged virtuals
    virtual void ParseCloseChild(CIwTextParserITX* pParser, CIwManaged* pChild);

    // Private utils
    void ParseClass();
    void ParseEndClass();
    void ParseName();

    static void ParseClassCallback(CIwTextParser* pParser);
    static void ParseEndClassCallback(CIwTextParser* pParser);
    static void ParseNameCallback(CIwTextParser* pParser);

    // Member data
    CIwManaged* m_Object;
    const bool m_ParseOpenAndClose;

    bool m_MatchedClassName;
    bool m_MatchedObjectName;
    bool m_ParsingObject;
};

}

#endif

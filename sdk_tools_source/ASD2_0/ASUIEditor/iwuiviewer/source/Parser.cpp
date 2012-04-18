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
#include "Parser.h"

// Library includes
#include "IwFile.h"
#include "IwUIElement.h"
#include <string>

//-----------------------------------------------------------------------------

namespace Parser
{

void* ParseFile(const char* pFilename)
{
    void* pRet = NULL;

    // Read file into buffer to avoid problems with text mode translation
    // (seeking backwards past new-line causes issues...)
    s3eFile* pFile = IwFileOpenPrefixed(pFilename, "r");

    if (pFile)
    {
        int fileSize = s3eFileGetSize(pFile);
        if (fileSize > 0)
        {
            char* pData = new char[fileSize + 1];

            // Less than fileSize due to text mode translation
            int bytesRead = s3eFileRead(pData, 1, fileSize, pFile);
            if (bytesRead > 0)
            {
                pData[bytesRead] = 0;
                pRet = IwGetTextParserITX()->ParseString(pData, pFilename);
            }
            else
                IwAssertMsg(VIEWER, false, ("Failed to read file %s", pFilename));

            delete[] pData;
        }

        s3eFileClose(pFile);
    }
    else
        IwAssertMsg(VIEWER, false, ("Failed to open file %s", pFilename));

    return pRet;
}

void ParseContents(CIwManaged* pObject, const char* pFile)
{
    if (pObject)
    {
        CIwTextParserITX* pTextParser = IwGetTextParserITX();

        pTextParser->PushObject(pObject);
        pObject->ParseOpen(pTextParser);

        ParseFile(pFile);

        pObject->ParseClose(pTextParser);
        pTextParser->PopObject();
    }
}

CIwUIElement* ParseElement(const char* pItxFile)
{
    CIwResGroup* pPrevGroup = IwGetResManager()->GetCurrentGroup();

    // Create temporary group to read resource into
    CIwResGroup* pTempGroup = new CIwResGroup("__temp__");
    IwGetResManager()->AddGroup(pTempGroup);
    IwGetResManager()->SetCurrentGroup(pTempGroup);

    ParseFile(pItxFile);

    CIwUIElement* pElement = NULL;
    if (CIwResList* pResList = pTempGroup->GetListNamed(IW_UI_RESTYPE_ELEMENT))
    {
        CIwManagedList& resList = pResList->m_Resources;

        if (resList.GetSize() > 0)
        {
            pElement = IwSafeCast<CIwUIElement*>(resList[0]);
            resList.RemoveFast(pElement);
            pElement->DecCount(); // Decrement count group added
        }
    }

    IwGetResManager()->DestroyGroup(pTempGroup);

    // Restore previous group
    IwGetResManager()->SetCurrentGroup(pPrevGroup);

    IwAssertMsg(VIEWER, pElement, ("Failed to load element from: %s", pItxFile));
    return pElement;
}

void CTextParserAttributes::ParseAttributes(CIwManaged* pObject,
                                            const char* pData, bool callParseOpenAndClose)
{
    if (!pObject || !pData)
    {
        IwAssertMsg(VIEWER, false, ("Unable to parse attributes"));
        return;
    }

    // Hide singleton
    CIwTextParserITX* pSingleton = g_IwTextParserITX;
    g_IwTextParserITX = NULL;

    // Scope of our override singleton
    {
        std::string parseString(pData);
        parseString += "\nendclass"; // Indicator so we know when to stop

        CTextParserAttributes textParser(pObject, callParseOpenAndClose);
        textParser.ParseString(parseString.c_str());
    }

    // Restore singleton
    g_IwTextParserITX = pSingleton;
}

CTextParserAttributes::CTextParserAttributes(CIwManaged* pObject,
                                             bool callParseOpenAndClose) :
    m_Object(pObject),
    m_ParseOpenAndClose(callParseOpenAndClose),
    m_MatchedClassName(false),
    m_MatchedObjectName(false),
    m_ParsingObject(false)
{
    // Add our override token functions
    AddTokenFunction("class",       ParseClassCallback);
    AddTokenFunction("endclass",    ParseEndClassCallback);
    AddTokenFunction("name",        ParseNameCallback);

    // Use ourself as a container object so we can do standard
    // PushObject, ParseOpen, ParseClose, PopObject behaviour
    PushObject(this);
}

CTextParserAttributes::~CTextParserAttributes()
{
    CIwManaged* pObject = PopObject();

    IwAssertMsg(VIEWER, m_MatchedClassName && m_MatchedObjectName,
                ("Didn't start parsing object correctly"));
    IwAssertMsg(VIEWER, !m_ParsingObject && pObject == this,
                ("Didn't end parsing object correctly"));
}

// IwManaged virtuals
void CTextParserAttributes::ParseCloseChild(CIwTextParserITX* pParser, CIwManaged* pChild)
{
    IwAssertMsg(VIEWER, m_ParsingObject && pChild == m_Object,
                ("Parser closing unexpected object"));
}

// Private utils
void CTextParserAttributes::ParseClass()
{
    if (!m_ParsingObject)
    {
        CIwStringL className;
        ReadString(className);

        m_MatchedClassName = !strcmp(m_Object->GetClassName(), className.c_str());
        IwAssertMsg(VIEWER, m_MatchedClassName, ("Class name didn't match"));
    }
    else
        IwAssertMsg(VIEWER, false, ("Expected to push starting object"));
}

void CTextParserAttributes::ParseEndClass()
{
    CIwManaged* pObject = GetObject();

    if (pObject && pObject == m_Object && m_ParsingObject)
    {
        if (m_ParseOpenAndClose)
            pObject->ParseClose(this);

        PopObject();
        m_ParsingObject = false;
    }
    else
        IwAssertMsg(VIEWER, false, ("Expected to pop starting object"));
}

void CTextParserAttributes::ParseName()
{
    if (!m_ParsingObject)
    {
        CIwStringL objectName;
        ReadString(objectName);

        m_MatchedObjectName = !strcmp(m_Object->DebugGetName(), objectName.c_str());
        IwAssertMsg(VIEWER, m_MatchedObjectName, ("Object name didn't match"));

        if (m_MatchedClassName && m_MatchedObjectName)
        {
            m_ParsingObject = true;
            PushObject(m_Object);

            if (m_ParseOpenAndClose)
                m_Object->ParseOpen(this);
        }
    }
    else
    {
        CIwManaged* pObject = GetObject();
        if (pObject && pObject != m_Object)
            // Pass on to current object
            pObject->ParseAttribute(this, "name");
        else
        {
            CIwStringL discard;
            ReadString(discard);

            IwAssertMsg(VIEWER, false, ("Discarding name '%s'", discard.c_str()));
        }
    }
}

void CTextParserAttributes::ParseClassCallback(CIwTextParser* pParser)
{
    IwSafeCast<CTextParserAttributes*>(pParser)->ParseClass();
}

void CTextParserAttributes::ParseEndClassCallback(CIwTextParser* pParser)
{
    IwSafeCast<CTextParserAttributes*>(pParser)->ParseEndClass();
}

void CTextParserAttributes::ParseNameCallback(CIwTextParser* pParser)
{
    IwSafeCast<CTextParserAttributes*>(pParser)->ParseName();
}

}

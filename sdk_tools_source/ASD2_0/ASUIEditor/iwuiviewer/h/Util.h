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
#ifndef UTIL_H
#define UTIL_H

// Library includes
#include <string>
#include <vector>

// Forward declarations
class CIwResGroup;
class CIwResList;
class CIwResource;
class CIwUIElement;
class CIwUIElementItem;
class CIwUILayout;
class CIwUILayoutItemContainer;
class CIwUIRect;
class CIwVec2;

//-----------------------------------------------------------------------------

namespace Util
{

void SplitQuotedStrings(const std::string& str, std::vector<std::string>& out,
                        char quote = '\"');

void SplitSeparatedStrings(const std::string& str, std::vector<std::string>& out,
                           char seperator = '|', bool includeEmptyStrings = false);

bool FindAttribute(const char* pAttribute, const std::string& str, std::string& out);

bool ReadLinesFromFile(const char* pFileName, std::vector<std::string>& out);

template<class _T1, class _T2, class _T3> struct triple
{
    typedef _T1 first_type;
    typedef _T2 second_type;
    typedef _T3 third_type;

    triple()
        : first(_T1()), second(_T2()), third(_T3()) {}
    triple(const _T1& _V1, const _T2& _V2, const _T3& _V3)
        : first(_V1), second(_V2), third(_V3) {}
    template<class U, class V, class W> triple(const triple<U, V, W> &t)
        : first(t.first), second(t.second), third(t.third) {}

    _T1                 first;
    _T2                 second;
    _T3                 third;
};

typedef triple<CIwResGroup*, CIwResList*, CIwResource*> ResourceLocation;

ResourceLocation FindResourceLocation(CIwResGroup* pResGroup,
                                      const char* pResName, const char* pResType);

bool IsBaseElement(CIwUIElement* pElement);

// Find element contained in root without recursion
CIwUIElement* FindBaseElement(const std::string& baseName);

CIwUIElement* FindChildElement(CIwUIElement* pParent, const std::string& childName);

CIwUIElement* FindElementByPath(const std::vector<std::string>& elementPath);

void GetElementPath(CIwUIElement* pElement, std::string& path);

bool IsUniqueElementName(const std::string& name);

void GetUniqueElementName(std::string& name);

void DestroyElement(CIwUIElement* pElement);

typedef std::pair<CIwUIElement*, CIwUILayoutItemContainer*> LayoutLocation;

LayoutLocation RemoveElement(CIwUIElement* pElement);

void RestoreElement(CIwUIElement* pElement, LayoutLocation& layoutLocation);

CIwUILayout* GetContainingLayout(CIwUIElement* pElement);

CIwUIElement* IntersectElement(CIwUIElement* pElement, const CIwVec2& pos,
                               CIwUIElement* pIgnoreAndDescendants = NULL, int16 border = 0);

void IntersectElement(CIwUIElement* pElement, const CIwUIRect& rect,
                      std::vector<CIwUIElement*>& list, int16 border = 0);

}

#endif

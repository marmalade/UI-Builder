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
#include "Util.h"

// Library includes
#include "IwResManager.h"
#include "IwUIElement.h"
#include "IwUIElementItem.h"
#include "IwUILayout.h"
#include "IwUIView.h"
#include "IwUIScrollableView.h"
#include <sstream>

//-----------------------------------------------------------------------------

namespace Util
{

void SplitQuotedStrings(const std::string& str, std::vector<std::string>& out,
                        char quote)
{
    bool quoted = false;
    bool escape = false;
    std::string curr;

    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    {
        if (escape)
        {
            curr += *it;
            escape = false;
            continue;
        }

        if (*it == '\\')
        {
            escape = true;
            continue;
        }

        if (*it == quote)
        {
            if (quoted || !curr.empty())
            {
                out.push_back(curr);
                curr = "";
            }

            quoted = !quoted;
            continue;
        }

        if (!quoted && isspace(*it))
        {
            if (!curr.empty())
            {
                out.push_back(curr);
                curr = "";
            }

            continue;
        }

        curr += *it;
    }

    if (quoted || !curr.empty())
        out.push_back(curr);
}

void SplitSeparatedStrings(const std::string& str, std::vector<std::string>& out,
                           char seperator, bool includeEmptyStrings)
{
    std::string curr;
    bool lastWasSeperator = false;

    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it)
    {
        if (*it == seperator)
        {
            if (includeEmptyStrings || !curr.empty())
            {
                out.push_back(curr);
                curr = "";
            }

            lastWasSeperator = true;
            continue;
        }

        curr += *it;
        lastWasSeperator = false;
    }

    if ((lastWasSeperator && includeEmptyStrings) || !curr.empty())
        out.push_back(curr);
}

void TrimQuotes(std::string & str)
{
    int len = str.length();
    if (len > 0 && str[0] == '"')
        str.erase(0, 1);

    len = str.length();
    if (len > 0 && str[len-1] == '"')
        str.erase(len-1, 1);
}

bool FindAttribute(const char* pAttribute, const std::string& str, std::string& out)
{
    std::string attr;

    std::istringstream iss(str);
    iss >> std::skipws >> attr;

    if (attr == pAttribute)
    {
        iss >> out;
        TrimQuotes(out);
        return out.length() != 0;
    }

    return false;
}

bool ReadLinesFromFile(const char* pFileName, std::vector<std::string>& out)
{
    if (s3eFile* pFile = s3eFileOpen(pFileName, "r"))
    {
        char line[0x400];

        while (s3eFileReadString(line, sizeof(line), pFile))
            out.push_back(line);

        s3eFileClose(pFile);
        return true;
    }

    return false;
}

ResourceLocation FindResourceLocation(CIwResGroup* pResGroup,
                                      const char* pResName, const char* pResType)
{
    // Account for child groups. Avoid usual res-manager behaviour to
    // search shared groups, etc. In an attempt to make it more deterministic.
    if (pResGroup && pResName && pResType)
    {
        if (CIwResList* pResList = pResGroup->GetListNamed(pResType, IW_RES_PERMIT_NULL_F))
        {
            if (CIwResource* pResource = (CIwResource*)
                                         pResList->m_Resources.GetObjNamed(pResName))
                return ResourceLocation(pResGroup, pResList, pResource);
        }

        const CIwManagedList& childGroups = pResGroup->GetChildren();
        for (int i=0; i<(int)childGroups.GetSize(); ++i)
        {
            CIwResGroup* pResGroup = IwSafeCast<CIwResGroup*>(childGroups[i]);

            ResourceLocation resLoc = FindResourceLocation(pResGroup, pResName, pResType);

            if (resLoc.first)
                return resLoc;
        }
    }

    return ResourceLocation(NULL, NULL, NULL);
}

bool IsBaseElement(CIwUIElement* pElement)
{
    for (int i=0; i<IwGetUIView()->GetNumElements(); ++i)
    {
        CIwUIElement* pBaseElement = IwGetUIView()->GetElement(i);

        if (pBaseElement == pElement)
            return true;
    }

    return false;
}

CIwUIElement* FindBaseElement(const std::string& baseName)
{
    for (int i=0; i<IwGetUIView()->GetNumElements(); ++i)
    {
        CIwUIElement* pElement = IwGetUIView()->GetElement(i);

        if (pElement && baseName == pElement->DebugGetName())
            return pElement;
    }

    return NULL;
}

CIwUIElement* FindChildElement(CIwUIElement* pParent, const std::string& childName)
{
    if (pParent)
    {
        for (int i=0; i<pParent->GetNumChildren(); ++i)
        {
            CIwUIElement* pChild = pParent->GetChild(i);

            if (pChild && childName == pChild->DebugGetName())
                return pChild;
        }
    }

    return NULL;
}

CIwUIElement* FindElementByPath(const std::vector<std::string>& elementPath)
{
    CIwUIElement* pElement = NULL;

    for (int i=0; i<(int)elementPath.size(); ++i)
    {
        const std::string& elementName = elementPath[i];

        if (!pElement)
            pElement = FindBaseElement(elementName);
        else
        {
            CIwUIElement* pChild = FindChildElement(pElement, elementName);

            if (!pChild)
            {
                if (pElement->GetLayout())
                {
                    // Builder treats layouts as part of hierarchy, ignore...
                    // TODO Handle nested layouts here too
                    if (elementName == pElement->GetLayout()->DebugGetName())
                        continue;
                }

                if (elementName[0] == '^')
                {
                    // Ignore things like '^background' which become drawables
                    // rather than actual elements.
                    continue;
                }
            }

            pElement = pChild;
        }

        if (!pElement)
        {
            IwAssertMsg(VIEWER, false,
                        ("Failed to find element: %s", elementName.c_str()));

            return NULL;
        }
    }

    return pElement;
}

void GetElementPath(CIwUIElement* pElement, std::string& path)
{
    if (!pElement)
        return;

    if (CIwUIElement* pParent = pElement->GetParent())
    {
        GetElementPath(pParent, path);

        path += "|";
    }

    path += pElement->DebugGetName();
}

bool IsUniqueElementName(const std::string& name)
{
    if (IwGetResManager()->GetResNamed(name.c_str(), IW_UI_RESTYPE_ELEMENT,
                                       IW_RES_SEARCH_ALL_F | IW_RES_PERMIT_NULL_F) != NULL)
        return false;

    if (IwGetUIView()->GetChildNamed(name.c_str(), true) != NULL)
        return false;

    return true;
}

void GetUniqueElementName(std::string& name)
{
    // Check if name is already unqiue
    if (IsUniqueElementName(name))
        return;

    std::string baseName;

    size_t underscore = name.rfind('_');
    if (underscore != std::string::npos)
        baseName.assign(name, 0, underscore);
    else
        baseName = name;

    for (int i=0;; ++i)
    {
        std::ostringstream oss;
        oss << baseName << '_' << i;

        if (IsUniqueElementName(oss.str()))
        {
            name = oss.str();
            return;
        }
    }
}

void DestroyElement(CIwUIElement* pElement)
{
    if (pElement)
    {
        if (CIwUIElement* pParent = pElement->GetParent())
            pParent->RemoveChild(pElement);

        IwAssertMsg(VIEWER, pElement->IsCountZero(),
                    ("Destroying element with non-zero reference count"));
        delete pElement;
    }
}

LayoutLocation RemoveElement(CIwUIElement* pElement)
{
    // Remove child from parent without destroying layout container
    // So we can re-add later to same position
    if (CIwUIElement* pParent = pElement ? pElement->GetParent() : NULL)
    {
        CIwUIElementItem* pElementItem = CIwUIElementItem::GetElementItem(pElement);
        CIwUILayoutItemContainer* pContainerItem =
            pElementItem ? IwSafeCast<CIwUILayoutItemContainer*>(
                pElementItem->GetParentItem()) : NULL;

        if (pContainerItem)
        {
            // Break connection to element item first, otherwise container
            // will be destroyed with element item
            pContainerItem->SetItem(NULL);

            // Detaches it from element.
            delete pElementItem;
            pElementItem = NULL;
        }

        pParent->RemoveChild(pElement);

        return LayoutLocation(pParent, pContainerItem);
    }

    return LayoutLocation(NULL, NULL);
}

void RestoreElement(CIwUIElement* pElement, LayoutLocation& layoutLocation)
{
    if (pElement && layoutLocation.first)
    {
        layoutLocation.first->AddChild(pElement);

        bool reattached = false;

        // Behaviour changed here. We no longer have to manually put element
        // back into layout, should automatically occur in AddChild.
        // Check this has happened...
        if (layoutLocation.second)
        {
            if (CIwUIElementItem* pElementItem = IwSafeCast<CIwUIElementItem*>(
                    layoutLocation.second->GetItem()))
                reattached =
                    pElementItem == CIwUIElementItem::GetElementItem(pElement);
        }

        IwAssertMsg(UI, reattached, ("Failed to restore element into layout"));
        (void) reattached;
    }
}

CIwUILayout* GetContainingLayout(CIwUIElement* pElement)
{
    CIwUIElementItem* pElementItem =
        pElement ? CIwUIElementItem::GetElementItem(pElement) : NULL;

    CIwUILayoutItemContainer* pContainerItem =
        pElementItem ? IwSafeCast<CIwUILayoutItemContainer*>(
            pElementItem->GetParentItem()) : NULL;

    CIwUILayout* pLayout =
        pContainerItem ? IwSafeCast<CIwUILayout*>(
            pContainerItem->GetParentItem()) : NULL;

    return pLayout;
}

CIwUIElement* IntersectElement(CIwUIElement* pElement, const CIwVec2& pos,
                               CIwUIElement* pIgnoreAndDescendants, int16 border)
{
    if (!pElement)
        return NULL;

    // Ignore invisible elements, and their children.
    // Check if this is an element we want to ignore.
    if (pElement->IsVisible() && (!pIgnoreAndDescendants ||
                                  (pElement != pIgnoreAndDescendants && !pIgnoreAndDescendants->IsDescendant(pElement))))
    {
        CIwVec2 localPos = pos - pElement->GetPos();

        const CIwUIMat& transform = pElement->GetTransform();
        if (transform.GetDeterminant() == 0)
            // Can't transform pos into element's space
            return NULL;

        localPos = transform.GetInverse().TransformVec(localPos);

        // Special case for handling scrollable view
        if (CIwUIScrollableView* pScrollableView =
                dynamic_cast<CIwUIScrollableView*>(pElement))
            localPos +=  pScrollableView->GetScrollPosition();

        CIwUIRect localFrame(CIwVec2::g_Zero, pElement->GetSize());
        localFrame.x -= border;
        localFrame.y -= border;
        localFrame.w += 2 * border;
        localFrame.h += 2 * border;

        // Depth first. Check children, ignoring our frame
        for (int i=pElement->GetNumChildren()-1; i>=0; i--)
        {
            CIwUIElement* pChild = pElement->GetChild(i);

            if (CIwUIElement* pIntersection = IntersectElement(pChild,
                                                               localPos, pIgnoreAndDescendants, border))
                return pIntersection;
        }

        // Usual element intersect checks drawables not frame
        if (localFrame.Intersects(localPos))
            return pElement;
    }

    // No intersection.
    return NULL;
}

void IntersectElement(CIwUIElement* pElement, const CIwUIRect& rect,
                      std::vector<CIwUIElement*>& list, int16 border)
{
    if (!pElement)
        return;

    if (pElement->IsVisible())
    {
        CIwUIRect localRect(rect.GetPosition() - pElement->GetPos(),
                            rect.GetSize());

        const CIwUIMat& transform = pElement->GetTransform();
        if (transform.GetDeterminant() == 0)
            // Can't transform pos into element's space
            return;

        localRect = IwUIGetTransformedBounds(transform.GetInverse(), localRect);

        CIwUIRect localFrame(CIwVec2::g_Zero, pElement->GetSize());

        int shrinkX = MIN(border, localFrame.w / 2);
        int shrinkY = MIN(border, localFrame.h / 2);
        localFrame.x += shrinkX;
        localFrame.y += shrinkY;
        localFrame.w -= 2 * shrinkX;
        localFrame.h -= 2 * shrinkY;

        // Parent first.
        if (localRect.Contains(localFrame))
        {
            list.push_back(pElement);
            return;
        }

        // Check children
        for (int i=pElement->GetNumChildren()-1; i>=0; i--)
        {
            CIwUIElement* pChild = pElement->GetChild(i);

            IntersectElement(pChild, localRect, list, border);
        }
    }
}

}

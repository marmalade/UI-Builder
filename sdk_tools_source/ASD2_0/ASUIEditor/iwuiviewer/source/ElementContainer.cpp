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
#include "ElementContainer.h"

// Library includes
#include "IwUIElement.h"
#include "IwUIElementUtil.h"
#include <sstream>

//-----------------------------------------------------------------------------

CElementContainer::CElementContainer(CIwUIElement* pElement)
{
    Set(pElement);
}

void CElementContainer::Set(CIwUIElement* pElement)
{
    const char* elementName = pElement ? pElement->DebugGetName() : NULL;
    if (elementName == NULL || elementName[0] != '^')
    {
        m_Element = pElement;
        m_ChildName = "";
    }
    else
    {
        m_Element = pElement->GetParent();
        m_ChildName = elementName;
    }
}

CIwUIElement* CElementContainer::Get() const
{
    if (m_Element)
    {
        if (!IsContainer())
            return m_Element;
        else
            return m_Element->GetChildNamed(m_ChildName.c_str(), false, true);
    }

    return NULL;
}

bool CElementContainer::operator ==(const CElementContainer& rhs) const
{
    return m_Element == rhs.m_Element && m_ChildName == rhs.m_ChildName;
}

bool CElementContainer::operator !=(const CElementContainer& rhs) const
{
    return m_Element != rhs.m_Element || m_ChildName != rhs.m_ChildName;
}

static int GetStyleIndex(std::string& styleName)
{
    // Split list name and index
    int indexPos = styleName.find('[');
    std::string styleIndexString = styleName.substr(indexPos+1,
                                                    styleName.rfind(']') - (indexPos+1));
    styleName = styleName.substr(0, indexPos);

    std::istringstream iss (styleIndexString);

    int styleIndex = -1;
    iss >> styleIndex;

    return styleIndex;
}

CIwUIPropertySet* CElementContainer::GetPropertySet()
{
    if (m_Element)
    {
        if (!IsContainer())
            // Naughty
            return const_cast<CIwUIPropertySet*>(&m_Element->GetPropertySet());
        else if (m_ChildName.find('^') == 0)
        {
            std::string styleName = m_ChildName.substr(1);

            // Is are inline style in a list?
            int indexPos = styleName.find('[');
            if (indexPos != std::string::npos)
            {
                // Split list name and index
                int styleIndex = GetStyleIndex(styleName);
                if (styleIndex >= 0)
                    return _IwUICreateInlineStyleInList(m_Element,
                                                        styleName.c_str(), styleIndex, false);
            }
            else
                return _IwUICreateInlineStyle(m_Element, styleName.c_str());
        }

        IwAssertMsg(VIEWER, false,
                    ("Failed to locate property set for: '%s'", DebugGetName()));
    }

    return NULL;
}

const CIwUIPropertySet* CElementContainer::GetPropertySet() const
{
    if (m_Element)
    {
        if (!IsContainer())
            return &m_Element->GetPropertySet();
        else if (m_ChildName.find('^') == 0)
        {
            std::string styleName = m_ChildName.substr(1);

            // Is are inline style in a list?
            int indexPos = styleName.find('[');
            if (indexPos != std::string::npos)
            {
                int styleIndex = GetStyleIndex(styleName);

                CIwPropertyList<CIwUIStyle> styleList;
                if (m_Element->GetProperty(styleName.c_str(), styleList))
                {
                    if (0 <= styleIndex && styleIndex < styleList.size())
                    {
                        CIwUIStyle style = styleList[styleIndex];
                        if (style.HasInlinePropertySet())
                            return style.GetInlinePropertySet();
                    }
                }
            }
            else
            {
                CIwUIStyle style;
                if (m_Element->GetProperty(styleName.c_str(), style))
                    if (style.HasInlinePropertySet())
                        return style.GetInlinePropertySet();

            }
        }

        IwAssertMsg(VIEWER, false,
                    ("Failed to locate property set for: '%s'", DebugGetName()));
    }

    return NULL;
}

const char* CElementContainer::DebugGetName() const
{
    return IsContainer() ? m_ChildName.c_str() :
           m_Element ? m_Element->DebugGetName() : "";
}

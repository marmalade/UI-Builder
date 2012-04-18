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
#ifndef ELEMENT_CONTAINER_H
#define ELEMENT_CONTAINER_H

// Library includes
#include "IwUIElement.h"
#include "IwUIElementPtr.h"
#include "IwUIPropertySet.h"
#include <string>

// Forward declarations
class CIwUIPropertySet;

//-----------------------------------------------------------------------------

class CElementContainer
{
public:
    CElementContainer(CIwUIElement* pElement = NULL);

    // Public interface
    void Set(CIwUIElement* pElement);
    CIwUIElement* Get() const;
    operator bool() const { return Get() != NULL; }

    bool operator ==(const CElementContainer& rhs) const;
    bool operator !=(const CElementContainer& rhs) const;

    CIwUIElement* GetContainer() const { return m_Element; }

    template <class Type>
    void SetProperty(const char* pProperty, const Type& val)
    {
        if (CIwUIPropertySet* pPropertySet = GetPropertySet())
        {
            // Set property
            pPropertySet->SetProperty(pProperty, val);

            // Inform element property has changed
            m_Element->NotifyPropertyChanged();
        }
    }

    template<class Type>
    bool GetProperty(const char *pName, Type& val, bool allowMissing = false)
    {
        if (const CIwUIPropertySet* pPropertySet = GetPropertySet())
            return pPropertySet->GetProperty(pName, val, allowMissing);

        return false;
    }

    CIwUIPropertySet* GetPropertySet();
    const CIwUIPropertySet* GetPropertySet() const;

    const char* DebugGetName() const;

private:
    // Private utils
    bool IsContainer() const { return m_ChildName.length() != 0; }

    // Member data
    CIwUIElementPtr m_Element;
    std::string m_ChildName;
};

#endif

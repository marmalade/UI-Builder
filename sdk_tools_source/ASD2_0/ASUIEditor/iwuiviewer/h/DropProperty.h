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
#ifndef DROP_PROPERTY_H
#define DROP_PROPERTY_H

// Project includes
#include "ElementContainer.h"

// Library includes
#include <string>
#include <vector>

//-----------------------------------------------------------------------------

class CDropProperty : public CIwUIElement
{
public:
    IW_MANAGED_DECLARE(CDropProperty);

    CDropProperty(CIwUIElement* pPropertyElement, const char* pProperty,
                  CIwUIElement* pBase);
    CDropProperty(const char* pStyle, CIwUIElement* pBase);
    virtual ~CDropProperty();

    void RestoreOriginalState();

private:
    // Private structure
    struct StyleDrawable
    {
        StyleDrawable(const std::string& styleName, IIwUIDrawable* pDrawable) :
                       m_StyleName(styleName), m_Drawable(pDrawable) { }

        std::string    m_StyleName;
        IIwUIDrawable* m_Drawable;
    };

    // IwUIElement virtuals
    virtual void OnPosChanged();
    virtual void Activate(bool val);
    virtual CIwVec2 MeasureElement(const CIwVec2& availableSize) const;
    virtual void OnSizeChanged();
    virtual void RenderElement(CIwUIGraphics& graphics);

    // Private utils
    void _Setup();

    void _ApplyProperty(CElementContainer& element);
    void _RestoreProperty();

    void _ApplyStyle(CElementContainer& element);
    void _RestoreStyle();

    CIwUIRect _GetDrawableRect(const StyleDrawable& styleDrawable);
    void _CreateDrawable(const char* pStyleName, const CIwPropertySet* pDrawableStyle);
    void _CreateStyleDrawables(const CIwPropertySet* pRootPropertySet,
                               const CIwPropertySet* pPropertySet);
    void _CreateDrawables();
    void _DestroyDrawables();

    CIwPropertyBase* _CloneSourceProperty() const;

    // Member data
    CIwUIElementPtr m_PropertyElement;          // Element we're cloning property from
    CIwUIElementPtr m_BaseElement;              // Root element to look for elements in

    CElementContainer m_PreviewElement;         // Element we're previewing property on

    std::string m_PropertyName;                 // Name of property we're applying
    CIwPropertyBase* m_OriginalProperty;        // Hold onto original property so can restore
    std::string m_ReplacementName;              // Property we replaced (can refer to style)

    std::string m_StyleName;                    // Name of style we're applying
    CIwUIStyle m_OriginalStyle;                 // Hold onto original style so can restore
    std::vector<StyleDrawable> m_StyleDrawables; // Preview drawables we render
};

#endif

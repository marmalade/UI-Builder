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
#include "DropProperty.h"

// Project includes
#include "Util.h"

// Library includes
#include "IwPropertyString.h"
#include "IwUIGraphics.h"
#include "IwUILayout.h"

#include "IwUIAlertDialog.h"
#include "IwUICheckbox.h"
#include "IwUIDrawableImage.h"
#include "IwUILabel.h"
#include "IwUIPickerView.h"
#include "IwUIProgressBar.h"
#include "IwUISlider.h"

#include <algorithm>
#include <utility>

// Macros
IW_MANAGED_IMPLEMENT(CDropProperty);

//-----------------------------------------------------------------------------

namespace
{

const CIwUIPropertySet* GetElementPropertySet(const CElementContainer& element)
{
    IwAssert(VIEWER, element);

    return element ? element.GetPropertySet() : NULL;
}

CIwUIPropertySet* GetElementPropertySet(CElementContainer& element)
{
    IwAssert(VIEWER, element);

    return element ? element.GetPropertySet() : NULL;
}

CIwPropertyBase* GetElementProperty(const CElementContainer& element,
                                    const char* pPropertyName, bool checkParent = true)
{
    if (const CIwUIPropertySet* pPropertySet = GetElementPropertySet(element))
    {
        return const_cast<CIwPropertyBase*>(
                   pPropertySet->GetPropertyByHash(IwHashString(pPropertyName),
                                                   checkParent));
    }

    return NULL;
}

void AddElementProperty(CElementContainer& element, CIwPropertyBase* pProperty)
{
    IwAssert(VIEWER, element && pProperty);

    if (CIwUIPropertySet* pPropertySet = GetElementPropertySet(element))
        pPropertySet->AddProperty(pProperty);
}

void RemoveElementProperty(CElementContainer& element, CIwPropertyBase* pProperty)
{
    IwAssert(VIEWER, element && pProperty);

    if (CIwUIPropertySet* pPropertySet = GetElementPropertySet(element))
        pPropertySet->RemoveProperty(pProperty);
}

bool CheckElementOwnsProperty(const CElementContainer& element,
                              const CIwPropertyBase* pProperty)
{
    if (const CIwUIPropertySet* pPropertySet = GetElementPropertySet(element))
    {
        for (int i=0; i<(int)pPropertySet->GetNumProperties(); ++i)
        {
            if (pPropertySet->GetPropertyByIndex(i) == pProperty)
                return true;
        }
    }

    return false;
}

CIwPropertyBase* GetAlternateProperty(CElementContainer& element,
                                      const char* pPropertyName)
{
    const char* pAlternatives[][2] =
    {
        { "material",   "texture" },
        { "texture",    "material" }
    };

    int num = sizeof(pAlternatives)/sizeof(pAlternatives[0]);
    for (int i=0; i<num; ++i)
    {
        if (!strcmp(pPropertyName, pAlternatives[i][0]))
        {
            CIwPropertyBase* pProperty =
                GetElementProperty(element, pAlternatives[i][1]);
            if (pProperty)
                return pProperty;
        }
    }

    return NULL;
}

bool UsesBackgroundStyle(const CIwUIElement* pElement)
{
    // Lovely stuff. Currently no other way to discover this as the
    // element potentially doesn't already have "background" set.
    return
        dynamic_cast<const CIwUIAlertDialog*>(pElement) != NULL ||
        dynamic_cast<const CIwUICheckbox*>(pElement)    != NULL ||
        dynamic_cast<const CIwUILabel*>(pElement)       != NULL ||
        dynamic_cast<const CIwUIPickerView*>(pElement)  != NULL ||
        dynamic_cast<const CIwUIProgressBar*>(pElement) != NULL ||
        dynamic_cast<const CIwUISlider*>(pElement)      != NULL;
}

typedef std::pair<CIwProperty<CIwUIStyle>*, CIwProperty<CIwUIStyle>*> StyleReplacement;

StyleReplacement GetStyleReplacement(CElementContainer& element, const char* pPropertyName)
{
    if ((!strcmp(pPropertyName, "texture") || !strcmp(pPropertyName, "material")) &&
        UsesBackgroundStyle(element.Get()))
    {
        // Could be null
        CIwProperty<CIwUIStyle>* pOriginal = IwSafeCast<CIwProperty<CIwUIStyle>*>(
            GetElementProperty(element, "background"));

        CIwUIStyle originalStyle;
        element.GetProperty("background", originalStyle, true);

        // Clone original style (if present) to make replacement
        CIwUIStyle replacementStyle;
        replacementStyle.Clone(originalStyle);

        // Ensure have inline property set (if not cloned from one)
        if (!replacementStyle.HasInlinePropertySet())
            replacementStyle.InlinePropertySet();

        CIwUIPropertySet* pInlinePropSet = replacementStyle.GetInlinePropertySet();

        // Ensure a drawable type is set
        uint32 drawableType = 0;
        if (!pInlinePropSet->GetProperty("drawableType", drawableType, true))
            pInlinePropSet->SetProperty("drawableType", IwHashString("image"));

        // Wrap up in a property to set on the element
        CIwProperty<CIwUIStyle>* pReplacement =
            new CIwProperty<CIwUIStyle>(IwHashString("background"),
                                        "background", replacementStyle);

        return StyleReplacement(pOriginal, pReplacement);
    }

    return StyleReplacement(NULL, NULL);
}

bool CheckStyleCompatibleWithElement(const CIwUIElement* pElement,
                                     const std::string& styleName)
{
    // Just compares style name and element class.
    // UI Builder actually has more useful information than us about style applicability.
    if (pElement)
    {
        std::string className = pElement->GetClassName();
        std::transform(className.begin(), className.end(), className.begin(), ::tolower);

        std::string baseName = styleName;
        std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::tolower);

        // Take off leading '<'
        int bracketOpen = baseName.find('<');
        if (bracketOpen != std::string::npos)
            baseName = baseName.substr(bracketOpen+1);

        // Take off trailing '>'
        int bracketClose = baseName.rfind('>');
        baseName = baseName.substr(0, bracketClose);

        int firstUnderscore = baseName.find('_');
        if (className.find(baseName.substr(0, firstUnderscore)) != std::string::npos)
            return true;

        int lastUnderscore = baseName.rfind('_');
        if (lastUnderscore != std::string::npos &&
            className.find(baseName.substr(lastUnderscore+1)) != std::string::npos)
            return true;
    }

    return false;
}

}

//-----------------------------------------------------------------------------

CDropProperty::CDropProperty(CIwUIElement* pPropertyElement,
                             const char* pProperty, CIwUIElement* pBase) :
    m_PropertyElement(pPropertyElement),
    m_BaseElement(pBase),
    m_PropertyName(pProperty),
    m_OriginalProperty(NULL)
{
    // Add property element as our child
    CIwUILayout* pLayout = new CIwUILayout;
    SetLayout(pLayout);
    pLayout->AddElement(pPropertyElement);

    _Setup();
}

CDropProperty::CDropProperty(const char* pStyle, CIwUIElement* pBase) :
    m_BaseElement(pBase),
    m_OriginalProperty(NULL),
    m_StyleName(pStyle)
{
    CIwUIStyle style(pStyle);
    IwAssertMsg(VIEWER, style.GetPropertySet(), ("Can't find style: '%s'", pStyle));

    SetStyle(style);

    // So text drawables contain something!
    SetProperty("caption", pStyle);

    _Setup();
}

CDropProperty::~CDropProperty()
{
    delete m_OriginalProperty;
}

// Public interface
void CDropProperty::RestoreOriginalState()
{
    if (m_PropertyName.length())
        _RestoreProperty();

    if (m_StyleName.length())
        _RestoreStyle();

    if (m_PreviewElement)
    {
        // Make element reflect property changes
        m_PreviewElement.Get()->NotifyPropertyChanged();
        m_PreviewElement = NULL;
    }
}

// IwUIElement virtuals
void CDropProperty::OnPosChanged()
{
    CIwVec2 intersectPos = GetPos() + GetSize() / IW_FIXED(2);

    // Possibly restoring properties will destroy element
    CElementContainer interest(
        Util::IntersectElement(m_BaseElement, intersectPos, this));

    if (m_PreviewElement != interest)
        RestoreOriginalState();

    if (interest && !m_PreviewElement)
    {
        if (m_PropertyName.length())
            _ApplyProperty(interest);

        if (m_StyleName.length())
            _ApplyStyle(interest);

        if (m_PreviewElement)
            // Make element reflect property changes
            m_PreviewElement.Get()->NotifyPropertyChanged();
    }
}

void CDropProperty::Activate(bool val)
{
    CIwUIElement::Activate(val);

    if (val)
        _CreateDrawables();
    else
        _DestroyDrawables();
}

CIwVec2 CDropProperty::MeasureElement(const CIwVec2& availableSize) const
{
    if (m_StyleDrawables.size())
        return m_StyleDrawables[0].m_Drawable->Measure(availableSize);

    return CIwVec2::g_Zero;
}

void CDropProperty::OnSizeChanged()
{
    for (int i=0; i<(int)m_StyleDrawables.size(); ++i)
    {
        StyleDrawable& styleDrawable = m_StyleDrawables[i];

        CIwUIRect rect = _GetDrawableRect(styleDrawable);

        styleDrawable.m_Drawable->SetPosition(rect.GetPosition());
        styleDrawable.m_Drawable->SetSize(rect.GetSize());
    }
}

void CDropProperty::RenderElement(CIwUIGraphics& graphics)
{
    CIwUIElement::RenderElement(graphics);

    // Reverse order
    for (int i=(int)m_StyleDrawables.size()-1; i>=0; --i)
    {
        m_StyleDrawables[i].m_Drawable->Draw(graphics);
    }
}

// Private utils
void CDropProperty::_Setup()
{
    // Size to child
    SetSizeToContent(true);

    // Render child half alpha
    SetModulatesColour(true);
    SetColour(CIwUIColour(0xff, 0xff, 0xff, 0x80));
}

void CDropProperty::_ApplyProperty(CElementContainer& element)
{
    // See if element has property we're interested in
    CIwPropertyBase* pOriginalProperty = GetElementProperty(element,
                                                            m_PropertyName.c_str());
    CIwPropertyBase* pReplacementProperty = NULL;

    // Treat null ptr property same as no property at all.
    // Handles case of null texture and material being set (or vice-versa).
    if (pOriginalProperty &&
        pOriginalProperty->isDataType(IwHashString("CIwManaged*")))
        if (!(*(CIwManaged**)pOriginalProperty->ExposeData()))
            pOriginalProperty = NULL;

    if (!pOriginalProperty)
        // Allow replacing "texture" with "material" property (and vice-versa).
        pOriginalProperty = GetAlternateProperty(element, m_PropertyName.c_str());

    if (pOriginalProperty)
        pReplacementProperty = _CloneSourceProperty();
    else
    {
        // Property we're trying to be applying might be more applicable within a style
        // Attempt to find such a style property to replace
        StyleReplacement styleReplacement = GetStyleReplacement(
            element, m_PropertyName.c_str());

        if (styleReplacement.second)
        {
            if (CIwPropertyBase* pBackgroundProperty = _CloneSourceProperty())
            {
                pOriginalProperty = styleReplacement.first;
                pReplacementProperty = styleReplacement.second;

                styleReplacement.second->Get().GetInlinePropertySet()->AddProperty(
                    pBackgroundProperty);
            }
            else
                delete styleReplacement.second;
        }
    }

    if (pReplacementProperty)
    {
        m_PreviewElement = element;
        m_ReplacementName = pReplacementProperty->DebugGetPropertyDefine();

        if (pOriginalProperty &&
            CheckElementOwnsProperty(element, pOriginalProperty))
        {
            m_OriginalProperty = pOriginalProperty;
            RemoveElementProperty(m_PreviewElement, m_OriginalProperty);
        }
        else
            // Element inherited this property from style, so leave alone
            m_OriginalProperty = NULL;

        AddElementProperty(m_PreviewElement, pReplacementProperty);
    }
}

void CDropProperty::_RestoreProperty()
{
    // Restore property to value before preview
    if (m_PreviewElement)
    {
        // Refind property to protect against external changes
        if (CIwPropertyBase* pReplacementProperty = GetElementProperty(
                m_PreviewElement, m_ReplacementName.c_str(), false))
        {
            RemoveElementProperty(m_PreviewElement, pReplacementProperty);

            // No longer attached to an element
            delete pReplacementProperty;
        }

        if (m_OriginalProperty)
            AddElementProperty(m_PreviewElement, m_OriginalProperty);
    }
    else
    {
        // Avoid leak if preview element disappears
        // Presumably m_ReplacementProperty was destroyed with m_PreviewElement
        delete m_OriginalProperty;
    }

    m_OriginalProperty = NULL;
    m_ReplacementName = "";
}

void CDropProperty::_ApplyStyle(CElementContainer& element)
{
    if (CheckStyleCompatibleWithElement(element.Get(), m_StyleName))
    {
        if (CIwUIPropertySet* pPropertySet = GetElementPropertySet(element))
        {
            m_PreviewElement = element;
            m_OriginalStyle = pPropertySet->GetStyle();
            pPropertySet->SetStyle(m_StyleName.c_str());
        }
    }
}

void CDropProperty::_RestoreStyle()
{
    if (m_PreviewElement)
        if (CIwUIPropertySet* pPropertySet = GetElementPropertySet(m_PreviewElement))
            pPropertySet->SetStyle(m_OriginalStyle);

    m_OriginalStyle = CIwUIStyle();
}

static const char* GetDefaultDrawableType(const CIwPropertySet* pDrawableStyle)
{
    // Try to determine a default drawable type based on
    // what properties are set
    const char* pDefaultDrawableType = NULL;

    CIwGxFont* pFont = NULL;
    CIwTexture* pTexture = NULL;
    CIwMaterial* pMaterial = NULL;

    if (pDrawableStyle->GetProperty("font", pFont, true))
        pDefaultDrawableType = "text";
    else if (pDrawableStyle->GetProperty("texture", pTexture, true) ||
             pDrawableStyle->GetProperty("material", pMaterial, true))
    {
        CIwSVec2 border(0, 0);
        if (pDrawableStyle->GetProperty("border", border, true))
            pDefaultDrawableType = "border";
        else
            pDefaultDrawableType = "image";
    }

    return pDefaultDrawableType;
}

CIwUIRect CDropProperty::_GetDrawableRect(const StyleDrawable& styleDrawable)
{
    CIwUIRect rect;

    const std::string& styleName = styleDrawable.m_StyleName;
    const IIwUIDrawable* pDrawable = styleDrawable.m_Drawable;

    bool gotSize = false;
    if (styleName.length())
    {
        // There's some vague convention for size property name
        int styleIndex = styleName.rfind("Style");
        const std::string sizeName = styleName.substr(0, styleIndex) + "Size";

        if (GetPropertySet().GetPropertyByHash(IwHashString(sizeName.c_str())))
        {
            CIwVec2 size;
            if (GetProperty(sizeName.c_str(), size))
            {
                rect.SetSize(size);
                gotSize = true;
            }
        }
        else if (!dynamic_cast<const CIwUIDrawableImage*>(pDrawable))
        {
            CIwVec2 border(0, 0);

            // Don't apply border to these styles
            if (styleName != "background" && styleName != "focus")
                GetProperty("border", border, true);

            rect.SetSize(GetSize() - IW_FIXED(2) * border);
            gotSize = true;
        }
    }

    if (!gotSize)
        rect.SetSize(pDrawable->Measure(GetSize()));

    // Centre drawable
    rect.SetPosition((GetSize() - rect.GetSize()) / IW_FIXED(2));

    return rect;
}

void CDropProperty::_CreateDrawable(const char* pStyleName,
                                    const CIwPropertySet* pDrawableStyle)
{
    if (pDrawableStyle)
    {
        IIwUIDrawable* pDrawable = NULL;
        const CIwVec2 size = GetSize();

        // Explicitly set drawable type?
        uint32 drawableType = 0;
        if (pDrawableStyle->GetProperty("drawableType", drawableType, true))
            pDrawable = IwUICreateDrawable(size, *pDrawableStyle);
        else
        {
            const char* pDefaultDrawableType = GetDefaultDrawableType(pDrawableStyle);

            pDrawable = IwUICreateDrawable(size, *pDrawableStyle, pDefaultDrawableType);
        }

        if (pDrawable)
            // Store
            m_StyleDrawables.push_back(StyleDrawable(pStyleName, pDrawable));
    }
}

void CDropProperty::_CreateStyleDrawables(const CIwPropertySet* pRootPropertySet,
                                          const CIwPropertySet* pPropertySet)
{
    for (int i=0; i<(int)pPropertySet->GetNumProperties(); ++i)
    {
        const CIwPropertyBase* pProperty = pPropertySet->GetPropertyByIndex(i);

        // Is this a style property?
        if (pProperty->isDataType(IwHashString("CIwUIStyle")) &&
            // Check property not overridden by one higher up property-set chain
            pProperty == pRootPropertySet->GetPropertyByHash(pProperty->GetPropertyDefineID()))
        {
            CIwUIStyle style = IwSafeCast<const CIwProperty<CIwUIStyle>*>(pProperty)->Get();

            _CreateDrawable(pProperty->DebugGetPropertyDefine(), style.GetPropertySet());
        }
    }

    if (const CIwPropertySet* pParent = pPropertySet->GetParent())
        _CreateStyleDrawables(pRootPropertySet, pParent);
}

void CDropProperty::_CreateDrawables()
{
    // This codes a bit horrible - trying to create a preview of any style
    const CIwPropertySet* pPropertySet = &GetPropertySet();

    // Create root drawable
    _CreateDrawable("", pPropertySet);

    // Create drawables for contained styles
    _CreateStyleDrawables(pPropertySet, pPropertySet);
}

void CDropProperty::_DestroyDrawables()
{
    while (m_StyleDrawables.size())
    {
        IIwUIDrawable* pDrawable = m_StyleDrawables.back().m_Drawable;
        m_StyleDrawables.pop_back();
        delete pDrawable;
    }
}

CIwPropertyBase* CDropProperty::_CloneSourceProperty() const
{
    IwAssert(VIEWER, m_PropertyElement);

    if (m_PropertyElement)
    {
        const CIwPropertyBase* pSourceProperty =
            m_PropertyElement->GetPropertySet().GetPropertyByHash(
                IwHashString(m_PropertyName.c_str()));

        IwAssertMsg(VIEW, pSourceProperty, ("No source property"));
        if (pSourceProperty)
            return pSourceProperty->Clone();
    }

    return NULL;
}

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
#include "IwUIViewer.h"

// Project includes
#include "IwConfig.h"
#include "IwHostLink.h"
#include "IwUIApp.h"

#include "ClassFactory.h"
#include "Drawing.h"
#include "DropProperty.h"
#include "DropShadow.h"
#include "Parser.h"
#include "Util.h"

// Library includes
#include "IwFile.h"
#include "IwUI.h"
#include "s3ewindows.h"

// Namespace imports
using namespace ClassFactory;
using namespace Drawing;
using namespace Parser;
using namespace Util;

// Externs
extern bool g_IwTextureAssertPowerOf2;
extern bool g_IwUIAssertCloneOverride;
extern bool g_IwUIAssertMissingSlot;

IW_UI_SINGLETON_IMPLEMENT(Viewer)

// warning C4355: 'this' : used in base member initializer list
#pragma warning( disable : 4355 )

//-----------------------------------------------------------------------------

namespace
{

template<class ResType>
struct ResTypeTraits
{
    static const char* s_Name;
};

const char* ResTypeTraits<CIwGxFont*>::s_Name = IW_GX_RESTYPE_FONT;
const char* ResTypeTraits<CIwTexture*>::s_Name = IW_GX_RESTYPE_TEXTURE;
const char* ResTypeTraits<CIwMaterial*>::s_Name = IW_GX_RESTYPE_MATERIAL;
const char* ResTypeTraits<CIwUIElement*>::s_Name = IW_UI_RESTYPE_ELEMENT;

template<class ResType>
ResType GetResNamed(CIwResGroup* pResGroup,
                    const char* pResName, bool allowMissing = false)
{
    if (pResGroup)
    {
        uint32 flags = allowMissing ? IW_RES_PERMIT_NULL_F : 0;

        const char* pResType = ResTypeTraits<ResType>::s_Name;

        return IwSafeCast<ResType>(pResGroup->GetResNamed(
                                       pResName, pResType, allowMissing));
    }

    return NULL;
}

}

//-----------------------------------------------------------------------------

CIwUIViewer::CIwUIViewer(CIwUIApp* pApp, CIwHostLink* pHostLink, CIwConfig* pConfig) :
    m_App(pApp),
    m_HostLink(pHostLink),
    m_Config(pConfig),
    m_Input(*this),
    m_ResGroup(NULL),
    m_Palette(NULL),
    m_PaletteMode(PALETTE_VOID),
    m_IsDragging(false),
    m_DragOffset(0, 0),
    m_IsSizing(false),
    m_SizeHandle(0),
    m_IsSelecting(false),
    m_DebugRenderFlags(0),
    m_AntCount(0),
    m_SerialiseCount(0),
    m_SerialiseTime(0)
{
    IW_UI_SINGLETON_SET(Viewer);

    _ConfigureAsserts(true);

    _SetSerialiseCallback(true);

    _ChangeDataDir();

    _SetWritePathPrefix(true);

    RegisterClasses();

    RegisterCustomClasses();

    _LoadResources();

    _ChooseStyleSheet();

    _CreateSelection();
}

CIwUIViewer::~CIwUIViewer()
{
    IW_UI_SINGLETON_CLEAR(Viewer);

    DestroyElement(m_DropProperty);
    DestroyElement(m_DropShadow);

    UnRegisterClasses();

    _DestroyResources();

    _SetWritePathPrefix(false);

    _SetSerialiseCallback(false);

    _ConfigureAsserts(false);
}

void CIwUIViewer::SetStyleSheet(const char* pResName)
{
    if (pResName)
        IwGetUIStyleManager()->SetStylesheet(pResName);
    else
        IwGetUIStyleManager()->SetStylesheet((CIwUIStylesheet*) NULL);
}

void CIwUIViewer::SetOutlineFlags(bool elements, bool layouts, bool siblings,
                                  bool hierarchy, bool margins)
{
    // No separate option for layouts, use hierarchy setting
    layouts = hierarchy;

    m_DebugRenderFlags =
        (elements   ? IW_UI_DEBUG_ELEMENT_OUTLINE_F : 0) |
        (layouts    ? IW_UI_DEBUG_LAYOUT_OUTLINE_F | IW_UI_DEBUG_LAYOUT_ITEM_CONTAINER_OUTLINE_F : 0) |
        (siblings   ? IW_UI_DEBUG_SIBLINGS_F : 0) |
        (hierarchy  ? IW_UI_DEBUG_HIERARCHY_F : 0) |
        (margins    ? IW_UI_DEBUG_DRAWABLE_OUTLINE_F : 0 );
}

void CIwUIViewer::SetLocalisation(const char* pLocaliseDat)
{
    if (pLocaliseDat)
    {
        std::string localiseDat(pLocaliseDat);
        localiseDat = localiseDat.substr(m_DataDir.length());

        m_Localise.SetLocalisation(localiseDat.c_str());
    }
    else
        m_Localise.Clear();

    IwGetUIView()->Reactivate();
}

void CIwUIViewer::AddSelection(const char* pSelection)
{
    std::vector<std::string> selectionPath;
    SplitSeparatedStrings(pSelection, selectionPath);

    CIwUIElement* pElement = FindElementByPath(selectionPath);
    _AddSelection(pElement);
}

void CIwUIViewer::RemoveSelection(const char* pSelection)
{
    std::vector<std::string> selectionPath;
    SplitSeparatedStrings(pSelection, selectionPath);

    CIwUIElement* pElement = FindElementByPath(selectionPath);
    _RemoveSelection(pElement);
}

void CIwUIViewer::ParseUpdate(const char* pData)
{
    bool result = false;

    std::vector<std::string> lines;
    SplitSeparatedStrings(pData, lines, '\n');

    if (lines.size() >= 2)
    {
        std::string className;
        bool foundClass = FindAttribute("class", lines[0], className);

        std::string objectName;
        bool foundName = FindAttribute("name", lines[1], objectName);

        if (foundClass && foundName)
        {
            if (className == "CIwUIPropertySet" ||
                className == "CIwPropertySet")
            {
                if (CIwPropertySet* pPropertySet = const_cast<CIwPropertySet*>(
                        IwGetUIStyleManager()->GetPropertySet(objectName.c_str())))
                {
                    // Seem to not always get all attributes, so don't call
                    // ParseOpen / Close so existing properties not cleared out.
                    CTextParserAttributes::ParseAttributes(pPropertySet, pData, false);

                    IwGetUIStyleManager()->BuildHierarchy();

                    result = true;
                }
            }
            else if (className == "CIwMaterial")
            {
                if (CIwMaterial* pMaterial = GetResNamed<CIwMaterial*>(
                        m_ResGroup, objectName.c_str()))
                {
                    CTextParserAttributes::ParseAttributes(pMaterial, pData);

                    result = true;
                }
            }
            else if (CIwUIElement* pSelection = _GetSelected())
            {
                // TODO: Handle nested layouts
                CIwUILayout* pLayout = pSelection->GetLayout();

                if (className == pSelection->GetClassName() &&
                    objectName == pSelection->DebugGetName())
                {
                    CTextParserAttributes::ParseAttributes(pSelection, pData);

                    pSelection->NotifyPropertyChanged();

                    result = true;
                }
                else if (pLayout &&
                         className == pLayout->GetClassName() &&
                         objectName == pLayout->DebugGetName())
                {
                    CTextParserAttributes::ParseAttributes(pLayout, pData);

                    // No NotifyPropertyChanged for layouts
                    IwGetUIView()->Reactivate();

                    result = true;
                }
            }
            else
                IwAssertMsg(VIEWER, false, ("Failed to parse %s : %s",
                                            className.c_str(), objectName.c_str()));
        }
    }

    IwAssertMsg(VIEWER, result, ("Unable to parse update"));
}

void CIwUIViewer::UpdateElements(const char* pRoot, const char* pFile,
                                 const char* pSelection, UpdateMode mode, const char* pPath)
{
    switch (mode)
    {
    case LOAD_ELEMENTS_FROM_UI:
        // pSelection is path to element separated by |
        _LoadBaseElement(pRoot, pFile, pSelection);
        break;
    case LOAD_STYLE_CONTENTS:
        _LoadStyle(pRoot, pFile);
        break;
    case LOAD_MATERIAL_CONTENTS:
        _LoadMaterial(pRoot, pFile, pPath);
        break;
    case LOAD_RESOURCE_FROM_GROUP:
    default:
        IwAssertMsg(VIEWER, false, ("Unrecognised element update"));
        break;
    }
}

CIwUIElement* CIwUIViewer::_FindPaletteTemplate(const std::string& name) const
{
    // Expect string of the form:
    // "ExpMetaPalette:<mode>/<group> <element>"
    std::vector<std::string> path;
    SplitQuotedStrings(name, path);

    if (path.size() == 2)
    {
        CIwUIElement* pGroup = GetResNamed<CIwUIElement*>(m_Palette, path[0].c_str());

        if (pGroup)
            if (CIwUIElement* pTemplate = pGroup->GetChildNamed(path[1].c_str(), false))
                return pTemplate;

    }

    IwAssertMsg(VIEWER, false, ("Unable to find palette element: %s", name.c_str()));
    return NULL;
}

CIwUIViewer::PaletteMode CIwUIViewer::_GetPaletteMode(const std::string& name) const
{
    PaletteMode mode = PALETTE_ELEMENT;

    size_t modePos = name.find(':');
    if (modePos != std::string::npos)
    {
        char modeChar = name[modePos + 1];
        switch (tolower(modeChar))
        {
        case 'l':
            mode = PALETTE_LAYOUT;          break;
        case 'f':
            mode = PALETTE_FOCUS_HANDLER;   break;
        case 's':
            mode = PALETTE_LAYOUT_SPACER;   break;
        case 'p':
            mode = PALETTE_SIZE_POLICY;     break;
        default:
            IwAssertMsg(VIEWER, false, ("Unrecognised palette mode: %c", modeChar));
            break;
        }
    }

    return mode;
}

void CIwUIViewer::UpdatePaletteDrag(DragMode dragMode, int posX, int posY, const char* pText)
{
    if (!m_Palette)
    {
        IwAssertMsg(VIEWER, false, ("No palette loaded"));
        return;
    }

    const CIwVec2 pos(posX, posY);
    m_Input.SetPointerPos(pos);

    bool movePaletteElements = false;

    switch (dragMode)
    {
    case DRAG_DROP:
    {
        _DestroyDropShadow();
        _DestroyDropProperty(false);

        if (m_PaletteMode == PALETTE_PROPERTY ||
            m_PaletteMode == PALETTE_STYLE)
            // Send back updated property
            _SendUpdate();
        else
            _DropElement(m_PaletteElement, m_PaletteMode);

        m_PaletteElement = NULL;
        m_PaletteMode = PALETTE_VOID;
    }
    break;

    case DRAG_ENTER:
    {
        IwAssertMsg(VIEWER, m_PaletteMode == PALETTE_VOID,
                    ("Already dragging palette element"));

        _DestroyDropShadow();
        _DestroyDropProperty();
        DestroyElement(m_PaletteElement);

        const char* pPaletteFont = "gxfont ";
        const char* pPaletteTexture = "texture ";
        const char* pPaletteMaterial = "material ";
        const char* pPaletteStyle = "style ";

        // Are we dragging a font?
        if (strstr(pText, pPaletteFont) == pText)
        {
            const char* pFontName = pText + strlen(pPaletteFont);
            CIwGxFont* pFont = GetResNamed<CIwGxFont*>(m_ResGroup, pFontName);

            if (pFont)
            {
                m_PaletteMode = PALETTE_PROPERTY;

                CIwUILabel* pLabel = new CIwUILabel;
                pLabel->SetFont(pFont);
                pLabel->SetCaption(pFontName);
                pLabel->SetTextColour(CIwUIColour(IW_GX_COLOUR_BLACK));

                _CreateDropProperty(pLabel, "font");
            }
        }
        // Are we dragging a texture?
        else if (strstr(pText, pPaletteTexture) == pText)
        {
            const char* pTextureName = pText + strlen(pPaletteTexture);
            CIwTexture* pTexture = GetResNamed<CIwTexture*>(m_ResGroup, pTextureName);

            if (pTexture)
            {
                m_PaletteMode = PALETTE_PROPERTY;

                CIwUIImage* pImage = new CIwUIImage;
                pImage->SetTexture(pTexture);

                _CreateDropProperty(pImage, "texture");
            }
        }
        // Are we dragging a material
        else if (strstr(pText, pPaletteMaterial) == pText)
        {
            const char* pMaterialName = pText + strlen(pPaletteMaterial);
            CIwMaterial* pMaterial = GetResNamed<CIwMaterial*>(m_ResGroup, pMaterialName);

            if (pMaterial)
            {
                m_PaletteMode = PALETTE_PROPERTY;

                CIwUIImage* pImage = new CIwUIImage;
                pImage->SetMaterial(pMaterial);

                _CreateDropProperty(pImage, "material");
            }
        }
        // Are we dragging a style?
        else if (strstr(pText, pPaletteStyle) == pText)
        {
            const char* pStyleName = pText + strlen(pPaletteStyle);
            CIwUIStyle style(pStyleName);

            if (style)
            {
                m_PaletteMode = PALETTE_STYLE;

                _CreateDropStyle(pStyleName);
            }
        }
        // Assume we dragging an element/layout from the palette
        else
        {
            CIwUIElement* pTemplate = _FindPaletteTemplate(pText);

            if (pTemplate)
            {
                m_PaletteElement = pTemplate->Clone();
                m_PaletteMode = _GetPaletteMode(pText);

                std::string name = m_PaletteElement->DebugGetName();
                GetUniqueElementName(name);
                m_PaletteElement->SetName(name.c_str());

                _CreateDropShadow(m_PaletteElement, m_PaletteMode == PALETTE_ELEMENT);
            }
        }

        // Initial positioning
        movePaletteElements = true;
    }
    break;

    case DRAG_LEAVE:
        _DestroyDropShadow();
        _DestroyDropProperty(true);
        DestroyElement(m_PaletteElement);
        m_PaletteMode = PALETTE_VOID;
        break;

    case DRAG_MOVE:
        // Handled below
        movePaletteElements = true;
        break;
    }

    if (movePaletteElements)
    {
        CIwVec2 offset = m_PaletteElement ?
                         -m_PaletteElement->GetSize() / IW_FIXED(2) : CIwVec2::g_Zero;
        _PositionDropShadow(pos, offset);

        _PositionDropProperty(pos);

        IwGetUIView()->Layout();
    }
}

void CIwUIViewer::_CreateDropProperty(CIwUIElement* pSource, const char* pProperty)
{
    _DestroyDropProperty();

    if (pSource)
    {
        m_DropProperty = new CDropProperty(pSource, pProperty, m_BaseElement);
        IwGetUIView()->AddElement(m_DropProperty);
    }
}

void CIwUIViewer::_CreateDropStyle(const char* pStyle)
{
    _DestroyDropProperty();

    m_DropProperty = new CDropProperty(pStyle, m_BaseElement);
    IwGetUIView()->AddElement(m_DropProperty);
}

void CIwUIViewer::_DestroyDropProperty(bool restoreOriginalProperty)
{
    if (restoreOriginalProperty)
    {
        CDropProperty* pDropProperty = IwSafeCast<CDropProperty*>(
            m_DropProperty.operator CIwUIElement*());

        if (pDropProperty)
            pDropProperty->RestoreOriginalState();
    }

    DestroyElement(m_DropProperty);
}

void CIwUIViewer::_PositionDropProperty(const CIwVec2& pos)
{
    if (m_DropProperty)
    {
        CIwVec2 offset = -m_DropProperty->GetSize() / IW_FIXED(2);
        m_DropProperty->SetPos(pos + offset);
    }
}

void CIwUIViewer::_CreateDropShadow(CIwUIElement* pSource, bool preferLayoutInsert)
{
    _DestroyDropShadow();

    if (pSource)
    {
        m_DropShadow =
            new CDropShadow(pSource, m_BaseElement, preferLayoutInsert);

        // Don't put in layout so can be positioned absolutely.
        IwGetUIView()->AddElement(m_DropShadow);
    }
}

void CIwUIViewer::_DestroyDropShadow()
{
    DestroyElement(m_DropShadow);
}

void CIwUIViewer::_PositionDropShadow(const CIwVec2& pos, const CIwVec2& offset)
{
    if (m_DropShadow)
    {
        CDropShadow* pDropShadow = IwSafeCast<CDropShadow*>(
            m_DropShadow.operator CIwUIElement*());

        if (pDropShadow)
            pDropShadow->Position(pos, offset);
    }
}

void CIwUIViewer::_DropElement(CIwUIElement* pElement, PaletteMode mode)
{
    if (!pElement)
    {
        IwAssertMsg(VIEWER, false, ("Failed dropping element"));
        return;
    }

    if (!m_BaseElement || IsBaseElement(pElement))
        _SetBaseElement(pElement);
    else if (mode == PALETTE_ELEMENT)
    {
        // Nothing to-do, element already in position
    }
    else
    {
        // Determine behaviour by mode
        CIwUIElement* pTarget = pElement->GetParent();

        if (pTarget)
        {
            switch (mode)
            {
            case PALETTE_LAYOUT:
                if (CIwUILayout* pLayout = pElement->GetLayout())
                {
                    CIwUILayout* pLayoutClone = pLayout->Clone();

                    if (CIwUILayout* pTargetLayout = GetContainingLayout(pElement))
                        // Add child layout to existing layout
                        pTargetLayout->AddLayoutItem(pLayoutClone);
                    else
                        pTarget->ReplaceLayout(pLayoutClone);
                }

                DestroyElement(pElement);
                break;

            case PALETTE_FOCUS_HANDLER:
                if (CIwUIFocusHandler* pFocus = pElement->GetFocusHandler())
                {
                    CIwUIFocusHandler* pFocusClone = pFocus->Clone();
                    pTarget->SetFocusHandler(pFocusClone);
                }

                DestroyElement(pElement);
                break;

            case PALETTE_ELEMENT:
            case PALETTE_LAYOUT_SPACER:
            case PALETTE_SIZE_POLICY:
                IwAssertMsg(VIEWER, false, ("Unrecognised palette mode"));
                DestroyElement(pElement);
                break;
            }
        }
        else
        {
            IwAssertMsg(VIEWER, false, ("Failed to drop element"));
            DestroyElement(pElement);
        }

        // Better not set selected as destroyed element!
        pElement = pTarget;
    }

    _SetSelected(pElement);
    _SendUpdate();
}

void CIwUIViewer::_PositionAndSizeElement(CIwUIElement* pElement,
                                          const CIwUIRect& frame, bool controlSpan)
{
    if (pElement)
    {
        const CIwVec2 pos = frame.GetPosition();
        const CIwVec2 size = frame.GetSize();

        CIwVec2 sizeMin = pElement->GetSizeMin();
        CIwVec2 sizeMax = pElement->GetSizeMax();

        // Set restrictions fist, before setting size
        pElement->SetSizeMin(size);
        pElement->SetSizeMax(size);
        pElement->SetSizeHint(size);

        pElement->SetPos(pos);
        pElement->SetSize(size);

        CIwUILayout* pLayout = GetContainingLayout(pElement);

        if (CIwUILayoutGrid* pGrid = dynamic_cast<CIwUILayoutGrid*>(pLayout))
        {
            // Relax size restrictions
            sizeMin.x = MIN(size.x, sizeMin.x);
            sizeMin.y = MIN(size.y, sizeMin.y);
            pElement->SetSizeMin(sizeMin);

            sizeMax.x = MAX(size.x, sizeMax.x);
            sizeMax.y = MAX(size.y, sizeMax.y);
            pElement->SetSizeMax(sizeMax);

            // Hold shift to control grid span
            if (controlSpan)
                pGrid->SetLayoutItemSpanFromPosSize(m_SizeElement);
        }
        else if (CIwUILayoutForm* pForm = dynamic_cast<CIwUILayoutForm*>(pLayout))
        {
            // Relax size restrictions
            sizeMin.x = MIN(size.x, sizeMin.x);
            sizeMin.y = MIN(size.y, sizeMin.y);
            pElement->SetSizeMin(sizeMin);
            pElement->SetSizeMax(CIwVec2(-1, -1));
            pElement->SetSizeToContent(false);

            pForm->RemoveElement(pElement);
            IwGetUIView()->Layout();
            pForm->InsertElement(pElement, pElement->GetPosAbsolute(), 1);
            IwGetUIView()->Layout();
        }
    }
}

void CIwUIViewer::UpdatePointer(int posX, int posY,
                                bool mouseDown, bool controlDown, bool shiftDown)
{
    m_Input.SetControl(controlDown);
    m_Input.SetShift(shiftDown);
    m_Input.SetPointerPos(CIwVec2(posX, posY));
    m_Input.SetPointerButton(mouseDown);
}

void CIwUIViewer::UpdateKeyboard(int key)
{
    m_Input.SetKey(key);
}

void CIwUIViewer::HandleKey(int keyDown)
{
    // We'd need to utf-8 encode high-ascii
    if (keyDown < 0 || keyDown > 127)
        return;

    if (keyDown == 0x7f)
    {
        // Delete selected elements
        for (int i=0; i<(int)m_Selection.size(); ++i)
        {
            DestroyElement(m_Selection[i].Get());
        }
        m_Selection.clear();

        _SendUpdate();

        return;
    }

    if (m_EditElement.Get())
    {
        CIwPropertyString caption;
        if (m_EditElement.GetProperty("caption", caption, true))
        {
            bool updateCaption = false;
            bool stopEditing = false;
            CIwPropertyString newCaption;

            const char* pCaption = caption.c_str();
            int len = strlen(pCaption);

            if (keyDown == '\b')
            {
                // Backspace
                // TODO Handle utf-8 strings properly
                if (len > 0 && !(pCaption[MAX(0, len-2)] & 0x80))
                {
                    char* pStr = new char[len];
                    memcpy(pStr, pCaption, len-1);
                    pStr[len-1] = '\0';

                    newCaption = pStr;
                    updateCaption = true;
                    delete[] pStr;
                }
            }
            else if (keyDown == 0x1b)
            {
                // Escape (cancel edit)
                newCaption = m_OriginalCaption;
                updateCaption = true;
                stopEditing = true;
            }
            else if (keyDown == '\n' || keyDown == '\r')
                // Enter
                stopEditing = true;
            else if (keyDown >= 32)
            {
                // Printable character
                char* pStr = new char[len+2];
                memcpy(pStr, pCaption, len);
                pStr[len] = (char) keyDown;
                pStr[len+1] = '\0';

                newCaption = pStr;
                updateCaption = true;
                delete[] pStr;
            }

            if (updateCaption)
            {
                m_EditElement.SetProperty("caption", newCaption);
                _SendUpdate();
            }

            if (stopEditing)
                m_EditElement.Set(NULL);
        }
    }
}

void CIwUIViewer::HandleClick(const CIwVec2& pos)
{
    // Don't use containers because we want to be able to edit radio button caption
    CIwUIElement* pSelection = _IntersectBase(pos);

    if (m_Input.GetControl())
    {
        if (_IsSelected(pSelection))
            _RemoveSelection(pSelection);
        else
            _AddSelection(pSelection);
    }
    else
        _SetSelected(pSelection);

    _SendSelection();
}

void CIwUIViewer::HandleDragPress(const CIwVec2& start)
{
    HandleDragRelease(start);

    // Are we pulling on a sizing handle?
    CIwUIElement* pIntersect = _IntersectBase(start, NULL, true);
    if (pIntersect && _IsSelected(pIntersect))
    {
        CIwUIRect frame(pIntersect->GetPosAbsolute(), pIntersect->GetSize());
        int handle = IntersectHandles(frame, start);
        if (handle != HANDLES_NONE)
        {
            m_IsSizing = true;
            m_SizeElement = pIntersect;
            m_SizeHandle = handle;
            m_SizeRect.SetPosition(pIntersect->GetPosAbsolute());
            m_SizeRect.SetSize(pIntersect->GetSize());
            return;
        }
    }

    // Are we dragging on a selected element?
    CIwUIElement* pSelection = _GetSelectedContaining(pIntersect, true);

    // Don't drag base as just added to view
    if (pSelection && pSelection != m_BaseElement)
    {
        // TODO: Drag the entire selection?
        m_IsDragging = true;

        // Hold control to copy element
        bool copyElement = m_Input.GetControl();
        if (copyElement)
        {
            std::string name = pSelection->DebugGetName();
            GetUniqueElementName(name);

            // Remove element from hierarchy temporarily so can clone
            LayoutLocation layoutLocation = RemoveElement(pSelection);

            m_DragElement = pSelection->Clone();
            m_DragElement->SetName(name.c_str());

            RestoreElement(pSelection, layoutLocation);
        }
        else
            m_DragElement = pSelection;

        m_DragOffset = m_DragElement->GetPosAbsolute() - start;

        _CreateDropShadow(m_DragElement, true);
    }
    else
    {
        // Dragging selection box
        m_IsSelecting = true;
        m_SelectionRect.SetPosition(start);
        m_SelectionRect.SetSize(CIwVec2(0, 0));
    }
}

void CIwUIViewer::HandleDragMove(const CIwVec2& move)
{
    if (m_IsDragging)
        // Move shadow (and hence drag element)
        _PositionDropShadow(move, m_DragOffset);

    if (m_IsSizing)
    {
        if (m_SizeElement && m_SizeHandle)
        {
            // Don't use element rect to track sizing rect as can
            // walk across screen due to rounding issues
            m_SizeRect = SizeRectWithHandle(m_SizeRect, m_SizeHandle, move);

            CIwUIRect localFrame = m_SizeRect;
            localFrame.Translate(m_SizeElement->GetPos() - m_SizeElement->GetPosAbsolute());

            bool controlSpan = m_Input.GetShift();
            _PositionAndSizeElement(m_SizeElement, localFrame, controlSpan);
        }
    }

    if (m_IsSelecting)
    {
        // Drag selection box
        m_SelectionRect.SetSize(move - m_SelectionRect.GetPosition());

        CIwUIRect rect(m_SelectionRect);
        rect.Normalise();

        std::vector<CIwUIElement*> contained;
        _IntersectBase(rect, contained);

        m_Selection.clear();
        for (int i=0; i<(int)contained.size(); ++i)
        {
            _AddSelection(contained[i]);
        }
    }
}

void CIwUIViewer::HandleDragRelease(const CIwVec2& end)
{
    if (m_IsDragging)
    {
        _DestroyDropShadow();

        _DropElement(m_DragElement, PALETTE_ELEMENT);

        m_IsDragging = false;
        m_DragElement = NULL;
    }

    if (m_IsSizing)
    {
        if (m_SizeElement)
        {
            m_SizeElement->InvalidateLayout();
            IwGetUIView()->Layout();
        }

        _SendUpdate();

        m_IsSizing = false;
        m_SizeElement = NULL;
        m_SizeHandle = 0;
        m_SizeRect = CIwUIRect::g_Zero;
    }

    if (m_IsSelecting)
    {
        _SendSelection();

        m_IsSelecting = false;
        m_SelectionRect = CIwUIRect::g_Zero;
    }
}

void CIwUIViewer::Shutdown()
{
    m_App->Quit();
}

void CIwUIViewer::Update(int32 deltaMS)
{
    m_HostLink->Update();

    ++m_AntCount;

    // Render before UI
    // Should be fine as we alternate update and render
    CIwVec2 screenSize(IwGxGetScreenWidth(), IwGxGetScreenHeight());
    DrawGrid(screenSize, CIwVec2(10, 10));

    // Ensure we layout before rendering
    IwGetUIView()->Layout();
}

void CIwUIViewer::Render()
{
    const CIwVec2 pos = m_Input.GetPointerPos();

    // Indicate element from palette / being dragged
    CIwUIElement* pDragElement = m_DragElement ? m_DragElement : m_PaletteElement;
    if (pDragElement)
    {
        // Draw outline around element we're dragging
        IwUIDebugRender(pDragElement, IW_UI_DEBUG_ELEMENT_OUTLINE_F, pos);

        uint32 flags = IW_UI_DEBUG_ELEMENT_OUTLINE_F |
                       IW_UI_DEBUG_LAYOUT_OUTLINE_F |
                       IW_UI_DEBUG_LAYOUT_ITEM_CONTAINER_OUTLINE_F;
        IwUIDebugRender(pDragElement->GetParent(), flags, pos);
    }
    else
    {
        if (m_DebugRenderFlags)
        {
            CIwUIElement* pEditElement = m_EditElement.Get();
            uint32 flags = m_DebugRenderFlags;
            if (pEditElement)
                // Rendering of cursor seems to be broken
                flags |= IW_UI_DEBUG_TEXT_CURSOR_F;

            IwUIDebugRender(pEditElement, flags, pos);
        }

        // Display single handle if we're over one (or all otherwise)
        CIwUIElement* pHighlight = _IntersectBase(pos, NULL, true);
        int highlightHandle = HANDLES_NONE;
        if (pHighlight)
        {
            CIwUIRect frame(pHighlight->GetPosAbsolute(), pHighlight->GetSize());

            highlightHandle = IntersectHandles(frame, pos);
            if (highlightHandle == HANDLES_NONE)
                highlightHandle = HANDLES_ALL;
        }

        // Draw ants around selected elements
        for (int i=0; i<(int)m_Selection.size(); ++i)
        {
            if (CIwUIElement* pSelection = m_Selection[i].Get())
            {
                // Display handles for the element we've over
                int showHandles = pSelection ==
                                  pHighlight ? highlightHandle : HANDLES_NONE;

                DrawOutline(pSelection, showHandles, m_AntCount);
            }
        }

        // Draw selection box
        if (m_IsSelecting)
            m_SelectionRect.DebugRender();
    }
}

bool CIwUIViewer::_ReplaceResource(CIwResource* pResource, const char* pResType)
{
    if (!pResource || !m_ResGroup)
        return false;

    if (!pResType)
        pResType = pResource->GetClassName();

    const char* pResName = pResource->DebugGetName();

    ResourceLocation resLoc =
        FindResourceLocation(m_ResGroup, pResName, pResType);

    if (resLoc.first)
    {
        CIwManagedList& resources = resLoc.second->m_Resources;
        CIwResource* pOriginal = resLoc.third;

        resources.RemoveSlow(pOriginal);
        delete pOriginal;

        // Use official route so reference count incremented correctly
        resLoc.first->AddRes(pResType, pResource);

        return true;
    }

    return false;
}

void CIwUIViewer::_LoadBaseElement(const char* pBaseName, const char* pUIFile,
                                   const char* pSelection)
{
    if (!m_ResGroup)
        IwAssertMsg(VIEWER, false, ("No resource group loaded"));

    if (!strlen(pBaseName))
    {
        // No name indicates we deleted the base element
        DestroyElement(m_BaseElement);
        return;
    }

    std::vector<std::string> selectionPath;
    SplitSeparatedStrings(pSelection, selectionPath);

    // We expect a selection starting at the base element
    if (!selectionPath.size() || selectionPath[0] != pBaseName)
    {
        IwAssertMsg(VIEWER, false, ("Unexpected selection"));

        return;
    }

    // Load .ui file
    CIwUIElement* pBase = ParseElement(pUIFile);

    // Check we have an element with the expected name
    if (!pBase || (strcmp(pBase->DebugGetName(), pBaseName) != 0))
    {
        delete pBase;
        IwAssertMsg(VIEWER, false, ("Loaded element didn't have expected name"));
        return;
    }

    if (m_BaseElement)
    {
        // Remove (hence deactivate) base element
        IwGetUIView()->RemoveElement(m_BaseElement);

        // Are we changing base element?
        if (strcmp(m_BaseElement->DebugGetName(), pBaseName) != 0)
        {
            // Replace resource with instance (hoping this will preserve changes)
            if (!_ReplaceResource(m_BaseElement))
                delete m_BaseElement;
        }
        else
            // Destroy base element
            delete m_BaseElement;

        m_BaseElement = NULL;
    }

    // Ensure we don't get a name clash
    IwGetUIView()->DestroyElements();

    // Search for existing element resource to replace
    ResourceLocation resLoc =
        FindResourceLocation(m_ResGroup, pBaseName, IW_UI_RESTYPE_ELEMENT);

    CIwResGroup* pResGroup;
    if (resLoc.first)
    {
        // Destroy old resource
        resLoc.second->m_Resources.RemoveSlow(resLoc.third);

        // Resource manager holds a reference to resources it holds
        if (!resLoc.third->IsCountZero())
            resLoc.third->DecCount();

        delete resLoc.third;

        // Load into previous location
        pResGroup = resLoc.first;
    }
    else
        pResGroup = m_ResGroup;

    pResGroup->AddRes(IW_UI_RESTYPE_ELEMENT, pBase);

    // Clone new base element and display
    pBase = pBase->Clone();
    _SetBaseElement(pBase);

    // Set selection
    {
        CIwUIElement* pSelection = FindElementByPath(selectionPath);
        _SetSelected(pSelection);
    }
}

static bool ReadNameAttribute(const char* pFile, std::string& objectName)
{
    // Check file starts with "name <pRoot>"
    std::vector<std::string> lines;
    if (ReadLinesFromFile(pFile, lines) && lines.size())
        return FindAttribute("name", lines[0], objectName);

    IwAssertMsg(VIEWER, false, ("Failed to read file: %s", pFile));
    return false;
}

void CIwUIViewer::_LoadStyle(const char* pRoot, const char* pFile)
{
    std::string objectName;
    if (!ReadNameAttribute(pFile, objectName) || objectName != pRoot)
    {
        IwAssertMsg(VIEWER, false, ("Unexpected style data"));

        return;
    }

    CIwUIStyle style(pRoot);

    // Looks in current style sheet, then res manager
    CIwPropertySet* pPropertySet = style ? const_cast<CIwPropertySet*>(
        IwGetUIStyleManager()->GetPropertySet(style, true)) : NULL;

    // TODO: Find out where this should really go
    if (style && !pPropertySet && m_ResGroup)
    {
        pPropertySet = new CIwUIPropertySet;
        pPropertySet->SetName(pRoot);
        m_ResGroup->AddRes(IW_UI_RESTYPE_PROPERTY_SET, pPropertySet);
    }

    if (pPropertySet)
    {
        ParseContents(pPropertySet, pFile);

        // Cause elements to reread styles
        IwGetUIStyleManager()->BuildHierarchy();
    }
    else
        IwAssertMsg(VIEWER, false, ("Could not find property set: %s", pRoot));
}

void CIwUIViewer::_LoadMaterial(const char* pRoot, const char* pFile, const char* pPath)
{
    std::string objectName;
    if (!ReadNameAttribute(pFile, objectName) || objectName != pRoot)
    {
        IwAssertMsg(VIEWER, false, ("Unexpected material data"));

        return;
    }

    CIwMaterial* pMaterial = GetResNamed<CIwMaterial*>(m_ResGroup, pRoot, true);

    if (!pMaterial && m_ResGroup)
    {
        pMaterial = new CIwMaterial;
        pMaterial->SetName(pRoot);
        m_ResGroup->AddRes(IW_GX_RESTYPE_MATERIAL, pMaterial);
    }

    if (pMaterial)
    {
        IwGetResManager()->AddLoadPath(pPath);

        ParseContents(pMaterial, pFile);

        IwGetResManager()->ClearLoadPaths();
    }
    else
        IwAssertMsg(VIEWER, false, ("Could not find material: %s", pRoot));
}

void CIwUIViewer::_SendUpdate()
{
    _AddSelection();

    std::string updateMessage(m_BaseElementName);
    updateMessage += "|";

    if (m_BaseElement)
    {
        const char* pUpdateFilename = "_viewertemp//fromviewer.ui";

        CIwTextDecomposer decomposer;

        IwGetUIView()->RemoveElement(m_BaseElement);

        // Can only decompose non-active (not in view) element
        decomposer.DecomposeToFile(pUpdateFilename, m_BaseElement);

        IwGetUIView()->AddElementToLayout(m_BaseElement);

        updateMessage += pUpdateFilename;
    }

    // No update file indicates base element has been deleted
    m_HostLink->ViewerMessage(CIwHostLink::_UPDATED_ELEMENTS, updateMessage.c_str());
}

void CIwUIViewer::_SendSelection()
{
    // Currently only way to update selection is
    // ADD_SELECTION followed by UPDATED_ELEMENTS
    _SendUpdate();
}

void CIwUIViewer::_AddSelection() const
{
    // We should  also be removing elements from selection too?!
    for (int i=0; i<(int)m_Selection.size(); ++i)
    {
        if (CIwUIElement* pSelected = m_Selection[i].Get())
        {
            std::string path;
            Util::GetElementPath(pSelected, path);
            m_HostLink->ViewerMessage(CIwHostLink::_ADD_SELECTION, path.c_str());
        }
    }
}

void CIwUIViewer::_SetBaseElement(CIwUIElement* pBase)
{
    DestroyElement(m_BaseElement);

    if (pBase)
    {
        m_BaseElement = pBase;
        m_BaseElementName = pBase->DebugGetName();

        if (!m_BaseElement->GetParent())
            IwGetUIView()->AddElementToLayout(m_BaseElement);
    }
    else
        IwAssertMsg(VIEWER, false, ("Failed to add base element"));
}

CIwUIElement* CIwUIViewer::_IntersectBase(const CIwVec2& pos,
                                          CIwUIElement* pIgnoreAndDescendants, bool useContainers) const
{
    CIwUIElement* pIntersection = m_BaseElement ?
                                  IntersectElement(m_BaseElement, pos, pIgnoreAndDescendants) : NULL;

    return useContainers ?
           CElementContainer(pIntersection).GetContainer() : pIntersection;
}

void CIwUIViewer::_IntersectBase(const CIwUIRect& rect,
                                 std::vector<CIwUIElement*>& list) const
{
    list.clear();
    if (m_BaseElement)
        // Border / tolerance so we can select along edges
        IntersectElement(m_BaseElement, rect, list, 3);
}

void CIwUIViewer::_SetSelected(CIwUIElement* pElement)
{
    m_Selection.clear();

    _AddSelection(pElement);
}

CIwUIElement* CIwUIViewer::_GetSelected() const
{
    // Require unique selection
    if (m_Selection.size() == 1)
        return m_Selection[0].Get();

    return NULL;
}

void CIwUIViewer::_AddSelection(CIwUIElement* pElement)
{
    if (_IsSelected(pElement))
        // Already selected
        return;

    if (pElement)
    {
        m_Selection.push_back(pElement);

        if (m_Selection.size() == 1)
        {
            m_EditElement.Set(pElement);
            m_OriginalCaption = "";
            m_EditElement.GetProperty("caption", m_OriginalCaption, true);
        }
    }
}

void CIwUIViewer::_RemoveSelection(CIwUIElement* pElement)
{
    if (pElement)
    {
        for (int i=0; i<(int)m_Selection.size(); )
        {
            CElementContainer& selection = m_Selection[i];

            if (selection.Get() == pElement)
                m_Selection.erase(&selection);
            else
                ++i;
        }

        if (m_EditElement.Get() == pElement)
            m_EditElement.Set(NULL);
    }
}

bool CIwUIViewer::_IsSelected(CIwUIElement* pElement) const
{
    if (pElement)
    {
        for (int i=0; i<(int)m_Selection.size(); ++i)
        {
            if (m_Selection[i].Get() == pElement)
                return true;
        }
    }

    return false;
}

CIwUIElement* CIwUIViewer::_GetSelectedContaining(CIwUIElement* pElement,
                                                  bool useContainers) const
{
    if (pElement)
    {
        for (int i=0; i<(int)m_Selection.size(); ++i)
        {
            const CElementContainer& selection = m_Selection[i];
            CIwUIElement* pSelection = useContainers ?
                                       selection.GetContainer() : selection.Get();
            if (!pSelection)
                continue;

            // Use contain because we don't want to drag radio buttons of tab bar
            if (pSelection == pElement || pSelection->IsDescendant(pElement))
                return pSelection;
        }
    }

    return NULL;
}

void CIwUIViewer::_CreateSelection()
{
    // Ask host to tell us to make the current selection
    m_HostLink->ViewerMessage(CIwHostLink::CREATE_SELECTION);
}

void CIwUIViewer::_LoadResources()
{
    _DestroyResources();

    bool success = false;
    const char* pResGroup = m_Config->GetConfig("viewerfile");
    if (pResGroup)
    {
        m_ResGroup = IwGetResManager()->LoadGroup(pResGroup);
        success = m_ResGroup != NULL;
    }

    if (success)
    {
        if (const char* pPalette = m_Config->GetConfig("palettefile"))
        {
            CIwResGroup* pCurrGroup = IwGetResManager()->GetCurrentGroup();

            // Load palette ui file into separate group
            m_Palette = new CIwResGroup;
            m_Palette->SetName("__palette__");

            // Sets current group to load palette into
            IwGetResManager()->AddGroup(m_Palette);

            IwMemBucketPush(IW_MEM_BUCKET_ID_SYSTEM_DEBUG);
            IwGetResManager()->LoadRes(pPalette);
            IwMemBucketPop();

            // Restore previous current group
            IwAssert(VIEWER, m_ResGroup == pCurrGroup);
            IwGetResManager()->SetCurrentGroup(pCurrGroup);
        }
    }

    IwAssertMsg(VIEWER, success, ("Failed to load group: %s", pResGroup));
}

void CIwUIViewer::_DestroyResources()
{
    if (m_ResGroup)
    {
        IwGetResManager()->DestroyGroup(m_ResGroup);
        m_ResGroup = NULL;
    }

    if (m_Palette)
    {
        IwGetResManager()->DestroyGroup(m_Palette);
        m_Palette = NULL;
    }
}

void CIwUIViewer::_ChooseStyleSheet()
{
    CIwUIStylesheet* currSheet=NULL;
    CIwResManager* pResManager = IwGetResManager();
    uint32 numGroups = pResManager->GetNumGroups();
    for (uint32 i=0; i<numGroups; ++i)
    {
        CIwResGroup* pResGroup = pResManager->GetGroup(i);
        if (CIwResList* pResList =
                pResGroup->GetListNamed(IW_UI_RESTYPE_STYLESHEET,IW_RES_PERMIT_NULL_F))
        {
            CIwManagedList& propertySets = pResList->m_Resources;
            IW_MANAGED_LIST_ITERATE(propertySets, itPS)
            currSheet=(CIwUIStylesheet*)*itPS;
        }
    }

    IwGetUIStyleManager()->SetStylesheet(currSheet);
}

void CIwUIViewer::_ChangeDataDir()
{
    bool success = false;
    if (const char* pDataDir = m_Config->GetConfig("viewerpath"))
    {
        m_DataDir = pDataDir;

        s3eExtWin32DataDirChangeFn dataDirChange;
        if (s3eExtGet("Win32DataDirChange", &dataDirChange, sizeof(dataDirChange)) ==
            S3E_RESULT_SUCCESS)
        {
            dataDirChange(pDataDir);
            success = true;
        }
    }

    IwAssertMsg(VIEWER, success, ("Failed to change data dir"));
}

void CIwUIViewer::_ConfigureAsserts(bool setNotClear)
{
    IwAssertRegisterCallback(setNotClear ? _AssertCallback : NULL);

    // Turn off some asserts we're not interested in
    g_IwTextureAssertPowerOf2 = false;
    g_IwUIAssertCloneOverride = false;
    g_IwUIAssertMissingSlot = false;
}

void CIwUIViewer::_SetSerialiseCallback(bool setNotClear)
{
    if (setNotClear)
    {
        IwSerialiseSetCallbackPeriod(0x400);
        IwSerialiseSetCallback(_SerialiseCallback);
    }
    else
    {
        IwSerialiseSetCallback(NULL);
        g_IwSerialiseContext.callbackPeriod = 0;
    }
}

void CIwUIViewer::_SetWritePathPrefix(bool tempNotDefault)
{
    CIwStringL buildStyleCurr = IwGetResManager()->GetBuildStyleCurrName();

    if (tempNotDefault)
    {
        // Ensure all files written out are in viewertemp folder
        static char buffer[0x40];
        sprintf(buffer, "_viewertemp/data-%s", buildStyleCurr.c_str());
        static const char* prefixes[] = { buffer, NULL };

        // Function holds pointer to buffer, hence buffer static
        IwFileReadSetPathGlobals(NULL);
        IwFileReadSetPathPrefixes(prefixes);
        IwFileWriteSetPathPrefix(buffer);
    }
    else
        // Restore write path prefix to usual state
        IwGetResManager()->SetBuildStyle(buildStyleCurr.c_str());
}

s3eErrorShowResult CIwUIViewer::_Assert(const char* pChannel,
                                        const char* pExpr, const char* pFilename, int32 line, const char* pMsg)
{
    // use shorter 1-line error message if available
    const char* pMsgStart = "Message:";
    const char* pOut = strstr(pMsg, pMsgStart);
    if (pOut)
        pOut += strlen(pMsgStart);
    else
        pOut = pMsg;

    bool showAgainNotIgnore = m_HostLink->LogError(pOut);

    // Handy for debugging
    bool stopOnError = false;
    if (stopOnError)
        return S3E_ERROR_SHOW_STOP;
    else
        return showAgainNotIgnore ? S3E_ERROR_SHOW_AGAIN : S3E_ERROR_SHOW_IGNORE;
}

s3eErrorShowResult CIwUIViewer::_AssertCallback(const char* pChannel,
                                                const char* pExpr, const char* pFilename, int32 line, const char* pMsg)
{
    // Ensure we avoid assert inside singleton getter
    if (IwUIViewerExists())
        return IwGetUIViewer()->_Assert(pChannel, pExpr, pFilename, line, pMsg);

    return S3E_ERROR_SHOW_AGAIN;
}

void CIwUIViewer::_Serialise()
{
    if (s3eDeviceCheckQuitRequest())
        // Get out quick
        exit(0);
    else
    {
        int64 now = s3eTimerGetMs();
        if (now - m_SerialiseTime > 250)
        {
            m_SerialiseTime = now;
            ++m_SerialiseCount;

            Drawing::DrawLoadingScreen(m_SerialiseCount);

            s3eDeviceYield();
        }
    }
}

void CIwUIViewer::_SerialiseCallback()
{
    if (IwUIViewerExists())
        IwGetUIViewer()->_Serialise();
}

//-----------------------------------------------------------------------------

int main()
{
    while (!s3eDeviceCheckQuitRequest())
    {
        CIwConfig config;
        CIwHostLink hostLink;
        CIwUIApp app;
        CIwUIViewer viewer(&app, &hostLink, &config);

        app.Run(&viewer);
    }

    return 0;
}

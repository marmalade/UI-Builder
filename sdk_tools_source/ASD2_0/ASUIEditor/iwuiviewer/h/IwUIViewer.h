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
#ifndef IW_UI_VIEWER_H
#define IW_UI_VIEWER_H

// Project includes
#include "ElementContainer.h"
#include "Input.h"
#include "IwUIApp.h"
#include "IwUIRect.h"
#include "Localise.h"

// Library includes
#include "IwUISingleton.h"
#include "IwPropertyString.h"
#include <string>
#include <vector>

// Forward declarations
class CIwConfig;
class CIwHostLink;
class CIwResource;

//-----------------------------------------------------------------------------

class CIwUIViewer : public IIwUIActivity, private IInputHandler
{
public:
    IW_UI_SINGLETON_DECLARE(Viewer)
    CIwUIViewer(CIwUIApp* pApp, CIwHostLink* pHostLink, CIwConfig* pConfig);
    ~CIwUIViewer();

    // Public enums
    enum UpdateMode
    {                                        // Write element to "_viewertemp//temp.ui" and reload
        LOAD_ELEMENTS_FROM_UI           = 0, // Load base element from "//_viewertemp//fromui%d.ui"
        LOAD_STYLE_CONTENTS             = 1, // Load property set from "//_viewertemp//fromui%d.ui"
        LOAD_MATERIAL_CONTENTS          = 2, // Load material from "//_viewertemp//fromui%d.ui"
        LOAD_RESOURCE_FROM_GROUP        = 3, // Load font or texture from "_viewertemp/fromui.group"
    };

    enum DragMode
    {
        DRAG_DROP,
        DRAG_ENTER,
        DRAG_LEAVE,
        DRAG_MOVE
    };

    // Public interface
    void SetStyleSheet(const char* pResName);

    void SetOutlineFlags(bool elements, bool layouts, bool siblings,
                         bool hierarchy, bool margins);

    void SetLocalisation(const char* pLocaliseDat);

    void AddSelection(const char* pSelection);

    void RemoveSelection(const char* pSelection);

    void ParseUpdate(const char* pData);

    void UpdateElements(const char* pRoot, const char* pFile, const char* pSelection,
                        UpdateMode mode, const char* pPath);

    void UpdatePaletteDrag(DragMode dragMode, int posX, int posY, const char* pText);

    void UpdatePointer(int posX, int posY, bool mouseDown, bool controlDown, bool shiftDown);

    void UpdateKeyboard(int key);

    void Shutdown();

protected:
    // IwUIActivity virtuals
    virtual void Update(int32 deltaMS);
    virtual void Render();

    // InputHandler virtuals
    virtual void HandleKey(int key);
    virtual void HandleClick(const CIwVec2& pos);

    virtual void HandleDragPress(const CIwVec2& start);
    virtual void HandleDragMove(const CIwVec2& move);
    virtual void HandleDragRelease(const CIwVec2& end);

private:
    // Private enums
    enum PaletteMode
    {
        PALETTE_VOID,

        PALETTE_PROPERTY,
        PALETTE_STYLE,

        PALETTE_ELEMENT,
        PALETTE_LAYOUT,
        PALETTE_FOCUS_HANDLER,
        PALETTE_LAYOUT_SPACER,
        PALETTE_SIZE_POLICY
    };

    // Private utils
    void _ChangeDataDir();
    void _ConfigureAsserts(bool setNotClear);
    void _SetSerialiseCallback(bool setNotClear);
    void _SetWritePathPrefix(bool tempNotDefault);

    bool _ReplaceResource(CIwResource* pResource, const char* pResType = NULL);

    void _LoadResources();
    void _DestroyResources();
    void _ChooseStyleSheet();

    void _CreateSelection();

    void _LoadBaseElement(const char* pBaseElem, const char* pUIFile, const char* pSelection);
    void _LoadStyle(const char* pRoot, const char* pFile);
    void _LoadMaterial(const char* pRoot, const char* pFile, const char* pPath);

    CIwUIElement* _FindPaletteTemplate(const std::string& name) const;
    PaletteMode _GetPaletteMode(const std::string& name) const;

    void _CreateDropShadow(CIwUIElement* pSource, bool preferLayoutInsert);
    void _DestroyDropShadow();
    void _PositionDropShadow(const CIwVec2& pos, const CIwVec2& offset);

    void _CreateDropProperty(CIwUIElement* pSource, const char* pProperty);
    void _CreateDropStyle(const char* pStyle);
    void _DestroyDropProperty(bool restoreOriginalProperty = false);
    void _PositionDropProperty(const CIwVec2& pos);

    void _DropElement(CIwUIElement* pElement, PaletteMode mode = PALETTE_ELEMENT);
    void _PositionAndSizeElement(CIwUIElement* pElement, const CIwUIRect& frame, bool controlSpan);

    void _SendUpdate();
    void _SendSelection();
    void _AddSelection() const;

    void _SetBaseElement(CIwUIElement* pBase);
    CIwUIElement* _IntersectBase(const CIwVec2& pos,
                                 CIwUIElement* pIgnoreAndDescendants = NULL, bool useContainers = false) const;
    void _IntersectBase(const CIwUIRect& rect, std::vector<CIwUIElement*>& list) const;

    void _SetSelected(CIwUIElement* pElement);
    CIwUIElement* _GetSelected() const;

    void _AddSelection(CIwUIElement* pElement);
    void _RemoveSelection(CIwUIElement* pElement);
    bool _IsSelected(CIwUIElement* pElement) const;
    CIwUIElement* _GetSelectedContaining(CIwUIElement* pElement,
                                         bool useContainers = false) const;

    s3eErrorShowResult _Assert(const char* pChannel, const char* pExpr,
                               const char* pFilename, int32 line, const char* pMsg);
    void _Serialise();

    // Static utils
    static s3eErrorShowResult _AssertCallback(const char* pChannel,
                                              const char* pExpr, const char* pFilename, int32 line, const char* pMsg);
    static void _SerialiseCallback();

    // Member data
    CIwUIApp* m_App;
    CIwHostLink* m_HostLink;
    CIwConfig* m_Config;

    CInput m_Input;
    CLocalise m_Localise;

    CIwResGroup* m_ResGroup;
    CIwResGroup* m_Palette;                 // Keep separate from rest of ui

    CIwUIElementPtr m_BaseElement;
    std::string m_BaseElementName;          // So name recorded even if we delete base element

    std::vector<CElementContainer> m_Selection;

    CIwUIElementPtr m_PaletteElement;       // Element we're dragging from palette
    PaletteMode m_PaletteMode;

    CIwUIElementPtr m_DropProperty;         // Element applying property to element it is over
    CIwUIElementPtr m_DropShadow;           // Element controlling where element will be dropped

    CElementContainer m_EditElement;        // Element who's caption we're editing
    CIwPropertyString m_OriginalCaption;

    bool m_IsDragging;                      // Are we dragging an element?
    CIwUIElementPtr m_DragElement;
    CIwVec2 m_DragOffset;

    bool m_IsSizing;                        // Are we pulling a sizing handle?
    CIwUIElementPtr m_SizeElement;
    int m_SizeHandle;
    CIwUIRect m_SizeRect;

    bool m_IsSelecting;                     // Are we drawing a selection box?
    CIwUIRect m_SelectionRect;

    int m_DebugRenderFlags;
    int m_AntCount;

    int m_SerialiseCount;
    int64 m_SerialiseTime;

    std::string m_DataDir;
};
IW_UI_SINGLETON_ACCESSOR(Viewer);

#endif

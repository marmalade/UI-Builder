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
#ifndef DROP_SHADOW_H
#define DROP_SHADOW_H

// Library includes
#include "IwUIElement.h"

//-----------------------------------------------------------------------------

class CDropShadow : public CIwUIElement
{
public:
    IW_MANAGED_DECLARE(CDropShadow);

    CDropShadow(CIwUIElement* pSource, CIwUIElement* pBaseElement,
                bool preferLayoutInsert);
    virtual ~CDropShadow();

    void Position(const CIwVec2& pos, const CIwVec2& offset);

private:
    // IwUIElement virtuals
    virtual void OnPosChanged();
    virtual void UpdateElement(int32 deltaMS);
    virtual void Render(const CIwUIGraphics& parentGraphics);

    // Private utils
    void _PositionSource();
    void _InsertSource();
    CIwVec2 _GetInsertPos() const;
    bool _InsideForm() const;

    // Member data
    CIwUIElementPtr m_Source;
    CIwUIElementPtr m_BaseElement;
    const bool m_PreferLayoutInsert;
    bool m_SizeToContent;
    CIwVec2 m_OriginalSize;

    CIwVec2 m_InsertOffset;
    CIwVec2 m_LastMovePos;
    bool m_InsertSource;
};

#endif

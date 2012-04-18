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
#include "DropShadow.h"

// Library includes
#include "IwUIGraphics.h"
#include "IwUILayout.h"
#include "IwUILayoutForm.h"
#include "IwUIView.h"

// Project includes
#include "ElementContainer.h"
#include "Util.h"

// Macros
IW_MANAGED_IMPLEMENT(CDropShadow);

//-----------------------------------------------------------------------------

CDropShadow::CDropShadow(CIwUIElement* pSource, CIwUIElement* pBaseElement,
                         bool preferLayoutInsert) :
    m_Source(pSource),
    m_BaseElement(pBaseElement),
    m_PreferLayoutInsert(preferLayoutInsert),
    m_InsertOffset(0, 0),
    m_InsertSource(false),
    m_LastMovePos(-1, -1)
{
    SetUpdatable(true);

    if (m_Source)
    {
        // Ensure we have non-zero size to start with
        if (m_Source->GetSize() == CIwVec2::g_Zero)
            m_Source->SetSize(CIwVec2(50, 50));

        m_SizeToContent = m_Source->GetSizeToContent();
        m_OriginalSize = m_Source->GetSize();

        SetSize(m_OriginalSize);
    }
}

CDropShadow::~CDropShadow()
{

}

void CDropShadow::Position(const CIwVec2& pos, const CIwVec2& offset)
{
    m_InsertOffset = -offset;

    SetPosAbsolute(pos + offset);
}

// IwUIElement virtuals
void CDropShadow::OnPosChanged()
{
    _PositionSource();
}

void CDropShadow::UpdateElement(int32 deltaMS)
{
    if (m_InsertSource)
        _InsertSource();

    // Match our size to source size
    if (m_Source && !m_Source->IsLayoutInvalid())
        SetSize(m_Source->GetSize());
}

void CDropShadow::Render(const CIwUIGraphics& parentGraphics)
{
    if (m_Source)
    {
        CIwUIGraphics shadowGraphics(parentGraphics);

        // Half-alpha
        CIwUIColour colour(0xff, 0xff, 0xff, 0x80);
        shadowGraphics.SetModulateColour(colour);

        // Offset to allow for difference in position
        shadowGraphics.TranslateTransform(GetPos() - m_Source->GetPos());

        m_Source->Render(shadowGraphics);
    }
}

// Private utils
void CDropShadow::_PositionSource()
{
    if (m_Source)
    {
        if (m_BaseElement == m_Source)
        {
            IwAssertMsg(VIEWER, false, ("Source not expected to be base"));
            return;
        }

        const CIwVec2 pos = GetPosAbsolute();
        const CIwVec2 insertPos = _GetInsertPos();

        CIwUIElement* pTarget = NULL;
        if (m_BaseElement)
        {
            pTarget = Util::IntersectElement(m_BaseElement, insertPos, m_Source);

            // Don't drop on radio-button inside tab-bar
            pTarget = CElementContainer(pTarget).GetContainer();
        }

        if (!pTarget)
            pTarget = m_BaseElement;

        // Prefer inserting into layout
        if (m_PreferLayoutInsert && pTarget && !pTarget->GetLayout())
        {
            CIwUIElement* pPotentialTarget = pTarget;
            while (pPotentialTarget)
            {
                if (pPotentialTarget->GetLayout())
                {
                    pTarget = pPotentialTarget;
                    break;
                }

                if (pPotentialTarget == m_BaseElement)
                    // Stop searching at base
                    break;

                // Maybe parent has layout?
                pPotentialTarget = pPotentialTarget->GetParent();
            }
        }

        CIwUIElement* pSourceParent = m_Source->GetParent();

        if (pTarget)
        {
            if (pTarget->GetLayout())
            {
                // Is source already within target layout?
                if (pTarget == pSourceParent && m_Source->IsWithinLayout())
                {
                    // Wait until layout is valid
                    if (!m_Source->IsLayoutInvalid())
                    {
                        bool insideForm = _InsideForm();

                        // Only move occasionally to avoid flicking back and forth
                        // unless we're inside a form, in which case track closely
                        CIwVec2 minMove(GetSize() / IW_FIXED(2));
                        if (insideForm ||
                            ABS(pos.x - m_LastMovePos.x) >= minMove.x ||
                            ABS(pos.y - m_LastMovePos.y) >= minMove.y)
                        {
                            // Move within current layout
                            CIwUILayout* pParentLayout = pSourceParent->GetLayout();
                            pParentLayout->MoveElement(m_Source, insideForm ? pos : insertPos, 1);
                            m_LastMovePos = pos;
                        }
                    }
                }
                else
                {
                    // Change parent to target
                    if (pSourceParent)
                        pSourceParent->RemoveChild(m_Source);

                    pTarget->AddChild(m_Source);

                    // Have to recalculate layout before insert
                    m_InsertSource = true;
                }
            }
            else
            {
                // Target has now layout, just position absolutely
                if (pSourceParent)
                    pSourceParent->RemoveChild(m_Source);

                pTarget->AddChild(m_Source);
                m_Source->SetPosAbsolute(pos);

                // Some elements (slider & progress bar) collapse if set
                // to sizeToContent when not in a layout. Avoid this...
                CIwVec2 measure = m_Source->Measure(
                    CIwVec2(LAYOUT_INFINITY, LAYOUT_INFINITY));
                if (m_SizeToContent && (!measure.x || !measure.y))
                {
                    m_Source->SetSizeToContent(false);
                    m_Source->SetSize(m_OriginalSize);
                }
            }
        }
        else
        {
            // No base element - insert as root
            if (pSourceParent)
                pSourceParent->RemoveChild(m_Source);

            IwGetUIView()->AddElementToLayout(m_Source);
        }
    }
}

void CDropShadow::_InsertSource()
{
    bool retryInsert = false;

    if (m_Source)
    {
        CIwUILayout* pLayout = m_Source->GetParent() ?
                               m_Source->GetParent()->GetLayout() : NULL;

        if (pLayout)
        {
            if (pLayout->IsLayoutInvalid())
                // Come back later
                retryInsert = true;
            else
            {
                const CIwVec2 insertPos = _InsideForm() ?
                                          GetPosAbsolute() : _GetInsertPos();

                // Can insert as layout valid
                pLayout->InsertElement(m_Source, insertPos, 1);
            }
        }
    }

    m_InsertSource = retryInsert;
}

CIwVec2 CDropShadow::_GetInsertPos() const
{
    return GetPosAbsolute() + m_InsertOffset;
}

bool CDropShadow::_InsideForm() const
{
    CIwUILayout* pParentLayout = Util::GetContainingLayout(m_Source);

    return dynamic_cast<CIwUILayoutForm*>(pParentLayout) != NULL;
}

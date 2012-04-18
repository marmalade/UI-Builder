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
#ifndef IW_UIED_MODULE_H
#define IW_UIED_MODULE_H

#define UIED_EXTRA_DATA_ID 1

#define METABASE_FILENAME L"metabase.txt"
#define METATEMPLATE_FILENAME L"expmetatemplate.txt"
#define METAUI_FILENAME L"uimetatemplate.txt"

typedef unsigned int u32;

//--------------------------------------------------------------------------------
class CIwUIEdModule : public CIwModule
{
public:
    CIwASDFilename* m_Root;
    CUIEdPane* m_Pane;
    CIwHost m_Host;
public:
    CIwUIEdModule();
    ~CIwUIEdModule();

    virtual CIwLayoutElement* MakeElement(const wxString& type);
    virtual void PostWindowInit();
    void SetupProjectDlg();
    void SetupProject2();
protected:
    virtual void OnInit();
    void SetupProject();
};

#define CIwTheUIEdModule ((CIwUIEdModule*)(CIwTheApp->m_ExtraData[UIED_EXTRA_DATA_ID]))
#define CIwTheHost (((CIwUIEdModule*)(CIwTheApp->m_ExtraData[UIED_EXTRA_DATA_ID]))->m_Host)

#endif // !IW_UIED_MODULE_H

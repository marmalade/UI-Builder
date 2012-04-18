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
//-----------------------------------------------------------------------------
//notes editor
//--------------------------------------------------------------------------------
#if !defined(IwSETPROJECTDIALOG_H)
#define IwSETPROJECTDIALOG_H

#include "wxInclude.h"
#include "IwStyleSheet/IwStyleSheet.h"

class CIwWinSourceControl;
class CIwProject;

extern CIwProject Project;

//--------------------------------------------------------------------------------
//project settings editor
class CIwSetProjectDialog : public CIwStyleDialog
{
    CIwWinSourceControl& m_SourceControl;
    bool m_bPromptedForDelete;
    bool DoesProjectNameAlreadyExist(const wxString& AddName);
public:
    enum
    {
        CTRLID_PROJECT,
        CTRLID_BROWSE,
        CTRLID_EDIT,
        CTRLID_REMOVE,
        CTRLID_ADD,
        CTRLID_SETTINGS,
        CTRLID_CHECKSC,
        CTRLID_COPY
    };
    wxListBox* m_Project;
    wxTextCtrl* m_SettingsList;

    wxTextCtrl* m_Dir;
    wxCheckBox* SCCheck;
    wxCheckBox* BKCheck;
    wxCheckBox* CLCheck;
    wxComboBox* m_ResBuildStyleList;
    wxString m_LastProject;
    wxString m_FirstProject;
    bool m_CancelPressed;
public:
    CIwSetProjectDialog(wxWindow* Parent, CIwWinSourceControl& pSC);

    void OnProject(wxCommandEvent&);
    void OnBrowse(wxCommandEvent&);
    void OnEdit(wxCommandEvent&);
    void OnRemove(wxCommandEvent&);
    void OnAdd(wxCommandEvent&);
    void OnCheckSC(wxCommandEvent&);
    void OnDone(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    void OnCopy(wxCommandEvent&);

    void ResetResBuildStyle();

    DECLARE_EVENT_TABLE()
};

#endif // IwSETPROJECTDIALOG_H

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
#ifndef IW_ASD_SOURCE_H
#define IW_ASD_SOURCE_H

class CIwASDSourceLayout;

//--------------------------------------------------------------------------------
// CIwASDSourcePanel
//--------------------------------------------------------------------------------
class CIwASDSourcePanel : public wxPanel
{
public:
    enum { CTRLID_SOURCE,CTRLID_SAVE,CTRLID_RELOAD };
public:
    wxSizer* m_Sizer;
    wxStyledTextCtrl* m_Text;
    CIwASDSourceLayout* m_Layout;
    bool m_ReLoaded;
public:
    CIwASDSourcePanel(wxWindow* parent,CIwASDSourceLayout* layout);
    ~CIwASDSourcePanel();

    void OnSaveOn(wxStyledTextEvent& e);
    void OnSaveOff(wxStyledTextEvent& e);

    void OnSave(wxCommandEvent&);
    void OnReload(wxCommandEvent&);
    bool GetText();

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwASDFileDataSource : public CIwASDFileData
{
public:
    wxString m_Buffer;

    CIwASDFileDataSource() : CIwASDFileData(FILEDATATYPE_SOURCE) {}
    virtual void Load(bool force=false);
    virtual void Close() { m_Buffer.clear(); }
    virtual void Save();
};

//--------------------------------------------------------------------------------
// CIwASDSourceLayout
//--------------------------------------------------------------------------------
class CIwASDSourceLayout : public CIwLayoutData
{
protected:
    CIwASDSourcePanel* m_Panel;
    bool m_NeedReset;
public:
    CIwASDFileDataSource* m_Data;

    CIwASDSourceLayout() : m_Panel(NULL),m_NeedReset(false),m_Data(NULL) {}
    ~CIwASDSourceLayout();
    virtual void Load(std::vector<wxString>& argv) {}
    virtual wxSizerItem* Create(wxWindow* parent);
    virtual void DoLayout() { m_Panel->Layout(); }
    virtual wxWindow* GetControl() { return m_Panel; }
    virtual CIwASDData* GetData() { return m_Data; }

    virtual bool CanHandleData(unsigned int type) { return type==FILETYPE_FILE; }
    virtual void RefreshData(CIwASDData* data,bool base,bool Delete);

    virtual void DoCheckForReset() { if (m_NeedReset) Reset();

                                     m_NeedReset=false; }
    virtual void SetData(CIwASDData* data);
    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList);
protected:
    void Reset();
};

#endif // !IW_ASD_TREE_H

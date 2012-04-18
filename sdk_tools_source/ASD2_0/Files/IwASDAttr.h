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
#ifndef IW_ASD_ATTR_H
#define IW_ASD_ATTR_H

class CIwASDAttrLayout;

//--------------------------------------------------------------------------------
// CIwASDAttrPanel
//--------------------------------------------------------------------------------
class CIwASDAttrPanel : public wxPanel
{
    class CShim : public CIwAttrDialog
    {
    public:
        CIwASDAttrPanel* m_Frame;
        CShim() : CIwAttrDialog(NULL,L"<>",0) {}
        virtual void Reset()            { m_Frame->Reset(); }
        virtual void ScheduleReset()    { m_Frame->Refresh();   }
        virtual void SetChanged(bool Changed);
        virtual wxWindow* GetDlgItem(wxWindow* Parent,CIwAttrData* Data,int Num,std::vector<CIwAttrData*>& Alts);
        virtual wxString GetBaseDir(bool fileBase);
    };
    CShim m_Shim;
    CIwAttrDescMgr& m_GameMgrMeta;
public:
    enum { CTRLID_SOURCE,CTRLID_TREE,CTRLID_SAVE,CTRLID_RELOAD,CTRLID_REMOVE,CTRLID_SWAP,CTRLID_OPENEDIT };
public:
    wxSizer* m_Sizer;
    wxSizer* m_AreaSizer;
    CIwStyleButtonBar* m_Bar;

    wxStyledTextCtrl* m_Text;
    wxTreeCtrl* m_Tree;
    wxScrolledWindow* m_PropPanel;
    CIwStyleCtrlGroup* m_PropSizer;
    CIwStyleButton* m_Swap;
    CIwStyleButton* m_OpenEdit;

    CIwAttrInstance* m_Sect;

    CIwASDAttrLayout* m_Layout;
    bool m_ReLoaded;
public:
    CIwASDAttrPanel(wxWindow* parent,CIwASDAttrLayout* layout);
    ~CIwASDAttrPanel();

    void OnSaveOn(wxStyledTextEvent& e);
    void OnSaveOff(wxStyledTextEvent& e);
    virtual void OnItemActivated(wxTreeEvent& event);
    //void OnPopupMenu(wxTreeEvent& event);
    void OnRemove(wxCommandEvent& event);

    void OnSave(wxCommandEvent&);
    void OnReload(wxCommandEvent&);
    void OnSwap(wxCommandEvent&);
    void OnOpenEdit(wxCommandEvent&);
    bool GetText();
    void Reset();
    virtual void ResetProp();
    void Refresh() { Reset(); }
    virtual void OnChanged() {}
    virtual CIwAttrInstance* GetResTemplate() { return NULL; }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwASDFileDataAttr : public CIwASDFileData
{
public:
    class Group : public CIwAttrFileGroup
    {
    public:
        CIwASDFileDataAttr* m_Parent;

        virtual CIwAttrMember* TryGetMember(const wxString& Name,CIwAttrInstance* inst);
        virtual bool SaveExtra(wxTextFile& fp){ return (m_Inst==NULL) ? false : m_Inst->SaveExtra(fp); }
        virtual void BrowseForFile(wxString& fileName);

        wxString MakeAbsolute(const wxString& fileName);
    };
public:
    Group m_Group;

    CIwASDFileDataAttr() : CIwASDFileData(FILEDATATYPE_ATTR) { m_Group.m_Parent=this; }
    virtual void Load(bool force=false);
    virtual void Close() {  }
    virtual void Save();
};

//--------------------------------------------------------------------------------
// CIwASDSourceLayout
//--------------------------------------------------------------------------------
class CIwASDAttrLayout : public CIwLayoutData
{
protected:
    CIwASDAttrPanel* m_Panel;
    bool m_NeedReset;
public:
    CIwASDFileData* m_Data;
    CIwASDFileDataSource* m_DataSource;
    CIwASDFileDataAttr* m_DataAttr;

    CIwASDAttrLayout() : m_Panel(NULL),m_NeedReset(false),m_Data(NULL),m_DataSource(NULL),m_DataAttr(NULL) {}
    ~CIwASDAttrLayout();
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
    virtual void SetData(CIwASDFile* root,bool source);
    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList);
protected:
    virtual void Reset();
};

#endif // !IW_ASD_TREE_H

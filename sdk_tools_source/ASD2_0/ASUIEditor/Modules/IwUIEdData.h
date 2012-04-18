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
#ifndef IWUIEDDATA_H
#define IWUIEDDATA_H

class CIwASDUIEdAttrPanel;
class CIwUIEdProjectFrame;
class CIwUIEdPaletteFrame;
class CIwUIEdMediaFrame;
class CIwHost;
class CUIEdProjectGroup;
class CUIEdProject;
class CUIEdProjectUI;
class CIwUIEdFrame;
class CIwASDUIEdAttrPanel;
class CIwUIEdSourcePanel;
class CUIEdAttrShared;

//--------------------------------------------------------------------------------
class CIwUIEdFileDataSource : public CIwASDData
{
public:
    wxString m_FileName;
    wxString m_Buffer;

    CIwUIEdFileDataSource(wxString fileName) : CIwASDData(FILEDATATYPE_SOURCE),m_FileName(fileName) {}
    virtual void Load(bool force=false);
    virtual void Close() { m_Buffer.clear(); }
    virtual void Save();
};

//--------------------------------------------------------------------------------
enum EIwUIEdFileType
{
    UIEDTYPE_PROJECT=MODULE_TAG(UIED_MODULE_ID,0),
    UIEDTYPE_GROUP,
    UIEDTYPE_UI,
    UIEDTYPE_PANE,
};

//--------------------------------------------------------------------------------
class CUIEdProjectGroupLine
{
public:
    wxString m_Prefix;
    wxString m_Line;
    wxString m_Extra;
    wxString m_Quote;

    wxString m_Name;
    wxString m_FileName;
    wxString m_AltFileName;
    CUIEdProjectGroup* m_Group;
    CUIEdProjectUI* m_UI;
    CIwUIEdFileDataSource* m_Source;
    wxBitmap m_Bmp;

    CUIEdProjectGroup* m_Parent;

    CUIEdProjectGroupLine() : m_Quote(L""),m_Group(NULL),m_UI(NULL),m_Source(NULL) {}
    ~CUIEdProjectGroupLine();
};

//--------------------------------------------------------------------------------
class CUIEdProjectGroup : public CIwASDData
{
public:
    wxString m_FileName;
    std::vector<CUIEdProjectGroupLine*> m_Lines;
    int m_Insert;
    int m_First;
    CUIEdProject* m_Parent;
    CUIEdProjectGroupLine* m_ParentLine;

    CUIEdProjectGroup(const wxString& fileName,CUIEdProject* parent) : CIwASDData(UIEDTYPE_GROUP),m_FileName(fileName),m_Insert(0),m_First(0),m_Parent(parent),
        m_ParentLine(NULL) {}
    ~CUIEdProjectGroup() { for (int i=0; i<(int)m_Lines.size(); i++) {delete m_Lines[i]; }}

    virtual void CheckSave(bool force);
    virtual wxString GetName()
    {
        wxFileName name(m_FileName);
        return name.GetFullName();
    }
    bool AddLine(const wxString& line,bool atEnd=false);
    bool AddLineStart(const wxString& str,int lineNum=-1);
    bool HasFile(const wxString& line);
    wxString MakeFileName(const wxString& line);
    void GetStrings(wxArrayString& strings,const std::vector<wxString>& exts);
    void CheckUsage();
    CUIEdProjectGroupLine* GetLine(CUIEdProjectUI* ui);

    bool SaveFiltered(wxString& fileName,int& num);
};

//--------------------------------------------------------------------------------
class CUIEdProject : public CIwASDData
{
public:
    wxString m_RootDir;
    wxString m_Project;
    wxString m_Group;
    wxString m_PropSet;
    wxString m_Material;
    wxString m_ResTemplate;
    wxString m_StyleSheet;
    wxString m_MetaTemplate;
    wxString m_LocalisationDir;
    wxString m_LocalisationFile;
    bool m_AutoStartViewer;
    bool m_OutlineElements;
    bool m_OutlineLayouts;
    bool m_OutlineHierarchy;
    bool m_OutlineSiblings;
    bool m_OutlineMargins;
    CUIEdProjectGroup* m_GroupData;
    CUIEdProjectUI m_Palette;
    CUIEdProjectUI* m_PropSetData;
    CUIEdProjectUI* m_MaterialData;
    int m_HWSWMode;
    int m_Width,m_Height;

    CUIEdProject() : CIwASDData(UIEDTYPE_PROJECT),
        m_AutoStartViewer(true),m_OutlineElements(false),m_OutlineLayouts(false),
        m_OutlineHierarchy(false),m_OutlineSiblings(false),m_OutlineMargins(false),
        m_GroupData(NULL),m_PropSetData(NULL),m_MaterialData(NULL),
        m_HWSWMode(-1),m_Width(320),m_Height(480) {}
    void Setup(const wxString& rootDir,const wxString& project,const wxString& group);
    void Create(bool fromGroup,const wxString& styleDir=L"");
    void CreateGroup(const wxString& file,wxString styleGroup=L"");
    void CreateUI(const wxString& file);
    void CreateStyleSheet(const wxString& file,const wxString& parent);
    void Init();
    bool Load(const wxString& file);
    CUIEdProjectGroup* LoadGroup(const wxString& file);
    virtual wxString GetName()
    {
        wxFileName name(m_Project);
        return name.GetFullName();
    }
    virtual void CheckSave(bool force) { CheckSave(force,true); }
    void CheckSave(bool force,bool saveData);
    void SetFile(CUIEdProjectUI* file);

    void SetAutoViewer(bool value) { m_AutoStartViewer=value; SetChanged(true); }
    bool GetAutoViewer()
    {
        return m_AutoStartViewer;
    }

    void SetOutlineElements(bool value) { m_OutlineElements=value; SyncFlags(false); }
    bool GetOutlineElements() { return m_OutlineElements; }

    void SetOutlineLayouts(bool value) { m_OutlineLayouts=value; SyncFlags(false); }
    bool GetOutlineLayouts() { return m_OutlineLayouts; }

    void SetOutlineHierarchy(bool value) { m_OutlineHierarchy=value; SyncFlags(); }
    bool GetOutlineHierarchy() { return m_OutlineHierarchy; }

    void SetOutlineSiblings(bool value) { m_OutlineSiblings=value; SyncFlags(); }
    bool GetOutlineSiblings() { return m_OutlineSiblings; }

    void SetOutlineMargins(bool value) { m_OutlineMargins=value; SyncFlags(); }
    bool GetOutlineMargins() { return m_OutlineMargins; }

    void SyncFlags(bool setChanged = true);

    void SyncStyleSheet();

    void SetLocalisationFile(const wxString& file);
    void SyncLocalisation();

private:
    void WriteCustomClassMap(int Slot, const wxString& fileName);
};

//--------------------------------------------------------------------------------
class CUIEdAttrPropSet
{
public:
    CIwAttrInstance* m_OrigInst;
    CIwAttrInstance* m_Inst;
    CUIEdProjectUI* m_UI;
    wxString m_Parent;
    wxString m_StyleSheet;
    wxBitmap m_Bmp;
    int m_Temp;

    CUIEdAttrPropSet(CIwAttrInstance* inst,CUIEdProjectUI* UI) : m_OrigInst(inst),m_Inst(inst),m_UI(UI) {}
    CUIEdAttrPropSet() : m_OrigInst(NULL),m_Inst(NULL),m_UI(NULL) {}
};

//--------------------------------------------------------------------------------
enum EIwUIEdSelSource
{
    SELSOURCE_DATA,
    SELSOURCE_CHANGE,
    SELSOURCE_UIED,
    SELSOURCE_LAYOUT,
    SELSOURCE_START,
    SELSOURCE_VIEWER,
    SELSOURCE_VUPDATE,

    CHANGETYPE_ADD,
    CHANGETYPE_MOVE,
    CHANGETYPE_REMOVE,
    CHANGETYPE_BIG,
    CHANGETYPE_GROUP,
    CHANGETYPE_SETLAYOUT,
    CHANGETYPE_SIZERPOLICY,

    CHANGETYPE_STYLE,
    CHANGETYPE_MAT,

    SELSOURCE_MASK=255,
    SELSOURCE_SCHEDULE=256,
    SELSOURCE_INSERT=512,
};

//--------------------------------------------------------------------------------
class CUIEdAttrSharedExtra : public CIwAttrExtra
{
public:
    CUIEdAttrShared* m_Parent;
public:
    virtual void GetStrings(EIwAttrDialogStringType Type,wxArrayString& Strings,CIwAttrData* Data);
};

//--------------------------------------------------------------------------------
class CUIEdAttrWriteOverrideStylesheetRoot : public CIwAttrFileGroupWriteOverride
{
public:
    bool m_SaveDefault;
public:
    CUIEdAttrWriteOverrideStylesheetRoot() : m_SaveDefault(false) {}
    virtual bool Override(CIwAttrData* data,int num,int Indent,wxString& Notes);
    virtual bool OverrideDefault(CIwAttrData* data,int num,bool initial);
    virtual void Append(CIwAttrData* data,int num,int Indent,wxString& Notes);
};
//--------------------------------------------------------------------------------
class CUIEdAttrWriteOverrideStylesheetInclude : public CIwAttrFileGroupWriteOverride
{
public:
    bool m_SaveDefault;
public:
    CUIEdAttrWriteOverrideStylesheetInclude() : m_SaveDefault(false) {}
    virtual bool Override(CIwAttrData* data,int num,int Indent,wxString& Notes);
    virtual bool OverrideDefault(CIwAttrData* data,int num,bool initial) { return !m_SaveDefault; }
    virtual void Append(CIwAttrData* data,int num,int Indent,wxString& Notes) {}
};

//--------------------------------------------------------------------------------
class CUIEdAttrWriteOverride : public CIwAttrFileGroupWriteOverride
{
public:
    virtual bool Override(CIwAttrData* data,int num,int Indent,wxString& Notes) { return false; }
    virtual bool OverrideDefault(CIwAttrData* data,int num,bool initial)
    {
        if (data->m_Instance->m_Class->m_Name.IsSameAs(L"CIwUIStylesheet",false) && data->m_Member->m_Name.IsSameAs(L"parent",false))
            return data->m_FromDefault;  //if((data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR && data->m_Items.size()>0 && data->m_Items[0].m_Inst==NULL)

        //    return true;
        if (data->m_Member->m_Type&ATTRMEMBER_LIST)
            return data->m_Items.empty();

        return data->m_FromDefault;
    }
    virtual void Append(CIwAttrData* data,int num,int Indent,wxString& Notes) {}
};

//--------------------------------------------------------------------------------
class CUIEdAttrShared
{
public:
    CIwAttrInstance* m_DragInst;
    wxString m_DragMode;
    CIwHost* m_Host;
    std::vector<CIwASDUIEdAttrPanel*> m_Panels;
    int pending,pending2;
    bool m_FromViewer;
    bool m_Selecting;

    CUIEdAttrSharedExtra m_Extra;
    CUIEdProject m_Project;

    CUIEdProjectUI* m_CurrUI;
    CIwAttrInstance* m_SelElem;

    CIwUIEdProjectFrame* m_ProjectFrame;
    CIwUIEdPaletteFrame* m_PaletteFrame;
    CIwUIEdPaletteFrame* m_PaletteFrame2;
    CIwUIEdMediaFrame* m_MediaFrame;
    CIwUIEdFrame* m_ViewerFrame;
    CIwASDUIEdAttrPanel* m_UIEdAttrUIPanel;
    CIwASDUIEdAttrPanel* m_UIEdAttrStylePanel;
    CIwASDUIEdAttrPanel* m_UIEdAttrMaterialPanel;
    CIwUIEdSourcePanel* m_UIEdSourcePanel;
    std::map<wxString,CUIEdAttrPropSet> m_PropSetDict;
    std::map<wxString,CUIEdAttrPropSet> m_MaterialDict;
    std::vector<CUIEdAttrPropSet> m_StyleList;
    std::vector<wxBitmap> m_IconList;
    std::vector<CIwAttrInstance*> m_SelList;

    wxString m_UpdateName;
    wxString m_UpdateFile;
    std::vector<wxString> m_UpdateSel;

    int scheduleSource;
    CIwAttrInstance* scheduleInst;
    CIwAttrInstance* scheduleInstParent;

    //CUIEdAttrWriteOverrideStylesheetRoot m_WriteOverrideRoot;
    //CUIEdAttrWriteOverrideStylesheetInclude m_WriteOverrideInclude;
    CUIEdAttrWriteOverride m_WriteOverride;
    int m_OpenEditor;
public:
    CUIEdAttrShared(CIwHost* host) : m_DragInst(NULL),m_Host(host),pending(0),pending2(0),m_Selecting(false),m_CurrUI(NULL),m_SelElem(NULL),
        m_ProjectFrame(NULL),m_PaletteFrame(NULL),m_PaletteFrame2(NULL),m_MediaFrame(NULL),m_ViewerFrame(NULL),
        m_UIEdAttrUIPanel(NULL),m_UIEdAttrStylePanel(NULL),m_UIEdAttrMaterialPanel(NULL),m_UIEdSourcePanel(NULL),scheduleSource(0),m_OpenEditor(0)
    {
        m_Extra.m_Parent=this;
        CIwTheFileMetaMgr.m_Extra=&m_Extra;
        m_Project.m_Palette.m_Group.m_Override=&m_WriteOverride;
        SetupIcons();
    }
    void SetupIcons();

    void SetCurrent(CIwAttrInstance* inst,bool reset,bool fromViewer=false);
    void SetSelected(CIwAttrInstance* inst,bool reset,bool fromViewer=false);

    bool Select(wxTreeCtrl* tree,wxTreeItemId parent,CIwAttrInstance* inst);
    void Idle();

    CIwAttrInstance* GetSelectedElem(const wxString& name2,bool checkCurr,CIwAttrInstance* base=NULL);

    void NewProject(bool fromGroup);
    void LoadProject();
    void SaveProject();
    void StartViewer();
    void MakeUI(CUIEdProjectGroup* group);

    void ChangeLocalisation();

    void Reset();
    void ResetUI();
    void Refresh();
    void RefreshUI(bool resetViewer);
    void RefreshCurr();

    void LoadUI(CUIEdProjectGroupLine* line,bool uiFile);
    void UnLoadUI();
    void AddPropSets(CUIEdProjectUI* ui);
    void AddPropSet(CIwAttrInstance* inst2,CUIEdProjectUI* ui);
    void AddMaterial(CIwAttrInstance* inst2,CUIEdProjectUI* ui);
    void RemovePropSets(CUIEdProjectUI* ui);
    void RemovePropSets(const wxString& styleSheet);
    void AddStyleSheet(CIwAttrInstance* inst2,CUIEdProjectUI* ui);
    void SetStyleSheet(const wxString& styleSheet);
    void AddPropSetsFromStylesheet(const wxString& styleSheet);

    wxString GetFullName(CIwAttrInstance* inst);
    CIwAttrInstance* GetFromFullName(const wxString& name);
    void SendChanges(CIwAttrInstance* inst);

    void UpdateFromViewer(const wxString& name,const wxString& file,std::vector<wxString>& sel);
    void UpdateFromViewer2();
    void UpdateFromViewer2(std::vector<CIwAttrNote>& notesnew,std::vector<CIwAttrNote>& notesold,int oldnum,int newnum);
    CIwAttrInstance* Remove(CIwAttrInstance* inst);
    CIwAttrInstance* GetFirst();

    void SetSelection(CIwAttrInstance* inst,int source=SELSOURCE_DATA);
    void Change(CIwAttrInstance* inst,int type,CIwAttrInstance* parent=NULL);
    void Signal(int type);
    void Clear();
    void GetStrings(EIwAttrDialogStringType Type,wxArrayString& Strings,CIwAttrData* Data);
    void GetPropertySetStrings(wxArrayString& strings,CIwAttrData* data);
    void GetPropertySetStrings(wxArrayString& strings,CIwAttrClass* klass,bool usesContainer);

    CIwAttrInstance* ToDefault(const wxString& fromStyle);
    CIwAttrInstance* ToDefault(const wxString& fromStyle,const wxString& name,CIwAttrInstance* inst);
    void SaveDefault(CIwAttrInstance* first);
    void ResetPtrs(CUIEdProjectGroup* group);
    void ResetPtrs(CIwAttrInstance* inst);

    void GetFileList(wxArrayString& strings,CUIEdProjectGroup* group,int mode);
    CUIEdProjectUI* GetFile(const wxString& string,CUIEdProjectGroup* group);
    wxBitmap* GetFileBitmap(const wxString& string,CUIEdProjectGroup* group);
    CUIEdProjectGroup* GetFileGroup(const wxString& string,CUIEdProjectGroup* group);
    wxString MakeNew(EIwAttrDialogStringType type,const wxString& klass=L"",const wxString& klass2=L"",const wxString& parent=L"");
    void CheckUsage(CIwAttrInstance* inst);
    void SendSelection(std::vector<CIwAttrInstance*>& add,std::vector<CIwAttrInstance*>& remove);

    void TrimmedSelList(std::vector<CIwAttrInstance*>& list);
};

#endif

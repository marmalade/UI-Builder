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
#ifndef IW_ATTR_DESC_GRID_H
#define IW_ATTR_DESC_GRID_H

#include "IwAttrDesc.h"

class CIwAttrGridCombo;
class CIwAttrGridNumber;

//--------------------------------------------------------------------------------
class wxGridCellChoiceRenderer : public wxGridCellStringRenderer
{
public:
    wxGridCellChoiceRenderer(wxLayoutAlignment border = wxLAYOUT_NONE) : m_border(border) {}
    virtual void Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected);
    virtual wxGridCellRenderer *Clone() const { return new wxGridCellChoiceRenderer; }
private:
    wxLayoutAlignment m_border;
};

//--------------------------------------------------------------------------------
class wxGridCellButtonRenderer : public wxGridCellStringRenderer, IIwAttrDataEditor
{
public:
    wxGridCellButtonRenderer(CIwAttrData* data=NULL) : IIwAttrDataEditor(data) {}
    virtual void Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected);
    virtual wxGridCellRenderer *Clone() const { return new wxGridCellButtonRenderer(m_Data); }
};

//--------------------------------------------------------------------------------
class wxGridCellButtonIconRenderer : public wxGridCellStringRenderer, IIwAttrDataEditor
{
    wxBitmap* m_Bmp;
public:
    wxGridCellButtonIconRenderer(wxBitmap* bmp,CIwAttrData* data=NULL) : IIwAttrDataEditor(data), m_Bmp(bmp) {}
    virtual void Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected);
    virtual wxGridCellRenderer *Clone() const { return new wxGridCellButtonIconRenderer(m_Bmp,m_Data); }
};

//--------------------------------------------------------------------------------
class wxGridCellExpandRenderer : public wxGridCellStringRenderer
{
    std::vector<int> m_Types;
public:
    wxGridCellExpandRenderer(const std::vector<int>& types) : m_Types(types) {}
    virtual void Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected);
    virtual wxGridCellRenderer *Clone() const { return new wxGridCellExpandRenderer(m_Types); }
};

//--------------------------------------------------------------------------------
class wxGridCellTextureRenderer : public wxGridCellRenderer
{
public:
    wxBitmap* m_Bmp;
public:
    wxGridCellTextureRenderer(wxBitmap* bmp) : m_Bmp(bmp) {}
    virtual void Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected);
    virtual wxGridCellRenderer *Clone() const { return new wxGridCellTextureRenderer(m_Bmp); }
    virtual wxSize GetBestSize(wxGrid& grid, wxGridCellAttr& attr, wxDC& dc, int row, int col) { return wxSize(m_Bmp->GetWidth(),m_Bmp->GetHeight()); }
};

//--------------------------------------------------------------------------------
class IIwAttrGridEditor
{
public:
    virtual void Changed()=0;
};

//--------------------------------------------------------------------------------
class CIwAttrGridComboEditor : public wxGridCellChoiceEditor,IIwAttrGridEditor, IIwAttrDataEditor
{
public:
    enum Mode
    {
        MODE_BOOL,
        MODE_3STATE,
        MODE_ENUM,
        MODE_CLASSES,
        MODE_STRINGS
    };
public:
    CIwAttrGridComboEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,const wxArrayString& strings,Mode mode,bool allowOthers=FALSE);
    CIwAttrGridComboEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,Mode mode);
    CIwAttrGridComboEditor(const wxArrayString& strings);
    virtual void Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler);

    virtual void BeginEdit(int row, int col, wxGrid* grid);

    virtual wxGridCellEditor *Clone() const;
    virtual void Changed();
protected:
    wxPoint m_pointActivate;

    Mode m_Mode;
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
};

//--------------------------------------------------------------------------------
class CIwAttrGridNumberEditor : public wxGridCellNumberEditor,IIwAttrGridEditor, IIwAttrDataEditor
{
    int m_Min2,m_Max2;
    bool m_Adjust;
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
public:
    CIwAttrGridNumberEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,int min=-1,int max=-1);
    virtual void Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler);
    void Changed(int pos);
    virtual void Changed();
    CIwAttrGridNumberEditor* Clone() { return new CIwAttrGridNumberEditor(m_Data,m_Num,m_Alts,m_Min2,m_Max2); }
};

//--------------------------------------------------------------------------------
class CIwAttrGridTextEditor : public wxGridCellTextEditor,IIwAttrGridEditor, IIwAttrDataEditor
{
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
public:
    CIwAttrGridTextEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts);
    virtual void Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler);
    virtual void Changed();
    CIwAttrGridNumberEditor* Clone() { return new CIwAttrGridNumberEditor(m_Data,m_Num,m_Alts); }
};

//--------------------------------------------------------------------------------
class CIwAttrGridFloatEditor : public wxGridCellFloatEditor,IIwAttrGridEditor, IIwAttrDataEditor
{
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
public:
    CIwAttrGridFloatEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts);
    virtual void Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler);
    virtual void Changed();
    CIwAttrGridFloatEditor* Clone() { return new CIwAttrGridFloatEditor(m_Data,m_Num,m_Alts); }
};

//--------------------------------------------------------------------------------
/*class CIwAttrGridButtonEditor : public wxGridCellEditor,IIwAttrGridEditor
   {
    CIwAttrData* m_Data;
    int m_Num;
    std::vector<CIwAttrData*> m_Alts;
   public:
    CIwAttrGridButtonEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts);
    virtual void Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler);
    virtual void Changed() {}
    CIwAttrGridButtonEditor* Clone() { return new CIwAttrGridButtonEditor(m_Data,m_Num,m_Alts); }

    virtual void BeginEdit(int row, int col, wxGrid* grid) {}
    virtual bool EndEdit(int row, int col, wxGrid* grid) {}

    virtual void Reset() {}
   };*/

//--------------------------------------------------------------------------------
class CIwAttrGridNumber : public wxSpinCtrl
{
    CIwAttrGridNumberEditor* m_Editor;
    bool m_NullField;
public:
    CIwAttrGridNumber(wxWindow *parent,wxWindowID id,CIwAttrGridNumberEditor* editor,int min,int max) : m_Editor(editor),m_NullField(false)
    {
        Create(parent,id,wxEmptyString,wxDefaultPosition, wxDefaultSize,wxSP_ARROW_KEYS,min,max);
    }
private:
    DECLARE_EVENT_TABLE()
    void OnChange(wxScrollEvent& event)
    {
        m_Editor->Changed(event.GetPosition());
    }
    void OnChangeText(wxCommandEvent& event)
    {
        m_NullField=(event.GetString()==L"");
        m_Editor->Changed();
    }
};

//--------------------------------------------------------------------------------
class CIwAttrGridText : public wxTextCtrl
{
    IIwAttrGridEditor* m_Editor;
public:
    CIwAttrGridText(wxWindow *parent,wxWindowID id,IIwAttrGridEditor* editor) : m_Editor(editor)
    {
        Create(parent,id,wxEmptyString,wxDefaultPosition, wxDefaultSize,wxTE_PROCESS_TAB|wxTE_AUTO_SCROLL|wxNO_BORDER);
    }
private:
    DECLARE_EVENT_TABLE()
    void OnChange(wxCommandEvent& event)
    {
        m_Editor->Changed();
    }
};

//--------------------------------------------------------------------------------
class CIwAttrGridCombo : public wxComboBox
{
    IIwAttrGridEditor* m_Editor;
public:
    CIwAttrGridCombo() : m_Editor(NULL) {}
    CIwAttrGridCombo(wxWindow *parent,wxWindowID id,const wxArrayString& strings,IIwAttrGridEditor* editor,long style=0) : m_Editor(editor)
    {
        Create(parent,id,wxEmptyString,wxDefaultPosition,wxDefaultSize,strings,style);
    }
private:
    DECLARE_EVENT_TABLE()
    void OnChange(wxCommandEvent& event);
};

//--------------------------------------------------------------------------------
class CIwAttrGrid : public wxGrid
{
    class DataItem
    {
    public:
        int m_Offset;
        int m_Indent;
        bool m_Last;
        bool m_Leaf;
        CIwAttrData* m_Data;
        std::vector<CIwAttrData*> m_Alts;
        std::vector<CIwAttrInstance*> m_Bases;
    public:
        DataItem(int offset,int indent,CIwAttrData* data,std::vector<CIwAttrData*> alts) : m_Offset(offset),m_Indent(indent),
            m_Last(true),m_Leaf(true),m_Data(data),m_Alts(alts) {}
        DataItem(int offset,int indent,const std::vector<CIwAttrInstance*>& bases) : m_Offset(offset),m_Indent(indent),m_Last(true),
            m_Leaf(true),m_Data(NULL),m_Bases(bases) {}
    };
    CIwAttrDescMgr& m_Mgr;

    int currRow;
    wxGridCellAttr* m_Even;
    wxGridCellAttr* m_Odd;
    wxGridCellAttr* m_Sect;
    wxGridSelection *m_selTemp;
    std::vector<DataItem*> m_DataList;
    int m_OldCurrRow,m_OldCurrCol;
public:
    bool m_HideInline;
public:
    CIwAttrGrid(wxWindow* parent,CIwAttrDescMgr& mgr);
    ~CIwAttrGrid();
    void Clean();
    void Fill(CIwAttrInstance* inst,wxArrayString* classStrings=NULL,bool basic=false);
private:
    void Fill2(std::vector<CIwAttrInstance*>& bases,wxArrayString* classStrings,int indent,bool basic);
    void AddRow(wxString name,int indent);
    void AddData(CIwAttrData* data,int num,int offset,std::vector<CIwAttrData*>& alts,int indent,bool basic);
    void SetupTree();
public:
    void OnCellLeftClick(wxGridEvent& event);
    void RevertSel()
    {
        if (!m_selTemp) return;

        m_selection=m_selTemp;
        m_selTemp=NULL;
    }
    void Activate(DataItem* data,int row,int col);
    void RemoveData(CIwAttrData* data);
    void MakeButton(CIwAttrInstance* inst,const wxString& textName,int currRow,int currCol);
    void OnMouseMotion(wxMouseEvent& e);
    wxString GetToolTip(int row,int col);

    DECLARE_EVENT_TABLE()
};

#endif

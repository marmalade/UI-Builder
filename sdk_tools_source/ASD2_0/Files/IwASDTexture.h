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
#ifndef IW_ASD_TEXTURE_H
#define IW_ASD_TEXTURE_H

class CIwASDTextureLayout;

//--------------------------------------------------------------------------------
// CIwASDTexturePanel
//--------------------------------------------------------------------------------
class CIwWinSummaryImage : public wxPanel
{
public:
    wxBitmap* m_Bitmap;

    CIwWinSummaryImage(wxWindow* Frame) : wxPanel(Frame),m_Bitmap(NULL) { }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxPaintDC dc(this);
        if (m_Bitmap!=NULL)
            dc.DrawBitmap(*m_Bitmap,0,0);
    }
protected:
    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwASDTexturePanel : public wxPanel
{
public:
    enum { CTRLID_SUMMARYTEXT, CTRLID_NORMAL, CTRLID_ZOOMIN, CTRLID_ZOOMOUT };
public:
    CIwStyleButtonBar* m_ToolBar;
    wxSizer* m_Sizer;
    CIwWinSummaryImage* m_Panel;
    CIwASDTextureLayout* m_Layout;
    //wxTextCtrl* m_Text;
    float m_Zoom;
public:
    CIwASDTexturePanel(wxWindow* parent,CIwASDTextureLayout* layout);
    ~CIwASDTexturePanel();

    void OnNormal(wxCommandEvent& event);
    void OnZoomIn(wxCommandEvent& event);
    void OnZoomOut(wxCommandEvent& event);

    void Update(float adjust);

    DECLARE_EVENT_TABLE()
};


//--------------------------------------------------------------------------------
class CIwASDTextureFile : public CIwASDFile
{
public:
    int x,y,depth;
    wxImage m_Image;
    wxString m_Source;
public:
    CIwASDTextureFile() : CIwASDFile(FILETYPE_TEXTURE) {}
    virtual void Load(int Stage,int path=0);

    virtual bool HasType(unsigned int type) { return (type==FILETYPE_TEXTURE) ? true : CIwASDFile::HasType(type); }
};

//--------------------------------------------------------------------------------
// CIwASDTextureLayout
//--------------------------------------------------------------------------------
class CIwASDTextureLayout : public CIwLayoutData
{
protected:
    CIwASDTexturePanel* m_Panel;
    bool m_NeedReset;
public:
    CIwASDTextureFile* m_Data;

    CIwASDTextureLayout() : m_Panel(NULL),m_NeedReset(false),m_Data(NULL) {}
    ~CIwASDTextureLayout();
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
protected:
    void Reset();
};

#endif // !IW_ASD_TEXTURE_H

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
/*!
    \file UIEdLAyout.h
    \brief ui layout
 */
//--------------------------------------------------------------------------------

#if !defined(UIEDLAYOUT_H)
#define UIEDLAYOUT_H

class CIwASDUIEdAttrPanel;
class CIwUIEdLayoutItemDef;
class CIwUIEdLayoutFrame;

class CIwUIEdFont
{
public:
    struct Kern
    {
        wxChar                                      from;
        wxChar                                      to;
        int                                         amount;

        Kern(wxChar _from,wxChar _to,int _amount) : from(_from),to(_to),amount(_amount) {}
    };
    struct Char
    {
        wxChar            c;
        int               x,y,w,h;

        Char(wxChar _c) : c(_c) {}
    };
public:
    wxString m_Name;
    wxImage m_Image;
    wxString m_CharString;
    int m_KernAll;
    int m_KernDef;
    int m_Leading;
    std::vector<Kern> m_Kernings;
    std::vector<Char> m_CharMap;
public:
    CIwUIEdFont(const wxString& fileName) : m_KernAll(0),m_KernDef(0),m_Leading(0) { Load(fileName); }
    void Load(const wxString& fileName);
    void RenderText(const wxString& text,wxBitmap& bmp,wxColour col);
protected:
    int Find(wxChar c,wxChar prev,int& x,int& y,int& depth);
    void Draw(int num,int x,int y,wxImage& image,wxColour col);
};

//--------------------------------------------------------------------------------
enum CIwUIEdLayoutAlign
{
    MIN_TO_MIN,
    MIN_TO_MID,
    MIN_TO_MAX,
    MID_TO_MIN,
    MID_TO_MID,
    MID_TO_MAX,
    MAX_TO_MIN,
    MAX_TO_MID,
    MAX_TO_MAX,
    NONE,
    FROM_CONTENT,
};
//--------------------------------------------------------------------------------
enum CIwUIEdLayoutAlignFont
{
    FONTALIGN_LEFT=0,
    FONTALIGN_HMID=1,
    FONTALIGN_RIGHT=2,
    FONTALIGN_HMASK=3,

    FONTALIGN_TOP=0,
    FONTALIGN_VMID=4,
    FONTALIGN_BOTTOM=8,
    FONTALIGN_VMASK=12,

    FONTALIGN_LEFT_TOP=FONTALIGN_LEFT|FONTALIGN_TOP,
    FONTALIGN_MID_TOP=FONTALIGN_HMID|FONTALIGN_TOP,
    FONTALIGN_RIGHT_TOP=FONTALIGN_RIGHT|FONTALIGN_TOP,
    FONTALIGN_LEFT_MID=FONTALIGN_LEFT|FONTALIGN_VMID,
    FONTALIGN_MID_MID=FONTALIGN_HMID|FONTALIGN_VMID,
    FONTALIGN_RIGHT_MID=FONTALIGN_RIGHT|FONTALIGN_VMID,
    FONTALIGN_LEFT_BOTTOM=FONTALIGN_LEFT|FONTALIGN_BOTTOM,
    FONTALIGN_MID_BOTTOM=FONTALIGN_HMID|FONTALIGN_BOTTOM,
    FONTALIGN_RIGHT_BOTTOM=FONTALIGN_RIGHT|FONTALIGN_BOTTOM,
};

//--------------------------------------------------------------------------------
enum CIwUIEdLayoutDisplay
{
    DISPLAY_NONE,
    DISPLAY_FILLEDBOX,
    DISPLAY_LINEBOX,
    DISPLAY_LINE,
    DISPLAY_TEXT,
    DISPLAY_BITMAP,
};

//--------------------------------------------------------------------------------
class CIwUIEdLayoutItemDef
{
public:
    wxString m_Name;        //root object: specifies class name
    wxString m_Category;    //where to display this in palette
    CIwUIEdLayoutDisplay m_Type;

    int m_Pos[2];
    int m_Size[2];
    float m_Rotation;
    wxString m_Colour;      //colour string #ffffff or attr reference !attrname
    wxString m_Content;     //text or material or attr reference (!attrname) to text or material
    wxString m_Font;

    std::vector<CIwUIEdLayoutItemDef*> m_Children;
    CIwUIEdLayoutItemDef* m_Parent;
public:
    CIwUIEdLayoutItemDef(const wxString& name,CIwUIEdLayoutDisplay type) : m_Name(name),m_Type(type),m_Font(L"<debug>"),m_Parent(NULL) {}
    CIwUIEdLayoutItemDef* Find(const wxString& name);
};

//--------------------------------------------------------------------------------
class CIwUIEdLayoutItem
{
public:
    enum Flags
    {
        NONE=0,
        INVISIBLE=1,
        HOVERING=2,
        FIXED_SIZE=4,
        PRESET_SIZE=8,
        ROOT_ELEMENT=16,
    };
public:
    int x,y,x1,y1,rot;
    int sel;

    wxColour m_Colour;
    wxBitmap m_Bitmap;
    CIwUIEdFont* m_Font;

    std::vector<CIwUIEdLayoutItem*> m_Children;
    CIwUIEdLayoutItem* m_Parent;

    wxString m_Name;
    int m_Flags;    //Flags

    CIwAttrInstance* m_Inst;
    CIwUIEdLayoutItemDef* m_Def;
    CIwUIEdLayoutFrame* m_Frame;
public:
    CIwUIEdLayoutItem() : x(0),y(0),x1(1),y1(1),sel(0),m_Colour(0,0,0),m_Font(NULL),m_Parent(NULL),m_Flags(NONE),m_Inst(NULL),m_Def(NULL),m_Frame(NULL) {}
    CIwUIEdLayoutItem(CIwUIEdLayoutItem& from);
    CIwUIEdLayoutItem(CIwUIEdLayoutItem* parent,CIwUIEdLayoutItemDef* def,bool palette,CIwAttrInstance* inst=NULL);

    void RenderText(const wxString& text);
    void ResetSize();

    CIwUIEdLayoutItem* Find(const wxString& name);
    CIwUIEdLayoutItem* Find2(const wxString& name);
    CIwUIEdLayoutItem* Find(CIwUIEdLayoutItemDef* def);
    CIwUIEdLayoutItem* Find2(CIwUIEdLayoutItemDef* def);

    CIwUIEdLayoutItem* Select(int xx,int yy);
    void Select(int xx,int yy,int x2,int y2);
    void Select2(int xx,int yy);
    void Move(bool stick[4],int val[4]);
    void GetSelection(std::vector<CIwUIEdLayoutItem*>& sel);

    void SizeChanged();
    void StickyOuterEdge(CIwUIEdLayoutItem* item,bool stick[4],int val[4]);
    void StickyInnerEdge(CIwUIEdLayoutItem* item,bool stick[4],int val[4]);
    void Clear();
    int Index();
    void GetLocation();
    void SetLocation(bool palette);
    void SetDefaultSize();
protected:
    void SetColour();
    void SetText();
    void SetBitmap();
};

//--------------------------------------------------------------------------------
class CIwUIEdBmp
{
public:
    wxString m_Name;
    wxString m_Filename;
    wxBitmap m_Bitmap;
public:
    CIwUIEdBmp(const wxString& name) : m_Name(name) {}
};

//--------------------------------------------------------------------------------
class CIwUIEdLayoutFrame : public wxPanel
{
public:
    CIwUIEdLayoutItem m_List;
    CIwUIEdLayoutItem m_Choose;
    CIwUIEdLayoutItem* m_Curr;
    CIwUIEdLayoutItem* m_CurrChoose;
    std::vector<CIwUIEdFont> m_Fonts;
    std::vector<CIwUIEdBmp> m_Materials;
    std::vector<CIwUIEdLayoutItemDef*> m_Defs;
    std::vector<CIwUIEdLayoutItem*> m_Selection;

    wxSize m_ScreenSize;
    wxSize m_ChooseSize;
    float m_Border;
    int m_DragStartX;
    int m_DragStartY;
    int m_DragStart2X;
    int m_DragStart2Y;
    int m_ChooseScroll;
    bool m_Dragging;
    bool m_Moving;

    CIwASDUIEdAttrPanel* m_Frame;
public:
    CIwUIEdLayoutFrame(wxWindow* parent,CIwASDUIEdAttrPanel* Frame);
    ~CIwUIEdLayoutFrame() { for (int i=0; i<(int)m_Defs.size(); i++) {delete m_Defs[i]; }}

    void OnPaint(wxPaintEvent& WXUNUSED(event));
    void OnLeftDown(wxMouseEvent& e);
    void OnLeftUp(wxMouseEvent& e);
    void OnMotion(wxMouseEvent& e);
    void OnKeyDown(wxKeyEvent& e);

    void Setup(CIwAttrInstance* inst);
    void Setup();
protected:
    void DrawCorner(wxDC& dc,CIwUIEdLayoutItem& item,int corner);
    void DrawLine(wxDC& dc,CIwUIEdLayoutItem& item,int line);
    wxBitmap Draw(CIwUIEdLayoutItem& item);
    void DrawLines(wxDC& dc,CIwUIEdLayoutItem& item);
    void DrawElem(wxDC& dc,wxBitmap& bmp,CIwUIEdLayoutItem& item);

    void Setup(CIwAttrInstance* inst,CIwUIEdLayoutItem* item,bool palette=false);
    int GetValue(CIwAttrData* data,int value,CIwUIEdLayoutItem* item);
    void Changed();
    void Move(int x,int y,int bw,int bh,bool stick[4],wxMouseEvent& e,CIwUIEdLayoutItem* curr);

    void LoadTemplate(const wxString& filename);
    void LoadMaterial(const wxString& filename);
    CIwUIEdLayoutItemDef* FindDef(const wxString& name);
    void LoadPalette(const wxString& filename);
    void ResetSize();
    void SetupRoot();
    void ResetPaletteSize();
    void ResetSel();
    CIwUIEdLayoutItem* Create(CIwUIEdLayoutItem* from);

    DECLARE_EVENT_TABLE()
};

#endif // !defined(UIEDLAYOUT_H)

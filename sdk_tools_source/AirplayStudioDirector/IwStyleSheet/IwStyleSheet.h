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
#ifndef IW_STYLE_SHEET_H
#define IW_STYLE_SHEET_H

#define THE_LOGO IwGetSharedResourceDir()+wxT("bannerLogo.bmp"),wxBITMAP_TYPE_BMP

//--------------------------------------------------------------------------------
//get platform specific resource dir with trailing slash
wxString IwGetResourceDir();
void IwSetResourceDir(const wxString& dir);

//--------------------------------------------------------------------------------
//get SDK identity
wxString IwGetSDKIdent();
wxString IwGetSDKCopyright();

//--------------------------------------------------------------------------------
//draws a banner for the App with Title (ie "Marmalade(TM) Studio") Help Text and logo (ie: \\depot\sdk\main\resources\iwlogosml_55x58.)
class CIwStyleHeader : public wxPanel
{
    wxString m_Title;
    wxString m_Help;
    wxBitmap Bmp;
public:
    CIwStyleHeader(wxWindow* Parent,const wxChar* title=wxT(""),const wxChar* help=wxT("")) :
        wxPanel(Parent,wxID_ANY,wxPoint(-1,-1),wxSize(-1,60)),m_Title(title),m_Help(help)
    {
        Bmp.LoadFile(THE_LOGO);
    }
    void SetText(bool Title,const wxChar* Text)
    {
        if (Title)
            m_Title=Text;
        else
            m_Help=Text;

        Refresh(true);
    }
    void OnPaint(wxPaintEvent& event)
    {
        int pitch=9;
        wxPaintDC wxdc(this);
        wxSize Size=GetClientSize();

        wxdc.SetPen(wxPen(wxColour(0xffffff),0,wxTRANSPARENT));
        wxdc.SetBrush(wxBrush(wxColour(0xffffff)));
        wxdc.DrawRectangle(wxRect(Size));

#ifdef I3D_OS_WINDOWS
        HDC dc=GetHdcOf(wxdc);
        RECT R={0,0,Size.x,Size.y};
        R.bottom--;
        DrawEdge(dc,&R,BDR_RAISEDINNER,BF_BOTTOM);
#else
        wxdc.SetPen(wxPen(wxColour(0xffffff),0,wxTRANSPARENT));
        wxdc.SetBrush(wxBrush(wxColour(0x7f7f7f)));
        wxdc.DrawRectangle(wxRect(0,Size.y-1,Size.x,Size.y));
#endif

        wxdc.DrawBitmap(Bmp,wxPoint(Size.x-Bmp.GetWidth(),0));
        wxdc.SetFont(GetFont(pitch,true));
        wxdc.DrawText(m_Title,18,11);

        wxdc.SetFont(GetFont(pitch,false));

        wxString part1=m_Help;
        wxString part2;
        while (true) {
            wxSize textSize=wxdc.GetTextExtent(part1);
            if (textSize.GetWidth()<Size.x-140)
                break;

            part2=part1.AfterLast(' ')+L" "+part2;
            part1=part1.BeforeLast(' ');
        }
        wxdc.DrawText(part1,36,26);
        wxdc.DrawText(part2,36,41);
    }
    void OnSize(wxSizeEvent& event)
    {
        Refresh(true);
        event.Skip();
    }
    static wxFont GetFont(int Pitch=8,bool Bold=false)
    {
#ifdef I3D_OS_OSX
        Pitch=(int)(Pitch*1.3333f);
#endif
        return wxFont(Pitch,wxFONTFAMILY_DEFAULT,wxFONTSTYLE_NORMAL,Bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,false,wxT("Trebuchet MS"));
    }

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwStyleDialog : public wxDialog
{
public:
    CIwStyleDialog(wxWindow* Parent,const wxChar* Title,const wxSize& Size=wxDefaultSize,long style=wxDEFAULT_DIALOG_STYLE|wxTHICK_FRAME) :
        wxDialog(Parent,wxID_ANY,Title,wxPoint(-1,-1),Size,style)
    {
        SetFont(CIwStyleHeader::GetFont());
        #ifdef I3D_OS_WINDOWS
        SetIcon(wxIcon(wxT("WXDEFAULT_FRAME")));
        #endif
    }
};

//--------------------------------------------------------------------------------
class CIwStyleButton : public wxButton
{
public:
    CIwStyleButton(wxWindow *parent,wxWindowID id,const wxString& label=wxEmptyString,const wxSize& size=wxSize(1,1),long style=0) :
        wxButton(parent,id,label,wxPoint(-1,-1),wxSize((size.x==0) ? 24 : (size.x*72),(size.y==0) ? 24 : (size.y*24)),style)
    {
        SetFont(CIwStyleHeader::GetFont());
    }
};

//CIwStyle<type>OD are owner draw versions of standard controls (used in the exporter, overkill for standalone apps)
//--------------------------------------------------------------------------------
class CIwStyleButtonOD : public CIwStyleButton
{
public:
    bool m_On;
public:
    CIwStyleButtonOD(wxWindow *parent,wxWindowID id,const wxString& label=wxEmptyString,const wxSize& size=wxSize(1,1),long style=0) :
        CIwStyleButton(parent,id,label,size,style),m_On(false) { }

#ifdef I3D_OS_WINDOWS
    void OnPaint(wxPaintEvent& event);
    void OnMouse(wxMouseEvent& event);
#endif

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwStyleLabelOD : public wxStaticText
{
public:
    CIwStyleLabelOD(wxWindow *parent,wxWindowID id,const wxString& label=wxEmptyString,const wxSize& size=wxSize(-1,-1),long style=0) :
        wxStaticText(parent,id,label,wxPoint(-1,-1),size,style) { }

#ifdef I3D_OS_WINDOWS
    void OnPaint(wxPaintEvent& event);
#endif

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwStyleCheckOD : public wxCheckBox
{
public:
    CIwStyleCheckOD(wxWindow *parent,wxWindowID id,const wxString& label=wxEmptyString,const wxSize& size=wxSize(-1,-1),long style=0) :
        wxCheckBox(parent,id,label,wxPoint(-1,-1),size,style) { }

#ifdef I3D_OS_WINDOWS
    void OnPaint(wxPaintEvent& event);
    void OnMouse(wxMouseEvent& event);
#endif

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwStyleNoteBook : public wxNotebook
{
    int m_OverPage;
public:
    CIwStyleNoteBook(wxWindow *parent,wxWindowID id,const wxSize& size=wxSize(-1,-1),long style=0) :
        wxNotebook(parent,id,wxPoint(-1,-1),size,style),m_OverPage(-1) { }

    void OnMouse(wxMouseEvent& event)
    {
        long flags=0;
        int New=HitTest(event.GetPosition(),&flags);

        if (m_OverPage!=New)
        {
            wxString Line;
            GetTip(New,Line);
            SetToolTip(Line);
        }

        m_OverPage=New;

        event.Skip();
    }
    virtual void GetTip(int Num,wxString& Line)=0;

    DECLARE_EVENT_TABLE()
};

//--------------------------------------------------------------------------------
class CIwStyleList : public wxListCtrl
{
    int m_OverPage;
public:
    CIwStyleList(wxWindow *parent,wxWindowID id,const wxSize& size=wxSize(-1,-1),long style=0) :
        wxListCtrl(parent,id,wxPoint(1000,1000),size,style),m_OverPage(-1) {}

    void OnMouse(wxMouseEvent& event)
    {
        int flags=0;
        int New=HitTest(event.GetPosition(),flags);

        if (m_OverPage!=New)
        {
            wxString Line;
            GetTip(New,Line);
            SetToolTip(Line);
        }

        m_OverPage=New;

        event.Skip();
    }
    virtual void GetTip(int Num,wxString& Line)=0;

    DECLARE_EVENT_TABLE()
};


//--------------------------------------------------------------------------------
class CIwStyleCtrlGroup : public wxBoxSizer
{
public:
    wxSizer* m_Sizer;
    int m_Columns;
    int m_CurrRow;
    bool m_SidePanels;
    bool m_AtLeft;
    bool m_Button;
    static bool s_UseODCtrls;
protected:
    void StartAdd()
    {
        int i;

        if (m_Button && !m_AtLeft)
            m_Sizer->AddSpacer(8);

        if (m_CurrRow>0)
            for (i=0; i<m_Columns; i++)
            {
                m_Sizer->AddSpacer(8);
            }

        m_CurrRow++;
        m_AtLeft=false;
    }
    wxStaticText* AddLabel(bool HasTop,wxWindow* Parent,const wxChar* Label)
    {
        wxStaticText* text=NULL;
        if (m_SidePanels)
        {
            if (s_UseODCtrls)
                text=new CIwStyleLabelOD(Parent,wxID_ANY,Label);
            else
                text=new wxStaticText(Parent,wxID_ANY,Label);

            m_Sizer->Add(text,0,wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT|wxRIGHT,4);
        }
        else if (HasTop && wxStrlen(Label)>0)
        {
            if (s_UseODCtrls)
                text=new CIwStyleLabelOD(Parent,wxID_ANY,Label,wxSize(-1,14));
            else
                text=new wxStaticText(Parent,wxID_ANY,Label,wxPoint(-1,-1),wxSize(-1,14));

            m_Sizer->Add(text,0,wxBOTTOM,2);

            for (int i=1; i<m_Columns; i++)
            {
                m_Sizer->AddSpacer(0);
            }
        }

        return text;
    }
public:
    CIwStyleCtrlGroup(bool SideLabels,bool Button,bool FromTop=false) : wxBoxSizer(wxVERTICAL),
        m_Columns(1),m_CurrRow(0),m_SidePanels(SideLabels),m_AtLeft(true),m_Button(Button)
    {
        if (m_SidePanels)
            m_Columns++;

        if (Button)
            m_Columns++;

        if (!FromTop)
            wxSizer::AddStretchSpacer();

        m_Sizer=new wxFlexGridSizer(m_Columns);
        wxSizer::Add(m_Sizer,0,wxALL|wxEXPAND,8);
        if (!FromTop)
            wxSizer::AddStretchSpacer();
    }

    void Clear(bool delete_windows=false)
    {
        m_AtLeft=true;
        m_CurrRow=0;
        m_Sizer->Clear(delete_windows);
    }
    void Add(wxWindow* Ctrl,const wxChar* Label,int prop=0,int flags=wxEXPAND)
    {
        StartAdd();
        wxStaticText* text=AddLabel(true,Ctrl->GetParent(),Label);
        m_Sizer->Add(Ctrl,prop,flags);

        if (text!=NULL && Ctrl->GetToolTip())
            text->SetToolTip(Ctrl->GetToolTip()->GetTip());
    }
    void Add(const wxChar* Label,wxWindow* Parent)
    {
        StartAdd();
        AddLabel(true,Parent,Label);
        m_Sizer->AddStretchSpacer();
    }
    void Add(wxSizer* Sizer,const wxChar* Label,wxWindow* Parent,int prop=0)
    {
        StartAdd();
        AddLabel(true,Parent,Label);
        m_Sizer->Add(Sizer,prop,wxEXPAND);
    }
    void Add(wxWindow* Ctrl,wxCheckBox* Check,int prop=0)
    {
        int i;
        StartAdd();

        if (m_SidePanels)
            m_Sizer->Add(Check,0,wxALIGN_CENTRE_VERTICAL|wxALIGN_RIGHT|wxRIGHT,4);
        else
        {
            m_Sizer->Add(Check,0,wxBOTTOM,2);

            for (i=1; i<m_Columns; i++)
            {
                m_Sizer->AddSpacer(0);
            }
        }

        m_Sizer->Add(Ctrl,prop,wxEXPAND);
    }
    void Add(wxCheckBox* Ctrl)
    {
        StartAdd();
        wxStaticText* text=AddLabel(false,Ctrl->GetParent(),Ctrl->GetLabel().c_str());

        m_Sizer->Add(Ctrl,0,wxALIGN_CENTRE_VERTICAL);

        if (text!=NULL && Ctrl->GetToolTip())
            text->SetToolTip(Ctrl->GetToolTip()->GetTip());
    }
    void Add(CIwStyleButton* Button)
    {
        m_AtLeft=true;
        m_Sizer->Add(Button,0,wxEXPAND|wxLEFT,4);
    }
    void AddAtLeft(wxWindow* ctrl)
    {
        m_AtLeft=true;
        m_Sizer->Add(ctrl,0,wxEXPAND|wxLEFT,4);
    }
    void AddAtLeft(wxSizer* ctrl)
    {
        m_AtLeft=true;
        m_Sizer->Add(ctrl,0,wxEXPAND|wxLEFT,4);
    }
};

//--------------------------------------------------------------------------------
class CIwStyleButtonBar : public wxBoxSizer
{
public:
    enum EIwStyleStretchSpace
    {
        SPACE_SMALL,
        SPACE_LARGE,
        SPACE_PROP,
    };
    wxSizer* m_Sizer;
    int InsertPoint;
public:
    CIwStyleButtonBar(wxWindow* parent,bool HasBar=true) : wxBoxSizer(wxVERTICAL)
    {
        if (HasBar)
            wxSizer::Add(new wxStaticLine(parent),0,wxEXPAND|wxALL,8);  //else

        //	AddSpacer(8);
        m_Sizer=new wxBoxSizer(wxHORIZONTAL);
        wxSizer::Add(m_Sizer,0,wxEXPAND);
        if (HasBar)
            AddSpacer(8);

        InsertPoint=0;
        m_Sizer->AddSpacer(8);
    }

    void Clear(bool delete_windows=false)
    {
        InsertPoint=0;
        m_Sizer->Clear(delete_windows);
    }
    void AddSpace(EIwStyleStretchSpace space=SPACE_LARGE)
    {
        switch (space)
        {
        case SPACE_SMALL :
            if (InsertPoint==0)
                m_Sizer->Insert(InsertPoint++,8,0);
            else
                m_Sizer->Insert(InsertPoint++,4,0);

            break;
        case SPACE_LARGE :
            if (InsertPoint==0)
                m_Sizer->Insert(InsertPoint++,8,0);
            else
                m_Sizer->Insert(InsertPoint++,12,0);

            break;
        case SPACE_PROP:
            if (InsertPoint==0)
                m_Sizer->Insert(InsertPoint++,8,0,1);
            else
                m_Sizer->Insert(InsertPoint++,12,0,1);

            break;
        }
    }
    void Add(wxWindow* Button=NULL,EIwStyleStretchSpace space=SPACE_LARGE)
    {
        AddSpace(space);

        if (Button==NULL)
            m_Sizer->Insert(InsertPoint++,72,24,0);
        else
        {
            m_Sizer->Insert(InsertPoint++,Button); //,0,wxALIGN_CENTRE_HORIZONTAL|wxALIGN_RIGHT);
        }
    }
    void AddCentred(wxWindow* Button=NULL,EIwStyleStretchSpace space=SPACE_LARGE)
    {
        AddSpace(space);

        if (Button==NULL)
            m_Sizer->Insert(InsertPoint++,72,24,0);
        else
            m_Sizer->Insert(InsertPoint++,Button,0,wxEXPAND|wxALIGN_CENTRE_HORIZONTAL|wxALIGN_RIGHT);
    }
};

#endif

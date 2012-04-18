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
#include "IwASDBase.h"

//--------------------------------------------------------------------------------
#ifdef ASD_AS_DLL
BEGIN_EVENT_TABLE(CIwLayoutFrame, wxDialog)
#else
BEGIN_EVENT_TABLE(CIwLayoutFrame, wxFrame)
#endif
    EVT_MENU_RANGE(3000,3999,CIwLayoutFrame::OnMainMenu)
    EVT_MENU_RANGE(2000,2999,CIwLayoutFrame::OnMainMenuTool)
    EVT_MENU_OPEN(CIwLayoutFrame::OnMainMenuOpen)
    EVT_SIZE(CIwLayoutFrame::OnSize)
    EVT_CLOSE(CIwLayoutFrame::OnFrameClose)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutSubFrame, wxFrame)
    EVT_CLOSE(CIwLayoutSubFrame::OnFrameClose)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
void CIwLayoutMenuDef::Setup(std::vector<wxString>& argv)
{
    int i;
    m_String=argv[1];

    for (i=3; i<(int)argv.size(); i++)
    {
        m_Conditions.push_back(argv[i]);
    }

    if (argv.size()>2)
    {
        m_Action=CIwTheApp->FindAction(argv[2]);

        if (m_Action!=NULL)
        {
            if (!m_Action->CheckShouldAddSubAction(argv[1]))
            {
                m_Type=FMENUITEM_NULL;
                return;
            }

            for (i=3; i<(int)argv.size(); i++)
            {
                CIwAction* act=m_Action->MakeSubAction(argv[i]);
                if (act!=NULL)
                    m_Action=act;
                else
                    break;
            }
            m_Action->AddControl(this);
        }
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutMenuDef::Update()
{
    if (m_Type==FMENUITEM_NULL)
        return;

    int i;
    wxMenu* menu=NULL;
    if (m_Parent!=NULL)
        menu=m_Parent->m_Menu;

    if (m_Action!=NULL)
        m_Action->m_Context=m_Context;

    if (menu!=NULL && m_Action!=NULL)
    {
        if (m_ID==-1) return;

        if (m_Type!=FMENUITEM_INLIST)
        {
            wxString ptr=m_String;
            m_Action->GetLabel(ptr);
            menu->SetLabel(m_ID,ptr);

            menu->Enable(m_ID,m_Action->GetEnable());

            if (m_Type==FMENUITEM_CHECK)
                menu->Check(m_ID,m_Action->GetValue()!=0);
        }
    }

    if (m_Menu!=NULL && m_Action!=NULL)
    {
        for (i=0; i<m_NumSubIDs; i++)
        {
            wxString ptr=m_String;
            if (i<(int)m_Conditions.size())
                ptr=m_Conditions[i];

            m_Action->GetLabel(ptr,i);
            menu->SetLabel(i+m_ID+1,ptr);

            m_Menu->Enable(i+m_ID+1,true);
            m_Menu->Check(i+m_ID+1,m_Action->GetValue()==i);
            m_Menu->Enable(i+m_ID+1,m_Action->GetEnable(i));
        }
    }

    if (m_Ctrl!=NULL && m_Action!=NULL)
    {
        wxString ptr=m_String;
        m_Action->GetLabel(ptr);
        m_Ctrl->SetLabel(ptr);

        m_Ctrl->Enable(m_Action->GetEnable());

        if (m_Type==FMENUITEM_CHECK)
            ((wxCheckBox*)m_Ctrl)->SetValue(m_Action->GetValue()!=0);

        if (m_Type==FTOOLITEM_TOGGLE)
            ((wxCustomButton*)m_Ctrl)->SetValue(m_Action->GetValue()!=0);

        if (m_Type==FMENUITEM_LIST || m_Type==FMENUITEM_INLIST)
        {
            wxComboBox* combo=(wxComboBox*)m_Ctrl;

            combo->Clear();
            for (int i=0;; i++)
            {
                wxString ptr;
                if (i<(int)m_Conditions.size())
                {
                    ptr=m_Conditions[i];
                    m_Action->GetLabel(ptr,i);
                }
                else if (!m_Action->GetLabel(ptr,i))
                    break;

                combo->Append(ptr);
            }

            combo->Select(m_Action->GetValue());
        }
    }

    for (i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->Update();
    }
}

//--------------------------------------------------------------------------------
#ifdef ASD_AS_DLL
CIwLayoutFrame::CIwLayoutFrame() : wxDialog(NULL,wxID_ANY,IwGetSDKIdent()+L" Studio",wxPoint(-1,-1),wxSize(-1,-1)),
#else
CIwLayoutFrame::CIwLayoutFrame() : wxFrame(NULL,wxID_ANY,IwGetSDKIdent()+L" Studio"),
#endif
    m_Elements(NULL),m_PopupMenu(NULL),m_CurrDnDElem(NULL),m_CurrDnDData(NULL)
{
    SetFont(CIwStyleHeader::GetFont());
    SetIcon(wxIcon(L"WXDEFAULT_FRAME"));
    //Maximize();

#ifdef ASD_AS_DLL
    m_StatusWindow=NULL;

    wxSizer* mainSizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);

    m_MenuToolBar=new wxToolBar(this,wxID_ANY,wxPoint(-1,-1),wxSize(-1,-1),wxNO_BORDER|wxTB_HORIZONTAL|wxTB_TEXT|wxTB_NOICONS|wxTB_FLAT);
    mainSizer->Add(m_MenuToolBar,0,wxEXPAND);

    m_Sizer=new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(m_Sizer,1,wxEXPAND);
#else
    m_StatusWindow=this;

    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);
#endif


    CIwTheApp->AddModule(FRAME_MODULE_ID,new CIwFrameModule(this));

    SetMainTitle();
}

//--------------------------------------------------------------------------------
CIwLayoutFrame::~CIwLayoutFrame()
{
    if (m_Elements!=NULL) delete m_Elements;

    if (m_PopupMenu!=NULL) delete m_PopupMenu;

    for (int i=0; i<(int)m_SubWindows.size(); i++)
    {
        delete m_SubWindows[i];
    }

    CIwTheApp->m_Frame=NULL;
}


//--------------------------------------------------------------------------------
CIwLayoutSubFrame::CIwLayoutSubFrame(CIwLayoutFrame* parent,wxPoint pt,wxSize sz,const wxString& ident) :
    wxFrame(parent,wxID_ANY,IwGetSDKIdent()+L" Studio",pt,sz),m_Elements(NULL),m_PopupMenu(NULL),m_Parent(parent),m_Ident(ident)
{
    SetFont(CIwStyleHeader::GetFont());
    SetIcon(wxIcon(L"WXDEFAULT_FRAME"));

    m_Sizer=new wxBoxSizer(wxVERTICAL);
    SetSizer(m_Sizer);
}

//--------------------------------------------------------------------------------
CIwLayoutSubFrame::~CIwLayoutSubFrame()
{
    if (m_Elements!=NULL) delete m_Elements;

    if (m_PopupMenu!=NULL) delete m_PopupMenu;
}

//--------------------------------------------------------------------------------
class CIwAbout : public wxHtmlWindow
{
public:
    CIwAbout(wxWindow* parent) : wxHtmlWindow(parent) {}
    virtual wxHtmlOpeningStatus OnOpeningURL(wxHtmlURLType type,const wxString& url, wxString *redirect) const
    {
        if (type==wxHTML_URL_PAGE && !url.StartsWith(L"file:"))
        {
            if (url.StartsWith(L"http:"))
                wxLaunchDefaultBrowser(url);

            return wxHTML_BLOCK;
        }
        else
            return wxHTML_OPEN;
    }
};

//--------------------------------------------------------------------------------
void CIwLayoutFrame::AboutBox(const wxString& FileName)
{
    int x,y;
    wxDisplaySize(&x,&y);
    if (x>440) x=440;

    if (y>370) y=370;

    wxDialog Dlg(this,wxID_ANY,L"About "+IwGetSDKIdent()+L" Studio...",wxPoint(-1,-1),wxSize(x,y));

    wxSizer* sizer=new wxBoxSizer(wxHORIZONTAL);
    Dlg.SetSizer(sizer);
    wxHtmlWindow* win=new CIwAbout(&Dlg);
    sizer->Add(win,1,wxEXPAND);

    win->LoadFile(CIwTheApp->MakeAbsoluteFilename(L"{viewer}"+FileName));

    wxSize Size=Dlg.GetMinSize();
    win->SetScrollRate(20,20);
    win->SetVirtualSize(-1,Size.y);
    win->Layout();
    sizer->Layout();

    Dlg.ShowModal();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::LoadHelp(const wxString& Args)
{
#ifdef I3D_OS_WINDOWS
    wxString env;
    wxGetEnv(L"S3E_DIR",&env);
    wxString File;
    wxString Action=L"0";

    if (Args.Find(L":")==-1)
        File=wxString::Format(L"%s\\..\\docs\\SDKDocumentation.chm",env.c_str());
    else
    {
        File=wxString::Format(L"%s\\%s",env.c_str(),Args.BeforeFirst(':').c_str());
        Action=Args.AfterFirst(':');
    }

    if (Action.empty())
        ShellExecute(GetHwnd(),NULL,L"hh.exe",File.c_str(),NULL,SW_MAXIMIZE);
    else
        ShellExecute(GetHwnd(),NULL,L"hh.exe",wxString(L"-mapid "+Action+L" "+File).c_str(),NULL,SW_MAXIMIZE);

#endif
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::LoadMenuToolbar(const wxString& FileName)
{
    wxTextFile fp(FileName);
    if (!fp.Exists() || !fp.Open())
        return;

    CIwLayoutMenuDef* parent=NULL;
    std::vector<int> indentList;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        CIwLayoutMenuDef* def=NULL;
        std::vector<wxString> argv;

        int argc=SuperSplit(fp[i],argv,L" \t\n");
        if (argc<1) continue;

        if (argv[0][0]=='/' || argv[0][0]=='#') continue;

        int indent;
        for (indent=0; indent<(int)fp[i].size(); indent++)
        {
            if (fp[i][indent]!=' ' && fp[i][indent]!='\t') break;
        }

        while (!indentList.empty() && indent<=indentList.back()) {
            if (parent!=NULL)
                parent=parent->m_Parent;

            indentList.pop_back();
        }

        if (argv[0].IsSameAs(L"section",false))
        {
            if (argc<2) continue;

            if (argv[1].IsSameAs(L"main",false))
                parent=&m_MainMenu;

            if (argv[1].IsSameAs(L"tab",false))
                parent=&m_TabMenu;

            if (argv[1].IsSameAs(L"right",false))
                parent=&m_RightMenu;
        }
        else if (argv[0].IsSameAs(L"menu",false))
        {
            if (argc<2) continue;

            def=new CIwLayoutMenuDef(parent,FMENUITEM_MENU);
            def->Setup(argv);

            parent=def;
            indentList.push_back(indent);
        }
        else if (argv[0].IsSameAs(L"item",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(parent,FMENUITEM_ITEM);
            def->Setup(argv);
        }
        else if (argv[0].IsSameAs(L"check",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(parent,FMENUITEM_CHECK);
            def->Setup(argv);
        }
        else if (argv[0].IsSameAs(L"list",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(parent,FMENUITEM_LIST);
            def->Setup(argv);
        }
        else if (argv[0].IsSameAs(L"inlist",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(parent,FMENUITEM_INLIST);
            def->Setup(argv);
        }
        else if (argv[0].IsSameAs(L"separator",false))
            def=new CIwLayoutMenuDef(parent,FMENUITEM_SEPARATOR);
        else if (argv[0].IsSameAs(L"toolbar",false))
        {
            if (argc<2) continue;

            def=new CIwLayoutMenuDef(parent,FTOOLITEM_BAR);
            def->Setup(argv);

            parent=def;
            indentList.push_back(indent);
        }
        else if (argv[0].IsSameAs(L"tool",false))
        {
            if (argc<3) continue;

            def=new CIwLayoutMenuDef(parent,FTOOLITEM_TOOL);
            def->Setup(argv);
        }
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::LoadLayoutDlg()
{
    wxFileDialog Dialog(this,L"Load New Layout...",CIwTheApp->MakeAbsoluteFilename(L"{user}"),LAYOUTSHARED,
                        L"Layout files (*.svl)|*.svl|All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);

    if (Dialog.ShowModal()!=wxID_OK)
        return;

    LoadLayout(Dialog.GetPath());
    Reset();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::SaveLayoutDlg()
{
    wxFileDialog Dialog(this,L"Save Current Layout...",CIwTheApp->MakeAbsoluteFilename(L"{user}"),
                        CIwTheApp->MakeAbsoluteFilename(L"VL{currproject}.svl"),L"Layout files (*.svl)|*.svl|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);

    if (Dialog.ShowModal()!=wxID_OK)
        return;

    SaveLayout(Dialog.GetPath());
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::EditTool()
{
    /*	if(m_CurrElem==NULL) return;

        if(m_CurrElem->Query(ELEMENT_QUERY_ISTOOL))
        {
            //CIwLayoutToolbarEdit Dlg(m_App->m_Frame);

            //Dlg.ShowModal();//==wxID_OK)
        }*/
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::LoadLayout(const wxString& FileName)
{
    int i;
    wxTextFile fp(FileName);
    std::vector<CIwLayoutElement*> Items;
    CIwLayoutSubFrame* currWindow=NULL;

    if (!fp.Exists())
        fp.Open(CIwTheApp->MakeAbsoluteFilename(L"{viewer}" LAYOUTSHARED));

    if (!fp.Exists() || !fp.Open())
    {
        wxMessageBox(L"Cannot load layout file needed for correct opperation, please reinstall",FileName);
        exit(-1);
        return;
    }

    if (m_Elements!=NULL)
        delete m_Elements;

    for (i=0; i<(int)m_SubWindows.size(); i++)
    {
        delete m_SubWindows[i];
    }
    m_SubWindows.clear();

    for (i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> argv;
        std::vector<wxString> argv2;

        int argc=SuperSplit(fp[i],argv,L" \t\n");
        if (argc<2) continue;

        if (argv[0].IsSameAs(L"startsize",false))
        {
            if (argv[1].IsSameAs(L"maximize",false))
            {
                Maximize();
                continue;
            }
            else if (argc>=3)
            {
                if (argc>4)
                {
                    SetSize(atoi(argv[3].mb_str()),atoi(argv[4].mb_str()));
                    CentreOnScreen();
                    SetSize(atoi(argv[1].mb_str()),atoi(argv[2].mb_str()));
                }
                else
                {
                    SetSize(atoi(argv[1].mb_str()),atoi(argv[2].mb_str()));
                    CentreOnScreen();
                }

                continue;
            }
        }

        if (argv[0].IsSameAs(L"window",false) && argc>4)
        {
            wxPoint pt=GetPosition();
            pt.x+=atoi(argv[1].mb_str());
            pt.y+=atoi(argv[2].mb_str());
            wxSize sz(atoi(argv[3].mb_str()),atoi(argv[4].mb_str()));

            if (argc>5)
            {
                currWindow=new CIwLayoutSubFrame(this,pt,sz,argv[5]);
                if (argv[5]==L"status")
                    m_StatusWindow=currWindow;
            }
            else
                currWindow=new CIwLayoutSubFrame(this,pt,sz);

            m_SubWindows.push_back(currWindow);
            Items.clear();

            continue;
        }

        CIwLayoutElement* CurrItem=CIwTheApp->MakeElement(argv[0]);
        if (CurrItem==NULL) continue;

        int indent;
        for (indent=0; indent<(int)fp[i].size(); indent++)
        {
            if (fp[i][indent]!=' ' && fp[i][indent]!='\t') break;
        }
        CurrItem->m_Indent=indent;
        CurrItem->m_ParentData=argv[1];

        while (!Items.empty())
        {
            if (Items.back()->m_Indent<CurrItem->m_Indent)
                break;

            Items.pop_back();
        }

        if (Items.empty())
        {
            CurrItem->m_Parent=NULL;
            if (currWindow!=NULL)
                currWindow->m_Elements=CurrItem;
            else
                m_Elements=CurrItem;
        }
        else
        {
            CurrItem->m_Parent=Items.back();
            CurrItem->m_Parent->m_Children.push_back(CurrItem);
        }

        for (int j=2; j<(int)argv.size(); j++)
        {
            argv2.push_back(argv[j]);
        }

        CurrItem->Load(argv2);
        Items.push_back(CurrItem);
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::SaveLayout(const wxString& FileName)
{
    wxTextFile fp(FileName);

    int i;
    wxPoint pt=GetPosition();
    wxSize sz=GetSize();
    wxSize sz1=GetSize();
    sz1.x+=pt.x;
    sz1.y+=pt.y;

    for (i=0; i<(int)m_SubWindows.size(); i++)
    {
        wxPoint pt2=m_SubWindows[i]->GetPosition();
        wxSize sz2=m_SubWindows[i]->GetSize();

        if (sz1.x<pt2.x+sz2.x) sz1.x=pt2.x+sz2.x;

        if (sz1.y<pt2.y+sz2.y) sz1.y=pt2.y+sz2.y;
    }

    fp.AddLine(wxString::Format(L"startsize %d %d",sz1.x-pt.x,sz1.y-pt.y,sz.x,sz.y));

    m_Elements->Save(fp,0);
    for (i=0; i<(int)m_SubWindows.size(); i++)
    {
        wxPoint pt2=m_SubWindows[i]->GetPosition();
        wxSize sz2=m_SubWindows[i]->GetSize();

        fp.AddLine(wxString::Format(L"window %d %d %d %d %s",pt2.x-pt.x,pt2.y-pt.y,sz2.x,sz2.y,m_SubWindows[i]->m_Ident.c_str()));

        m_SubWindows[i]->m_Elements->Save(fp,0);
    }


    fp.Write();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::SetMainTitle()
{
    if (CIwTheApp->MakeAbsoluteFilename(L"{projectdetails}").empty())
        SetTitle(CIwTheApp->MakeAbsoluteFilename(IwGetSDKIdent()+L" Studio - {currproject}"));
    else
        SetTitle(CIwTheApp->MakeAbsoluteFilename(IwGetSDKIdent()+L" Studio - {currproject} - {projectdetails}"));
}

//--------------------------------------------------------------------------------
void CIwLayoutSubFrame::Reset()
{
    if (m_Elements!=NULL)
    {
        m_Sizer->Clear(true);
        wxSizerItem* Item=m_Parent->CreateLayout(m_Elements,this);
        Item->SetBorder(0);
        m_Sizer->Add(Item);
        m_Sizer->Layout();

        m_Elements->Layout();
    }
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::Reset()
{
    if (m_Elements!=NULL)
    {
        m_Sizer->Clear(true);
        wxSizerItem* Item=CreateLayout(m_Elements,this);
        Item->SetBorder(0);
        m_Sizer->Add(Item);
        m_Sizer->Layout();

        m_Elements->Layout();
    }

    for (int i=0; i<(int)m_SubWindows.size(); i++)
    {
        m_SubWindows[i]->Show();
        m_SubWindows[i]->Reset();
    }

    MakeMainMenu();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::Update()
{
    if (m_Elements!=NULL)
        m_Elements->Layout();

    for (int i=0; i<(int)m_SubWindows.size(); i++)
    {
        m_SubWindows[i]->m_Elements->Layout();
    }

    m_MainMenu.Update();
}

//--------------------------------------------------------------------------------
class CIwLayoutMenu : public wxMenu
{
    CIwLayoutMenuDef* m_Def;
public:
    CIwLayoutMenu(CIwLayoutMenuDef* def) : m_Def(def) { }
    void OnItem(wxCommandEvent& e)
    {
        m_Def->Action(e.GetId());
    }

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CIwLayoutMenu, wxMenu)
    EVT_MENU_RANGE(0,10000,CIwLayoutMenu::OnItem)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
bool CIwLayoutMenuDef::Action(int id)
{
    //wxLogMessage(L"->%d %d:%d %s",id,m_ID,m_NumSubIDs,m_String);

    if (m_ID==id && m_Type!=FMENUITEM_LIST)
    {
        if (m_Action==NULL)
            return true;

        int value=1;
        if (m_Type==FMENUITEM_CHECK || m_Type==FTOOLITEM_TOGGLE)
            value=m_Parent->m_Menu->IsChecked(id) ? 1 : 0;

        m_Action->m_Context=m_Context;
        m_Action->Action(value);
        return true;
    }

    if (id>m_ID && id<m_ID+m_NumSubIDs+1)
    {
        if (m_Action==NULL)
            return true;

        int value=id-m_ID-1;

        m_Action->m_Context=m_Context;
        m_Action->Action(value);
        return true;
    }

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (m_Children[i]->Action(id))
            return true;
    }

    return false;
}

//--------------------------------------------------------------------------------
wxMenu* CIwLayoutMenuDef::Create(int& id,void* context)
{
    int i;

    m_Context=context;

    if (m_Action!=NULL)
    {
        m_Action->m_Context=m_Context;
        if (!m_Action->DoCreate())
        {
            m_ID=-1;
            return NULL;
        }
    }

    m_ID=id++;

    wxMenu* menu=NULL;
    if (m_Parent!=NULL)
        menu=m_Parent->m_Menu;

    wxMenuItem* Item=NULL;
    switch (m_Type)
    {
    case FMENUITEM_MENU:
        m_Menu=new CIwLayoutMenu(this);
        for (i=0; i<(int)m_Children.size(); i++)
        {
            m_Children[i]->Create(id,m_Context);
        }

        if (m_Parent==NULL)
            break;

    case FMENUITEM_ITEM:
        if (menu!=NULL)
            Item=new wxMenuItem(menu,m_ID,m_String,L"",wxITEM_NORMAL,m_Menu);

        break;
    case FMENUITEM_CHECK:
        if (menu!=NULL)
            Item=new wxMenuItem(menu,m_ID,m_String,L"",wxITEM_CHECK);

        break;
    case FMENUITEM_SEPARATOR:
        if (menu!=NULL)
            Item=new wxMenuItem(menu,wxID_SEPARATOR);

        break;
    case FMENUITEM_LIST:
        if (menu==NULL) break;

        m_Menu=new CIwLayoutMenu(this);
        Item=new wxMenuItem(menu,m_ID,m_String,L"",wxITEM_NORMAL,m_Menu);
    case FMENUITEM_INLIST:
        if (menu==NULL) break;

        if (m_Menu==NULL)
            m_Menu=menu;

        if (m_Action==NULL) break;

        for (m_NumSubIDs=0;; m_NumSubIDs++)
        {
            wxString ptr;
            if (m_NumSubIDs<(int)m_Conditions.size())
            {
                ptr=m_Conditions[m_NumSubIDs];
                m_Action->GetLabel(ptr,m_NumSubIDs);
            }
            else if (!m_Action->GetLabel(ptr,m_NumSubIDs))
                break;

            m_Menu->Append(new wxMenuItem(m_Menu,id++,ptr,L"",wxITEM_CHECK));
        }
        break;
    default:
        break;
    }

    if (m_Action!=NULL && Item!=NULL)
    {
        m_Action->OverrideMenu(Item,id);
        if (m_ID+1!=id)
            m_NumSubIDs=id-(m_ID+1);
    }

    if (menu!=NULL && Item!=NULL)
        menu->Append(Item);

    Update();

    return m_Menu;
}

//--------------------------------------------------------------------------------
wxMenu* CIwLayoutFrame::MakeMenu(CIwLayoutMenuDef& MenuDef,void* context)
{
    int id=0;
    if (MenuDef.m_Children.size()<1)
        return NULL;

    return MenuDef.m_Children[0]->Create(id,context);
}

//--------------------------------------------------------------------------------
wxMenu* CIwLayoutFrame::MakeMenu(CIwLayoutMenuDef& MenuDef,const wxString& name,void* context)
{
    int id=0;
    for (int i=0; i<(int)MenuDef.m_Children.size(); i++)
    {
        if (MenuDef.m_Children[i]->m_String.IsSameAs(name,false))
            return MenuDef.m_Children[i]->Create(id,context);
    }

    return NULL;
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnMainMenuTool(wxCommandEvent& event)
{
    int id=event.GetId()-2000;
    int id2=3000;

    PopupMenu(m_MainMenu.m_Children[id]->Create(id2,NULL));
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::MakeMainMenu()
{
#ifdef ASD_AS_DLL
    int id=2000;
    for (int i=0; i<(int)m_MainMenu.m_Children.size(); i++)
    {
        m_MenuToolBar->AddTool(id++,m_MainMenu.m_Children[i]->m_String,wxNullBitmap,wxNullBitmap);
        m_MainMenu.m_Children[i]->Update();
    }
    m_MenuToolBar->Realize();
#else
    int id=3000;
    wxMenuBar* Bar=new wxMenuBar();

    for (int i=0; i<(int)m_MainMenu.m_Children.size(); i++)
    {
        Bar->Append(m_MainMenu.m_Children[i]->Create(id,NULL),m_MainMenu.m_Children[i]->m_String);
        m_MainMenu.m_Children[i]->Update();
    }

    SetMenuBar(Bar);
#endif
}

//------------------------------------------------------------------------------
void CIwLayoutFrame::ReParent(wxSizerItem* item,wxWindow* parent)
{
    if (item->IsWindow())
        item->GetWindow()->Reparent(parent);

    if (item->IsSizer())
    {
        wxSizerItemList& List=item->GetSizer()->GetChildren();

        for (int i=0; i<(int)List.size(); i++)
        {
            ReParent(List[i],parent);
        }
    }
}

//--------------------------------------------------------------------------------
wxSizerItem* CIwLayoutFrame::CreateLayout(CIwLayoutElement* Element,wxWindow* parent)
{
    wxSizerItem* Item=Element->Create(parent);

    if (Item!=NULL && Item->GetWindow()!=NULL)
    {
        Item->GetWindow()->SetAutoLayout(true);
        parent=Item->GetWindow();
    }

    for (int i=0; i<(int)Element->m_Children.size(); i++)
    {
        wxSizerItem* Item2=CreateLayout(Element->m_Children[i],parent);

        if (!Element->AddChild(Item2,Element->m_Children[i]))
        {
            Item2->DeleteWindows();
            delete Item2;
        }
    }
    return Item;
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnSashDrag(wxSashEvent& event)
{
    if (event.GetDragStatus()==wxSASH_STATUS_OUT_OF_RANGE)
        return;

    CIwLayoutElemCont *CurrItem=(CIwLayoutElemCont*)event.m_callbackUserData;
    if (CurrItem==NULL) return;

    ((CIwLayoutElementSash*)(CurrItem->m_Elem))->m_Sashes[CurrItem->m_Num]->SetDefaultSize(wxSize(event.GetDragRect().width,event.GetDragRect().height));

    m_Elements->Layout();
    Refresh();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnSize(wxSizeEvent& event)
{
#ifdef ASD_AS_DLL
    wxDialog::OnSize(event);
#else
    wxFrame::OnSize(event);
#endif
    if (m_Elements!=NULL)
        m_Elements->Layout();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnMainMenu(wxCommandEvent& event)
{
    m_MainMenu.Action(event.GetId());
    m_MainMenu.Update();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnMainMenuOpen(wxMenuEvent& e)
{
    m_MainMenu.Update();
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnFrameClose(wxCloseEvent& e)
{
    bool veto=CloseElem(m_Elements,e.CanVeto(),true);
    if (e.CanVeto() && veto)
        e.Veto(true);
    else
        Destroy();
}

//--------------------------------------------------------------------------------
void CIwLayoutSubFrame::OnFrameClose(wxCloseEvent& e)
{
    m_Parent->OnFrameClose(e);
}

//--------------------------------------------------------------------------------
void CIwLayoutFrame::OnRMouse(CIwLayoutElement* Elem)
{
    if (m_PopupMenu!=NULL)
        delete m_PopupMenu;

    m_PopupMenu=NULL;

    m_PopupMenu=MakeMenu(m_TabMenu,Elem);

    if (m_PopupMenu!=NULL)
        PopupMenu(m_PopupMenu);
}

//--------------------------------------------------------------------------------
CIwLayoutElement* CIwLayoutFrame::FindNext(EIwLayoutElementQuery Query,CIwLayoutElement* Curr)
{
    while (true)
    {
        //get next
        if (Curr==NULL)
            Curr=m_Elements;
        else if (Curr->m_Children.size()>0)
            Curr=Curr->m_Children[0];
        else
        {
            while (Curr->m_Parent!=NULL)
            {
                int i;
                for (i=0; i<(int)Curr->m_Parent->m_Children.size(); i++)
                {
                    if (Curr==Curr->m_Parent->m_Children[i])
                        break;
                }

                if (i>=(int)Curr->m_Parent->m_Children.size()-1)
                    Curr=Curr->m_Parent;
                else
                {
                    Curr=Curr->m_Parent->m_Children[i+1];
                    break;
                }
            }
            if (Curr->m_Parent==NULL)
                return NULL;
        }

        //check
        if (Curr==NULL)
            return NULL;

        if (Query==ELEMENT_QUERY_ALL)
            return Curr;

        if (Curr->Query(Query))
            return Curr;
    }
    return NULL;
}
//--------------------------------------------------------------------------------
bool CIwLayoutFrame::ShowControl(wxWindow* win,CIwLayoutElement* Curr,int num)
{
    if (Curr==NULL)
        Curr=m_Elements;

    if (Curr->GetControl()==win)
    {
        if (Curr->m_Parent!=NULL && Curr->m_Parent->Query(ELEMENT_QUERY_ISNOTEBOOK))
        {
            wxNotebook* book=(wxNotebook*)Curr->m_Parent->GetControl();
            book->SetSelection(num);
        }

        return true;
    }

    for (int i=0; i<(int)Curr->m_Children.size(); i++)
    {
        if (ShowControl(win,Curr->m_Children[i],i))
        {
            if (Curr->m_Parent!=NULL && Curr->m_Parent->Query(ELEMENT_QUERY_ISNOTEBOOK))
            {
                wxNotebook* book=(wxNotebook*)Curr->m_Parent->GetControl();
                book->SetSelection(num);
            }

            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------
CIwLayoutElement* CIwLayoutFrame::CreateNew(CIwLayoutElement* parent,const wxString& newContainerType,const wxString& parentData)
{
    CIwLayoutElement* CurrItem=CIwTheApp->MakeElement(newContainerType);
    if (CurrItem==NULL) return NULL;

    CurrItem->m_Indent=parent->m_Indent+1;
    CurrItem->m_ParentData=parentData;
    CurrItem->m_Parent=parent;
    CurrItem->m_Parent->m_Children.push_back(CurrItem);

    wxSizerItem* Item=CurrItem->Create(parent->GetControl());

    if (Item!=NULL && Item->GetWindow()!=NULL)
        Item->GetWindow()->SetAutoLayout(true);

    if (!parent->AddChild(Item,CurrItem))
    {
        Item->DeleteWindows();
        delete Item;
    }

    if (parent->Query(ELEMENT_QUERY_ISNOTEBOOK))
    {
        wxNotebook* book=(wxNotebook*)parent->GetControl();
        book->SetSelection(book->GetPageCount()-1);
    }

    return CurrItem;
}

//--------------------------------------------------------------------------------
CIwLayoutData* CIwLayoutFrame::FindDataContainer(unsigned int dataType,const wxString& newContainerType,const wxString& addName)
{
    CIwLayoutData* Elem;

    Elem=(CIwLayoutData*)FindNext(ELEMENT_QUERY_ISDATACONTAINER);
    while (Elem!=NULL)
    {
        if (Elem->CanHandleData(dataType) && Elem->m_Type==newContainerType)
            return Elem;

        Elem=(CIwLayoutData*)FindNext(ELEMENT_QUERY_ISDATACONTAINER,Elem);
    }

    if (addName.empty())
        return NULL;

    CIwLayoutElementNotebook* Book=(CIwLayoutElementNotebook*)FindNext(ELEMENT_QUERY_ISNOTEBOOK);
    while (Book!=NULL)
    {
        if (Book->CanHandleData(newContainerType))
            return (CIwLayoutData*)CreateNew(Book,newContainerType,addName);

        Book=(CIwLayoutElementNotebook*)FindNext(ELEMENT_QUERY_ISNOTEBOOK,Book);
    }

    Book=(CIwLayoutElementNotebook*)FindNext(ELEMENT_QUERY_ISNOTEBOOK);
    while (Book!=NULL)
    {
        if (Book->CanHandleData(L"active"))
            return (CIwLayoutData*)CreateNew(Book,newContainerType,addName);

        Book=(CIwLayoutElementNotebook*)FindNext(ELEMENT_QUERY_ISNOTEBOOK,Book);
    }
    return NULL;
}

//--------------------------------------------------------------------------------
class CIwLayoutSaveDlg : public CIwStyleDialog
{
public:
    CIwLayoutSaveDlg(wxWindow* parent,const wxString& title,wxArrayString& names,bool canVeto) : CIwStyleDialog(parent,title)
    {
        wxSizer* sizer=new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        CIwStyleCtrlGroup* group=new CIwStyleCtrlGroup(false,false,true);
        sizer->Add(group,0,wxEXPAND);

        wxListBox* list=new wxListBox(this,wxID_ANY,wxPoint(-1,-1),wxSize(400,-1),names);
        group->Add(list,L"Save changes to the following items?");

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        sizer->Add(bar,0,wxEXPAND);

        bar->Add(new CIwStyleButton(this,wxID_YES,L"Yes"));
        bar->Add(new CIwStyleButton(this,wxID_NO,L"No"));
        if (canVeto)
            bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"));

        sizer->Fit(this);
    }
    void OnButton(wxCommandEvent& e)
    {
        EndModal(e.GetId());
    }

    DECLARE_EVENT_TABLE()
};
//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwLayoutSaveDlg,CIwStyleDialog)
    EVT_BUTTON(wxID_YES,CIwLayoutSaveDlg::OnButton)
    EVT_BUTTON(wxID_NO,CIwLayoutSaveDlg::OnButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------------
bool CIwLayoutFrame::CloseElem(CIwLayoutElement* elem,bool canVeto,bool all)
{
    int i,j;
    std::vector<CIwASDData*> list;

    elem->CheckSave(list);
    if (all)
    {
        for (i=0; i<(int)m_SubWindows.size(); i++)
        {
            m_SubWindows[i]->m_Elements->CheckSave(list);
        }

        if (CIwTheApp->m_RootData!=NULL)
            if (CIwTheApp->m_RootData->GetState(CIwASDData::STATE_CHANGED))
                list.push_back(CIwTheApp->m_RootData);

        for (i=0; i<(int)CIwTheApp->m_OpenData.size(); i++)
        {
            if (CIwTheApp->m_OpenData[i]->GetState(CIwASDData::STATE_CHANGED))
                list.push_back(CIwTheApp->m_OpenData[i]);
        }
    }

    wxArrayString names;
    for (i=0; i<(int)list.size(); i++)
    {
        for (j=0; j<i; j++)
        {
            if (list[i]==list[j])
                break;
        }
        if (j!=i)
            list[i]=NULL;
        else if (list[i]->GetState(CIwASDData::STATE_NOTSAVEABLE)==0)
            names.Add(list[i]->GetName());
    }
    if (names.empty())
        return false;

    CIwLayoutSaveDlg dlg(this,L"Closing Window",names,canVeto);
    switch (dlg.ShowModal())
    {
    case wxID_YES:
        break;
    case wxID_NO:
        return false;
    default:
        return canVeto;
    }

    for (i=0; i<(int)list.size(); i++)
    {
        if (list[i]!=NULL)
            list[i]->CheckSave(false);
    }

    return false;
}

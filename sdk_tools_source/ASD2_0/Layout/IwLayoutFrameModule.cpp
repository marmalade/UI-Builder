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

//------------------------------------------------------------------------------
class CIwActionTab : public CIwAction
{
public:
    enum Type
    {
        TAB,
        ALLTAB,
        EMPTYTAB,
    };
    Type m_Type;
    CIwLayoutFrame* m_Frame;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionTab(CIwModule* module,CIwLayoutFrame* frame,Type type) : CIwAction(module),m_Type(type),m_Frame(frame) {}
    virtual void Action(int value)
    {
        CIwLayoutElement* elem=(CIwLayoutElement*)m_Context;
        if (elem==NULL)
            return;

        CIwLayoutElementNotebook* Book=(CIwLayoutElementNotebook*)elem;
        CIwLayoutElementNotebook* Book2=(CIwLayoutElementNotebook*)elem->m_Parent;
        if (m_Type==TAB)
        {
            if (elem->Query(ELEMENT_QUERY_CANNOTCLOSE) || elem->Query(ELEMENT_QUERY_NOICON))
                return;

            if (Book2!=NULL && Book2->Query(ELEMENT_QUERY_ISNOTEBOOK))
                Book2->RemoveChild(elem,true);

            return;
        }

        if (Book!=NULL && Book->Query(ELEMENT_QUERY_ISNOTEBOOK))
            Book->RemoveAll(m_Type==EMPTYTAB);
        else if (Book2!=NULL && Book2->Query(ELEMENT_QUERY_ISNOTEBOOK))
            Book2->RemoveAll(m_Type==EMPTYTAB);
    }
    virtual bool DoCreate()
    {
        CIwLayoutElement* elem=(CIwLayoutElement*)m_Context;
        if (elem==NULL)
            return false;

        if (m_Type==TAB)
        {
            if (elem->Query(ELEMENT_QUERY_CANNOTCLOSE) || elem->Query(ELEMENT_QUERY_NOICON))
                return false;
        }
        else if (elem->Query(ELEMENT_QUERY_ISNOTEBOOK))
            return true;

        if (elem->m_Parent!=NULL)
            if (elem->m_Parent->Query(ELEMENT_QUERY_ISNOTEBOOK))
                return true;

        return false;
    }
};

//--------------------------------------------------------------------------------
CIwFrameModule::CIwFrameModule(CIwLayoutFrame* Frame) : CIwModule(L"frame_"),m_Frame(Frame)
{
}

//--------------------------------------------------------------------------------
void CIwFrameModule::OnInit()
{
    CIwTheApp->AddAction(L"frame_exit",new CIwActionDelegate(this,MakeDelegate(m_Frame,&CIwLayoutFrame::Close)));
    CIwTheApp->AddAction(L"frame_loadlayout",new CIwActionDelegate(this,MakeDelegate(m_Frame,&CIwLayoutFrame::LoadLayoutDlg)));
    CIwTheApp->AddAction(L"frame_savelayout",new CIwActionDelegate(this,MakeDelegate(m_Frame,&CIwLayoutFrame::SaveLayoutDlg)));
    CIwTheApp->AddAction(L"frame_edittool",new CIwActionDelegate(this,MakeDelegate(m_Frame,&CIwLayoutFrame::EditTool)));
    CIwTheApp->AddAction(L"frame_help",new CIwActionDelegateParam(this,MakeDelegate(m_Frame,&CIwLayoutFrame::LoadHelp)));
    CIwTheApp->AddAction(L"frame_about",new CIwActionDelegateParam(this,MakeDelegate(m_Frame,&CIwLayoutFrame::AboutBox)));

    CIwTheApp->AddAction(L"frame_tab",new CIwActionTab(this,m_Frame,CIwActionTab::TAB));
    CIwTheApp->AddAction(L"frame_alltab",new CIwActionTab(this,m_Frame,CIwActionTab::ALLTAB));
    CIwTheApp->AddAction(L"frame_emptytab",new CIwActionTab(this,m_Frame,CIwActionTab::EMPTYTAB));

    m_Frame->LoadMenuToolbar(m_App->MakeAbsoluteFilename(L"{viewer}Menu.ast"));

    m_Frame->LoadLayout(m_App->MakeAbsoluteFilename(L"{layout}"));
}

//------------------------------------------------------------------------------
CIwLayoutElement* CIwFrameModule::MakeElement(const wxString& type)
{
    if (type.IsSameAs(L"vertical",false))
        return new CIwLayoutElementSizer(wxVERTICAL);

    if (type.IsSameAs(L"horizontal",false))
        return new CIwLayoutElementSizer(wxHORIZONTAL);

    if (type.IsSameAs(L"sash",false))
        return new CIwLayoutElementSash;

    if (type.IsSameAs(L"notebook",false))
        return new CIwLayoutElementNotebook;

    if (type.IsSameAs(L"toolbar",false))
        return new CIwLayoutElementToolbar;

    if (type.IsSameAs(L"log",false))
        return new CIwLayoutElementLog;

    return NULL;
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementSizer::Create(wxWindow* parent)
{
    m_Ctrl=parent;
    m_Sizer=new wxBoxSizer(m_Orient);
    return new wxSizerItem(m_Sizer,1,wxEXPAND|wxALL,8,NULL);
}

//------------------------------------------------------------------------------
bool CIwLayoutElementSizer::AddChild(wxSizerItem* child,CIwLayoutElement* elem)
{
    child->SetProportion(atoi(elem->m_ParentData.mb_str()));
    //child->SetFlag(wxALL);
    m_Sizer->Add(child);
    return true;
}

//------------------------------------------------------------------------------
wxString CIwLayoutElementSizer::GetParentData(CIwLayoutElement* Child)
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (Child==m_Children[i])
            return wxString::Format(L"%d",m_Sizer->GetItem(i)->GetProportion());
    }

    return L"0";
}

//------------------------------------------------------------------------------
CIwLayoutElementSash::CIwLayoutElementSash() : m_MainPanel(NULL)
{
    static int ID=0;
    m_ID=ID;
    ID+=30;
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementSash::Create(wxWindow* parent)
{
    m_Panel=new CIwLayoutPanel(parent,this);

    return new wxSizerItem(m_Panel,1,wxEXPAND|wxALL,8,NULL);
}

//------------------------------------------------------------------------------
wxSashLayoutWindow* CIwLayoutElementSash::GetSash(CIwLayoutElement* child)
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (m_Children[i]==child)
            return m_Sashes[i];
    }

    return NULL;
}

//------------------------------------------------------------------------------
bool CIwLayoutElementSash::AddChild(wxSizerItem* child,CIwLayoutElement* elem)
{
    wxSashLayoutWindow* Layout=NULL;
    wxSizer* Sizer=NULL;

    switch (elem->m_ParentData[0])
    {
    case 'L':
    case 'l':
        Layout=new CIwLayoutSash(m_Panel,elem,m_ID+m_Sashes.size());
        Layout->SetAlignment(wxLAYOUT_LEFT);
        Layout->SetOrientation(wxLAYOUT_VERTICAL);
        Layout->SetSashVisible(wxSASH_RIGHT,true);
        Layout->SetSashBorder(wxSASH_RIGHT,true);
        break;
    case 'R':
    case 'r':
        Layout=new CIwLayoutSash(m_Panel,elem,m_ID+m_Sashes.size());
        Layout->SetAlignment(wxLAYOUT_RIGHT);
        Layout->SetOrientation(wxLAYOUT_VERTICAL);
        Layout->SetSashVisible(wxSASH_LEFT,true);
        Layout->SetSashBorder(wxSASH_LEFT,true);
        break;
    case 'T':
    case 't':
        Layout=new CIwLayoutSash(m_Panel,elem,m_ID+m_Sashes.size());
        Layout->SetAlignment(wxLAYOUT_TOP);
        Layout->SetOrientation(wxLAYOUT_HORIZONTAL);
        Layout->SetSashVisible(wxSASH_BOTTOM,true);
        Layout->SetSashBorder(wxSASH_BOTTOM,true);
        break;
    case 'B':
    case 'b':
        Layout=new CIwLayoutSash(m_Panel,elem,m_ID+m_Sashes.size());
        Layout->SetAlignment(wxLAYOUT_BOTTOM);
        Layout->SetOrientation(wxLAYOUT_HORIZONTAL);
        Layout->SetSashVisible(wxSASH_TOP,true);
        Layout->SetSashBorder(wxSASH_TOP,true);
        break;
    default:
        m_MainPanel=new CIwLayoutPanel(m_Panel,elem);
        Sizer=new wxBoxSizer(wxHORIZONTAL);
        m_MainPanel->SetSizer(Sizer);
        CIwTheFrame->ReParent(child,m_MainPanel);
        break;
    }
    if (Layout!=NULL)
    {
        Sizer=new wxBoxSizer(wxHORIZONTAL);
        Layout->SetSizer(Sizer);

        Layout->SetDefaultSize(wxSize(atoi(elem->m_ParentData.mb_str()+1),atoi(elem->m_ParentData.mb_str()+1)));

        CIwTheFrame->ReParent(child,Layout);

        CIwTheFrame->Connect(m_ID+m_Sashes.size(),wxEVT_SASH_DRAGGED,wxSashEventHandler(CIwLayoutFrame::OnSashDrag),
                             new CIwLayoutElemCont(this,m_Sashes.size()),CIwTheFrame);
    }

    child->SetBorder(2);
    Sizer->Add(child);
    m_Sashes.push_back(Layout);

    return true;
}

//------------------------------------------------------------------------------
wxString CIwLayoutElementSash::GetParentData(CIwLayoutElement* Child)
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (Child==m_Children[i])
        {
            if (m_Sashes[i]==NULL) return L"0";

            switch (m_Sashes[i]->GetAlignment())
            {
            case wxLAYOUT_LEFT:
                return wxString::Format(L"L%d",m_Sashes[i]->GetSize().GetWidth());
            case wxLAYOUT_RIGHT:
                return wxString::Format(L"R%d",m_Sashes[i]->GetSize().GetWidth());
            case wxLAYOUT_TOP:
                return wxString::Format(L"T%d",m_Sashes[i]->GetSize().GetHeight());
            case wxLAYOUT_BOTTOM:
                return wxString::Format(L"B%d",m_Sashes[i]->GetSize().GetHeight());
            default:
                break;
            }
        }
    }
    return L"0";
}

//------------------------------------------------------------------------------
void CIwLayoutElementSash::DoLayout()
{
    wxLayoutAlgorithm layout;
    layout.LayoutWindow(m_Panel,m_MainPanel);

    for (int i=0; i<(int)m_Sashes.size(); i++)
    {
        if (m_Sashes[i]!=NULL)
            m_Sashes[i]->Layout();
    }
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementNotebook::Create(wxWindow* parent)
{
    m_Book=new CIwLayoutNotebook(parent,this);
    return new wxSizerItem(m_Book,1,wxEXPAND|wxALL,8,NULL);
}

//------------------------------------------------------------------------------
void CIwLayoutElementNotebook::UpdateTitles()
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        UpdateTitle(i);
    }
}

//------------------------------------------------------------------------------
void CIwLayoutElementNotebook::UpdateTitle(int i)
{
    wxString changed=L"";
    if (m_Children[i]->Query(ELEMENT_QUERY_ISDATACONTAINER))
    {
        CIwLayoutData* item=(CIwLayoutData*)m_Children[i];
        CIwASDData* data=item->GetData();
        if (data!=NULL && data->HasChanged())
            changed=L" *";
    }

    m_Book->SetPageText(i,wxString::Format(L"%s%s",m_Children[i]->GetTitle().c_str(),changed.c_str()));
}

//------------------------------------------------------------------------------
bool CIwLayoutElementNotebook::AddChild(wxSizerItem* child,CIwLayoutElement* elem,int Pos)
{
    wxPanel* Panel=new CIwLayoutPanel(m_Book,elem);
    wxSizer* Sizer=new wxBoxSizer(wxHORIZONTAL);
    Panel->SetSizer(Sizer);

    if (elem->Query(ELEMENT_QUERY_NODRAGNDROP))
        m_NoDnD=true;

    CIwTheFrame->ReParent(child,Panel);
    Sizer->Add(child);

    m_Book->InsertPage(Pos,Panel,elem->GetTitle());
    return true;
}

//------------------------------------------------------------------------------
wxString CIwLayoutElementNotebook::GetParentData(CIwLayoutElement* Child)
{
    for (int i=0; i<(int)m_Children.size(); i++)
    {
        if (Child==m_Children[i])
            return wxString::Format(L"\"%s\"",m_Book->GetPageText(i).c_str());
    }

    return L"\"\"";
}

//------------------------------------------------------------------------------
bool CIwLayoutElementNotebook::Query(EIwLayoutElementQuery value)
{
    switch (value)
    {
    case ELEMENT_QUERY_ISNOTEBOOK:
        return true;
    case ELEMENT_QUERY_NODRAGNDROP:
        return m_NoDnD;
    default:
        break;
    }

    return false;
}

//------------------------------------------------------------------------------
void CIwLayoutElementNotebook::Load(std::vector<wxString>& argv)
{
    for (int i=0; i<(int)argv.size(); i++)
    {
        m_Targets.push_back(argv[i]);
    }
}

//------------------------------------------------------------------------------
wxString CIwLayoutElementNotebook::DoSave()
{
    wxString s;
    for (int i=0; i<(int)m_Targets.size(); i++)
    {
        s+=m_Targets[i]+L" ";
    }
    return s;
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementNotebook::RemoveChild(CIwLayoutElement* elem,bool Delete)
{
    int Page;
    for (Page=0; Page<(int)m_Children.size(); Page++)
    {
        if (m_Children[Page]==elem)
            break;
    }
    if (Page==(int)m_Children.size())
        return NULL;

    wxSizerItem* Item=NULL;
    if (!Delete)
    {
        wxPanel* Panel=(wxPanel*)m_Book->GetPage(Page);
        wxSizer* Sizer=Panel->GetSizer();
        wxSizerItem* Item1=Sizer->GetItem((size_t)0);

        CIwTheFrame->ReParent(Item1,NULL);
        if (Item1->IsWindow())
            Item=new wxSizerItem(Item1->GetWindow(),Item1->GetProportion(),Item1->GetFlag(),Item1->GetBorder(),Item1->GetUserData());
        else if (Item1->IsSizer())
            Item=new wxSizerItem(Item1->GetSizer(),Item1->GetProportion(),Item1->GetFlag(),Item1->GetBorder(),Item1->GetUserData());
        else return NULL;
    }
    else if (CIwTheFrame->CloseElem(elem,true,false))
        return NULL;

    m_Children.erase(m_Children.begin()+Page);
    m_Book->DeletePage(Page);

    if (Delete)
        delete elem;

    Layout();
    m_Book->Refresh();

    return Item;
}

//------------------------------------------------------------------------------
void CIwLayoutElementNotebook::RemoveAll(bool EmptyOnly)
{
    int Page=0;
    while (Page<(int)m_Children.size())
    {
        if (m_Children[Page]->Query(ELEMENT_QUERY_CANNOTCLOSE) || m_Children[Page]->Query(ELEMENT_QUERY_NOICON))
            Page++;
        else if (EmptyOnly && !m_Children[Page]->Query(ELEMENT_QUERY_ISEMPTY))
            Page++;
        else
            RemoveChild(m_Children[Page],true);
    }

    Layout();
    m_Book->Refresh();
}

//------------------------------------------------------------------------------
void CIwLayoutElementNotebook::Move(CIwLayoutElement* Item,int To,bool Replace)
{
    if (Item->m_Parent==NULL)
        return;

    wxSizerItem* child=Item->m_Parent->RemoveChild(Item,false);
    if (child==NULL)
        return;

    if (To>(int)m_Children.size())
        To=m_Children.size();

    m_Children.insert(m_Children.begin()+To,Item);
    AddChild(child,Item,To);
    m_Book->SetSelection(To);

    Item->m_Parent=this;

    Layout();
    m_Book->Refresh();
}

//------------------------------------------------------------------------------
wxDragResult CIwLayoutElementNotebook::DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data)
{
    if (CIwTheFrame->m_CurrDnDElem==NULL)
        return wxDragNone;

    if (CIwTheFrame->m_CurrDnDElem==this)
        return wxDragNone;

    if (m_NoDnD)
        return wxDragNone;

    wxPoint Pt2=m_Book->ScreenToClient(Pt);
    int Tab=m_Book->HitTest(Pt2);

    wxDragResult Result=wxDragCopy;
    if (Tab>=0 && Tab<(int)m_Children.size())
    {
        Result=m_Children[Tab]->DoDrop(Pt,def,Data);
        if (Result==wxDragNone)
            Result=DoDrop(Pt,def,m_Children[Tab],Data);
    }
    else
    {
        if (m_Children.size()>0 && m_Children[m_Children.size()-1]==CIwTheFrame->m_CurrDnDElem)
            return wxDragNone;

        if (Data!=NULL)
            Move(CIwTheFrame->m_CurrDnDElem,m_Children.size(),false);
    }

    return Result;
}

//------------------------------------------------------------------------------
wxDragResult CIwLayoutElementNotebook::DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutElement* Child,CIwLayoutDnDObject* Data)
{
    if (CIwTheFrame->m_CurrDnDElem==NULL)
        return wxDragNone;

    if (CIwTheFrame->m_CurrDnDElem==Child)
        return wxDragNone;

    if (m_NoDnD)
        return wxDragNone;

    int Page;
    for (Page=0; Page<(int)m_Children.size(); Page++)
    {
        if (m_Children[Page]==Child)
            break;
    }
    if (Page==(int)m_Children.size())
        return wxDragNone;

    if (Data!=NULL)
        Move(CIwTheFrame->m_CurrDnDElem,Page,def==wxDragMove);

    return wxDragCopy; //def;
}

//------------------------------------------------------------------------------
bool CIwLayoutElementNotebook::CanHandleData(const wxString& containerType)
{
    for (int i=0; i<(int)m_Targets.size(); i++)
    {
        if (m_Targets[i].IsSameAs(containerType,false))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void CIwLayoutElementLog::Load(std::vector<wxString>& argv)
{
    if (argv.size()>1)
        m_FileName=argv[1];
}

//------------------------------------------------------------------------------
CIwLayoutElementLog::~CIwLayoutElementLog()
{
    CIwTheApp->m_Log->Setup(NULL);
}

//------------------------------------------------------------------------------
wxSizerItem* CIwLayoutElementLog::Create(wxWindow* parent)
{
    m_TextCtrl=new wxTextCtrl(parent,wxID_ANY,wxEmptyString,wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxHSCROLL|wxTE_RICH|wxTE_READONLY);
    CIwTheApp->m_Log->Setup(m_TextCtrl,m_FileName);

    return new wxSizerItem(m_TextCtrl,1,wxEXPAND|wxALL,8,NULL);
}

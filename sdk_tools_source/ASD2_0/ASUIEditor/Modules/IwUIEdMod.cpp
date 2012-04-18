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
#include "IwUIEdHeader.h"


CIwProject Project;

//------------------------------------------------------------------------------
void CIwUIEdModule::SetupProjectDlg()
{
    CIwSetProjectDialog Setup(NULL,CIwTheFileSrcCtrl);
    if (Setup.ShowModal()!=wxID_OK)
        return;

    SetupProject2();
}
//------------------------------------------------------------------------------
void CIwUIEdModule::SetupProject2()
{
    CIwASDFile* root=(CIwASDFile*)CIwTheApp->GetRoot();
    if (root==NULL) return;

    root->Clear();

    wxString Data;
    Project.GetFile(L"data",Data);

    root->AddPath(Data);
    root->m_Paths[0]->m_Root=CIwASDFilename::ROOT_MAIN;

    CIwTheApp->SetRoot(root);
    root->Load(-1);

    CIwASDTreeLayoutProject* Elem=(CIwASDTreeLayoutProject*)m_App->m_Frame->FindDataContainer(FILETYPE_DIR,L"project",L"Data Directory");
    if (Elem!=NULL)
        Elem->SetData(root);
}

//------------------------------------------------------------------------------
class CIwWinCustomSize : public wxDialog
{
public:
    wxSpinCtrl* m_X;
    wxSpinCtrl* m_Y;
public:
    CIwWinCustomSize(wxWindow* parent,wxSize& Size) : wxDialog(parent,wxID_ANY,L"Choose custom viewer window size...",wxPoint(-1,-1))
    {
        wxSizer* vert=new wxBoxSizer(wxVERTICAL);
        wxSizer* hor =new wxBoxSizer(wxHORIZONTAL);

        SetSizer(vert);

        hor->Add(new wxStaticText(this,wxID_ANY,L"X Dimension:"),0,wxALL,4);

        m_X=new wxSpinCtrl(this,wxID_ANY,wxString::Format(L"%d",Size.x),wxPoint(-1,-1),wxSize(-1,-1),wxSP_ARROW_KEYS,128,9999);
        hor->Add(m_X,0,wxALL,4);

        hor->Add(new wxStaticText(this,wxID_ANY,L"Y Dimension:"),0,wxALL,4);

        m_Y=new wxSpinCtrl(this,wxID_ANY,wxString::Format(L"%d",Size.y),wxPoint(-1,-1),wxSize(-1,-1),wxSP_ARROW_KEYS,128,9999);
        hor->Add(m_Y,0,wxALL,4);

        vert->Add(hor);

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"OK"),CIwStyleButtonBar::SPACE_PROP);
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"),CIwStyleButtonBar::SPACE_SMALL);
        vert->Add(bar,0,wxEXPAND);

        vert->Layout();
        vert->Fit(this);
    }
};

//------------------------------------------------------------------------------
class CIwActionViewSize : public CIwAction
{
protected:
    std::vector<CIwActionViewSize*> m_Children;
    CIwActionViewSize* m_Parent;
public:
    CIwActionViewSize(CIwModule* module) : CIwAction(module),m_Parent(NULL) {}
    virtual ~CIwActionViewSize()
    {
        if (m_Parent!=NULL)
        {
            for (int i=0; i<(int)m_Parent->m_Children.size(); i++)
            {
                if (m_Parent->m_Children[i]==this)
                {
                    m_Parent->m_Children.erase(m_Parent->m_Children.begin()+i);
                    break;
                }
            }
        }
    }

    virtual void Action(int value=1)
    {
        if (!CIwTheHost.m_InViewer) return;

        if (m_Controls.size()==0 || CIwTheHost.m_Shared.m_ViewerFrame==NULL) return;

        wxSize size(CIwTheHost.m_Shared.m_Project.m_Width,CIwTheHost.m_Shared.m_Project.m_Height);

        if (m_Controls[0]->m_Conditions.size()<2)
        {
            CIwWinCustomSize dlg(CIwTheFrame,size);
            if (dlg.ShowModal()==wxID_OK)
            {
                size.x = dlg.m_X->GetValue();
                size.y = dlg.m_Y->GetValue();
            }
        }
        else
        {
            size=wxSize(atoi(m_Controls[0]->m_Conditions[0].mb_str()),atoi(m_Controls[0]->m_Conditions[1].mb_str()));
            if (CIwTheHost.m_Shared.m_Project.m_Width>CIwTheHost.m_Shared.m_Project.m_Height && size.x<size.y)
                size=wxSize(atoi(m_Controls[0]->m_Conditions[1].mb_str()),atoi(m_Controls[0]->m_Conditions[0].mb_str()));
            else if (CIwTheHost.m_Shared.m_Project.m_Width<CIwTheHost.m_Shared.m_Project.m_Height && size.x>size.y)
                size=wxSize(atoi(m_Controls[0]->m_Conditions[1].mb_str()),atoi(m_Controls[0]->m_Conditions[0].mb_str()));
        }

        CIwTheHost.m_Shared.m_ViewerFrame->SetViewerSize(size.x,size.y);

        CIwTheFrame->Update();
    }
    bool IsValue()
    {
        if (m_Controls.size()==0) return false;

        if (m_Controls[0]->m_Conditions.size()<2) return false;

        return (CIwTheHost.m_Shared.m_Project.m_Width==atoi(m_Controls[0]->m_Conditions[0].mb_str()) &&
                CIwTheHost.m_Shared.m_Project.m_Height==atoi(m_Controls[0]->m_Conditions[1].mb_str())) ||
               (CIwTheHost.m_Shared.m_Project.m_Width==atoi(m_Controls[0]->m_Conditions[1].mb_str()) &&
                CIwTheHost.m_Shared.m_Project.m_Height==atoi(m_Controls[0]->m_Conditions[0].mb_str()));
    }
    virtual int GetValue()
    {
        if (m_Controls.size()==0) return 0;

        if (m_Controls[0]->m_Conditions.size()<2)
        {
            if (m_Parent==NULL)
                return 0;

            for (int i=0; i<(int)m_Parent->m_Children.size(); i++)
            {
                if (m_Parent->m_Children[i]->IsValue())
                    return 0;
            }
            return 1;
        }
        else
            return IsValue() ? 1 : 0;
    }
    virtual CIwAction* MakeSubAction(const wxString& tag)
    {
        if (m_Parent!=NULL) return NULL;

        CIwActionViewSize* child=new CIwActionViewSize(m_Module);
        child->m_Parent=this;
        m_Children.push_back(child);
        return child;
    }
};

//------------------------------------------------------------------------------
class CIwActionInst : public CIwAction
{
protected:
    CUIEdAttrShared* m_Shared;
    int m_Mode;
    FastDelegate2<CIwAttrInstance*,int> m_Delegate;
public:
    CIwActionInst(CIwModule* module,CUIEdAttrShared* shared,int mode) : CIwAction(module),m_Shared(shared),m_Mode(mode)
    {
        m_Delegate=FastDelegate2<CIwAttrInstance*,int>(this,&CIwActionInst::DoAction);
    }
    CIwActionInst(CIwModule* module,CUIEdAttrShared* shared,int mode,FastDelegate2<CIwAttrInstance*,int> delegate) :
        CIwAction(module),m_Shared(shared),m_Mode(mode),m_Delegate(delegate) {}

    virtual bool GetEnable(int value=-1)
    {
        switch (m_Shared->m_OpenEditor)
        {
        case 0:
            if (m_Shared->m_UIEdAttrUIPanel==NULL)
                return false;

            if (m_Shared->m_UIEdAttrUIPanel->m_Sect==NULL)
                return false;

            return true;
        case 1:
            if (m_Shared->m_UIEdAttrStylePanel==NULL)
                return false;

            if (m_Shared->m_UIEdAttrStylePanel->m_Sect==NULL)
                return false;

            return true;
        case 2:
            if (m_Shared->m_UIEdAttrMaterialPanel==NULL)
                return false;

            if (m_Shared->m_UIEdAttrMaterialPanel->m_Sect==NULL)
                return false;

            return true;
        }
        return false;
    }
    virtual void Action(int value=1)
    {
        switch (m_Shared->m_OpenEditor)
        {
        case 0:
            if (m_Shared->m_UIEdAttrUIPanel==NULL)
                return;

            if (m_Shared->m_UIEdAttrUIPanel->m_Sect==NULL)
                return;

            m_Delegate(m_Shared->m_UIEdAttrUIPanel->m_Sect,m_Mode);
            return;
        case 1:
            if (m_Shared->m_UIEdAttrStylePanel==NULL)
                return;

            if (m_Shared->m_UIEdAttrStylePanel->m_Sect==NULL)
                return;

            m_Delegate(m_Shared->m_UIEdAttrStylePanel->m_Sect,m_Mode);
            return;
        case 2:
            if (m_Shared->m_UIEdAttrMaterialPanel==NULL)
                return;

            if (m_Shared->m_UIEdAttrMaterialPanel->m_Sect==NULL)
                return;

            m_Delegate(m_Shared->m_UIEdAttrMaterialPanel->m_Sect,m_Mode);
            return;
        }
    }
    void DoAction(CIwAttrInstance* inst,int mode)
    {
        m_Shared->Change(inst,mode);
    }
};

//------------------------------------------------------------------------------
void FindGroups(CIwAttrInstance* inst,int level,wxArrayString& strings,std::vector<CIwAttrInstance*>& insts,char type='l')
{
    for (int i=0; i<(int)inst->m_Data.size(); i++)
    {
        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
        {
            wxTreeItemId id;
            CIwAttrInstance* inst2=inst->m_Data[i]->m_Items[0].m_Inst;
            CIwAttrData* data=inst2->FindData(L"name",0);
            if (data==NULL)
                continue;

            wxString name=data->m_Items[0].m_String;

            if (name.Contains(L"/"))
            {
                if (!name.Contains(L":"))
                    continue;

                if (name[name.Find(L":")+1]!=type)
                    continue;
            }
            else
            {
                strings.Add(name);
                insts.push_back(inst2);
            }

            if (level<1)
                FindGroups(inst2,level+1,strings,insts);
        }
    }
}

//------------------------------------------------------------------------------
void ChooseGroup(CIwAttrInstance* inst,int mode)
{
    CUIEdAttrShared* shared=&CIwTheHost.m_Shared;
    wxArrayString strings;
    std::vector<CIwAttrInstance*> insts;

    if (shared->m_Project.m_Palette.m_Group.m_Inst==NULL)
        return;

    FindGroups(shared->m_Project.m_Palette.m_Group.m_Inst,0,strings,insts);

    wxSingleChoiceDialog dlg(CIwTheFrame,L"Please choose type of layout to group these elements by",L"Group Elements",strings);
    if (dlg.ShowModal()!=wxID_OK)
        return;

    shared->Change(insts[dlg.GetSelection()],CHANGETYPE_GROUP|SELSOURCE_INSERT,inst);
}

//------------------------------------------------------------------------------
void SetLayout(CIwAttrInstance* inst,int mode)
{
    CUIEdAttrShared* shared=&CIwTheHost.m_Shared;
    wxArrayString strings;
    std::vector<CIwAttrInstance*> insts;

    if (shared->m_Project.m_Palette.m_Group.m_Inst==NULL)
        return;

    FindGroups(shared->m_Project.m_Palette.m_Group.m_Inst,0,strings,insts);

    wxSingleChoiceDialog dlg(CIwTheFrame,L"Please choose type of layout to group these elements by",L"Set Layout",strings);
    if (dlg.ShowModal()!=wxID_OK)
        return;

    shared->Change(insts[dlg.GetSelection()],CHANGETYPE_SETLAYOUT|SELSOURCE_INSERT,inst);
}

//------------------------------------------------------------------------------
void SizerPolicy(CIwAttrInstance* inst,int mode)
{
    CUIEdAttrShared* shared=&CIwTheHost.m_Shared;
    wxArrayString strings;
    std::vector<CIwAttrInstance*> insts;

    if (shared->m_Project.m_Palette.m_Group.m_Inst==NULL)
        return;

    FindGroups(shared->m_Project.m_Palette.m_Group.m_Inst,0,strings,insts,'p');

    wxSingleChoiceDialog dlg(CIwTheFrame,L"Please choose type of sizer policy to apply to these elements",L"Sizer Policy",strings);
    if (dlg.ShowModal()!=wxID_OK)
        return;

    shared->Change(insts[dlg.GetSelection()],CHANGETYPE_SIZERPOLICY,inst);
}

//------------------------------------------------------------------------------
void SetOrientation(int orient)
{
    bool portrait(orient==0);

    int longer,shorter;
    if (CIwTheHost.m_Shared.m_Project.m_Width>CIwTheHost.m_Shared.m_Project.m_Height)
    {
        longer = CIwTheHost.m_Shared.m_Project.m_Width;
        shorter= CIwTheHost.m_Shared.m_Project.m_Height;
    }
    else
    {
        longer = CIwTheHost.m_Shared.m_Project.m_Height;
        shorter= CIwTheHost.m_Shared.m_Project.m_Width;
    }

    if (portrait)
    {
        CIwTheHost.m_Shared.m_ViewerFrame->SetViewerSize(shorter,longer); // Portrait
    }
    else
    {
        CIwTheHost.m_Shared.m_ViewerFrame->SetViewerSize(longer,shorter); // Landscape

    }

    CIwTheFrame->Update();
}

//------------------------------------------------------------------------------
int GetOrientation()
{
    return (CIwTheHost.m_Shared.m_Project.m_Width<CIwTheHost.m_Shared.m_Project.m_Height) ? 0 : 1;
}

//------------------------------------------------------------------------------
int GetHWSWMode()
{
    return CIwTheHost.m_Shared.m_Project.m_HWSWMode;
}

//------------------------------------------------------------------------------
bool GetHWSWMask(int value)
{
    /*	wxString HWSWMask=L"HS";
        Project.Get(L"ResBuildStyleRestrict",HWSWMask);

        switch(value)
        {
            case 0:
                return HWSWMask[0]=='H';
            case 1:
            case 3:
                return HWSWMask[1]=='S';
            case 2:
                return HWSWMask[0]=='H' && HWSWMask[1]=='S';
        }*/

    return true;
}

//------------------------------------------------------------------------------
void SetHWSWMode(int mode)
{
    bool should_restart=CIwTheHost.m_Shared.m_Project.m_HWSWMode!=mode;
    CIwTheHost.m_Shared.m_Project.m_HWSWMode=mode;

    if (should_restart)
    {
        CIwTheHost.m_Shared.m_Project.SetChanged(true);
        CIwTheHost.End(true);
    }
}

//------------------------------------------------------------------------------
class CIwDirectorProjList : public CIwAction
{
    int m_ID;
public:
    CIwDirectorProjList(CIwModule* module) : CIwAction(module) {}
    virtual void Action(int value=1)
    {
        wxString Data;
        if (Project.Get(L"project",Data,value))
        {
            Project.Save(true);
            Project.Set(L"currproject",Data);
            Project.Read(Data);
            Project.Save(false);

            Project.GetFile(L"data",Data);
            if (Data.empty())
                ((CIwUIEdModule*)m_Module)->SetupProjectDlg();
            else
                ((CIwUIEdModule*)m_Module)->SetupProject2();
        }
    }

    virtual void OverrideMenu(wxMenuItem* Item,int& id)
    {
        m_ID=id;

        CIwActionMenu* menu=new CIwActionMenu(this);
        wxString Data;
        int i=0;
        while (Project.Get(L"project",Data,i++))
        {
            menu->Append(id++,Data);
        }

        Item->SetSubMenu(menu);
    }
};

void NewProject() { CIwTheHost.m_Shared.NewProject(false); }
void NewProject2() { CIwTheHost.m_Shared.NewProject(true); }
void LoadProject() { CIwTheHost.m_Shared.LoadProject(); }
void SaveProject() { CIwTheHost.m_Shared.SaveProject(); }
void StartViewer() { CIwTheHost.m_Shared.StartViewer(); }
void NewUI() { CIwTheHost.m_Shared.MakeUI(NULL); }
void SaveUI()
{
    if (CIwTheHost.m_Shared.m_CurrUI==NULL) return;

    CIwTheHost.m_Shared.m_CurrUI->CheckSave(true);

    int num=0;
    CIwTheHost.m_Shared.m_Project.m_GroupData->SaveFiltered(CIwTheHost.m_AltGroupFile,num);
}
void ChangeLocalisation() { CIwTheHost.m_Shared.ChangeLocalisation(); }

//------------------------------------------------------------------------------
void CIwUIEdModule::OnInit()
{
    SetupProject();

    CIwAttrDescMgr::s_LiveEditing=true;
    CIwTheApp->m_ExtraData[UIED_EXTRA_DATA_ID]=this;

    CIwTheApp->AddAction(L"uied_pnew",new CIwActionDelegate(this,FastDelegate0<>(NewProject)));
    CIwTheApp->AddAction(L"uied_pgroup",new CIwActionDelegate(this,FastDelegate0<>(NewProject2)));
    CIwTheApp->AddAction(L"uied_pload",new CIwActionDelegate(this,FastDelegate0<>(LoadProject)));
    CIwTheApp->AddAction(L"uied_psave",new CIwActionDelegate(this,FastDelegate0<>(SaveProject)));

    CIwTheApp->AddAction(L"uied_new",new CIwActionDelegate(this,FastDelegate0<>(NewUI)));
    CIwTheApp->AddAction(L"uied_save",new CIwActionDelegate(this,FastDelegate0<>(SaveUI)));

    //CIwTheApp->AddAction(L"host_refresh",new CIwActionDelegate(this,FastDelegate0<>(RefreshConsole)));

    CIwTheApp->AddAction(L"uied_setupproject",new CIwActionDelegate(this,MakeDelegate(this,&CIwUIEdModule::SetupProjectDlg)));
    CIwTheApp->AddAction(L"uied_projects",new CIwDirectorProjList(this));

    CIwTheApp->AddAction(L"uied_orient",new CIwActionDelegateList(this,FastDelegate1<int>(SetOrientation),FastDelegate0<int>(GetOrientation)));
    CIwTheApp->AddAction(L"uied_hwswmode",new CIwActionDelegateList(this,
                                                                    FastDelegate1<int>(SetHWSWMode),FastDelegate0<int>(GetHWSWMode),FastDelegate1<int,bool>(GetHWSWMask)));
    //CIwTheApp->AddAction(L"uied_editkeymap",new CIwActionDelegate(this,FastDelegate0<>(OnEditKeyMap)));
    CIwTheApp->AddAction(L"uied_view",new CIwActionViewSize(this));

    CIwTheApp->AddAction(L"uied_autoviewer",new CIwActionDelegateBool(this,
                                                                      FastDelegate1<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::SetAutoViewer),
                                                                      FastDelegate0<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::GetAutoViewer)));
    CIwTheApp->AddAction(L"uied_assertbox",new CIwActionDelegateBool(this,
                                                                     FastDelegate1<bool>(&m_Host,&CIwHost::SetAssertBox),
                                                                     FastDelegate0<bool>(&m_Host,&CIwHost::GetAssertBox)));
    CIwTheApp->AddAction(L"uied_outlinehierarchy",new CIwActionDelegateBool(this,
                                                                            FastDelegate1<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::SetOutlineHierarchy),
                                                                            FastDelegate0<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::GetOutlineHierarchy)));
    CIwTheApp->AddAction(L"uied_outlinesiblings",new CIwActionDelegateBool(this,
                                                                           FastDelegate1<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::SetOutlineSiblings),
                                                                           FastDelegate0<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::GetOutlineSiblings)));
    CIwTheApp->AddAction(L"uied_outlinemargins",new CIwActionDelegateBool(this,
                                                                          FastDelegate1<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::SetOutlineMargins),
                                                                          FastDelegate0<bool>(&m_Host.m_Shared.m_Project,&CUIEdProject::GetOutlineMargins)));
    CIwTheApp->AddAction(L"uied_start",new CIwActionDelegate(this,FastDelegate0<>(StartViewer)));

    CIwTheApp->AddAction(L"uied_remove",new CIwActionInst(this,&CIwTheHost.m_Shared,CHANGETYPE_REMOVE));
    CIwTheApp->AddAction(L"uied_group",new CIwActionInst(this,&CIwTheHost.m_Shared,0,FastDelegate2<CIwAttrInstance*,int>(ChooseGroup)));
    CIwTheApp->AddAction(L"uied_layout",new CIwActionInst(this,&CIwTheHost.m_Shared,0,FastDelegate2<CIwAttrInstance*,int>(SetLayout)));
    CIwTheApp->AddAction(L"uied_sizerpolicy",new CIwActionInst(this,&CIwTheHost.m_Shared,0,FastDelegate2<CIwAttrInstance*,int>(SizerPolicy)));
    CIwTheApp->AddAction(L"uied_localisation",new CIwActionDelegate(this,FastDelegate0<>(ChangeLocalisation)));

    CIwTheApp->m_DirList[L"currproject"]=L"UI Builder (Beta Version)";

    CIwTheFileModule->m_LogSaves=true;
}

//------------------------------------------------------------------------------
void CIwUIEdModule::SetupProject()
{
    wxString OldData,Data;
    Project.GetFile(L"data",OldData);

    wxString Line=wxGetCwd()+L"/data/";
    if (wxDir::Exists(Line)) //is there a data sub project, if so use that
    {
        wxString File=Line+L"GlobalShared.txt";
        if ((wxFileName::FileExists(File)) || !Project.HasPath()) //if found global shared in <cwd>\data or project not set up, use <cwd>\data
        {
            Project.SetPath(Line);
            Project.m_UserPath=Line;
        }
    }   //otherwise use registry entry

    Project.ReadGlobals();

    if (Project.Get(L"sourcecontrol",Data))
        Data.Prepend(Project.GetPath());

    if (!CIwTheFileSrcCtrl.Load(Data))
        CIwTheFileSrcCtrl.m_LocalRoot=wxGetCwd();

    Project.AddTemp(L"localroot",CIwTheFileSrcCtrl.m_LocalRoot,false);

    int i=0;
    while (Project.Get(L"local",Data,i++))
    {
        wxString Data2;
        bool Found=false;
        int j=0;
        while (Project.Get(L"project",Data2,j++))
            if (Data2.IsSameAs(Data,false))
            {
                Found=true;
                break;
            }

        if (!Found)
            Project.Add(L"project",Data,false,false);
    }

    if (Project.Get(L"useSC",Data) && !Data.empty() && Data.IsSameAs(L"true",false))
    {
        CIwTheFileSrcCtrl.m_Active=true;
        CIwTheFileSrcCtrl.m_Backup=false;
    }
    else
    {
        CIwTheFileSrcCtrl.m_Active=false;

        if (Project.Get(L"useBackup",Data) && !Data.empty() && Data.IsSameAs(L"true",false))
            CIwTheFileSrcCtrl.m_Backup=true;
        else
            CIwTheFileSrcCtrl.m_Backup=false;
    }

    if (!Project.GetFile(L"viewerdata",Data))    //get viewers data path
    {
        Data=Project.GetPath();
        Project.AddTemp(L"viewerdata",Data,false);
    }

    // load the metabase from working \data below working directory
    if (Project.GetFile(L"viewerdata",Data))
    {
        Data+=METABASE_FILENAME;

        if (wxFileName::FileExists(Data))
            CIwTheFileMetaMgr.Load(Data,0);
    }

    Data=CIwTheApp->MakeAbsoluteFilename(L"{viewer}" METAUI_FILENAME);
    if (wxFileName::FileExists(Data))
        CIwTheFileMetaMgr.Load(Data,1);

    Project.GetFile(L"data",Data);
    if (!OldData.empty() && !Data.IsSameAs(OldData,false))
    {
        //RemoveViewerTemp(OldData);
    }
}

//------------------------------------------------------------------------------
void CIwUIEdModule::PostWindowInit()
{
    m_Host.m_Shared.m_Project.Init();
    m_Host.m_Shared.Refresh();

    wxString Data;
    m_Host.m_ShowAssertBox=(Project.Get(L"assertbox",Data) && Data.IsSameAs(L"True",false));
    CIwTheFrame->m_Elements->Layout();
}

//------------------------------------------------------------------------------
CIwLayoutElement* CIwUIEdModule::MakeElement(const wxString& type)
{
    if (type.IsSameAs(L"uiviewer",false))
    {
        m_Pane=new CUIEdPane(m_Host);
        return m_Pane;
    } /*
         if(type.IsSameAs(L"console",false)) {
         m_Console=new CHostUIConsolePane;
         return m_Console;
         }*/

    if (type.IsSameAs(L"uied_united",false))
        return new CUIEdAttrPane(&m_Host.m_Shared,ATTRPANETYPE_UNITY);

    if (type.IsSameAs(L"uied_styleed",false))
        return new CUIEdAttrPane(&m_Host.m_Shared,ATTRPANETYPE_STYLE);

    if (type.IsSameAs(L"uied_materialed",false))
        return new CUIEdAttrPane(&m_Host.m_Shared,ATTRPANETYPE_MATERIAL);

    if (type.IsSameAs(L"uied_palette",false))
        return new CUIEdPalettePane(&m_Host.m_Shared);

    if (type.IsSameAs(L"uied_project",false))
        return new CUIEdProjectPane(&m_Host.m_Shared);

    if (type.IsSameAs(L"uied_source",false))
        return new CIwUIEdSourceLayout(&m_Host.m_Shared);

    if (type.IsSameAs(L"uied_media",false))
        return new CIwUIEdMediaPane(&m_Host.m_Shared);

    return NULL;
}

//------------------------------------------------------------------------------
CIwUIEdModule::CIwUIEdModule() : CIwModule(L"uied_"),m_Root(NULL)
{
    /*int argc	= CIwTheApp->argc;
       wxChar** argv = CIwTheApp->argv;
       for(int i=1;i<argc;++i)
       {
        const wxChar* ch = wxStrstr(argv[i], L"-layout");
        if (ch)
        {
            wxChar line[512];
            wxStrcpy(line,ch);

            wxChar* colon = wxStrstr(line,L":");
            if(colon)
            {
       ++colon;
                wxChar* openquote = wxStrstr(colon,L"\"");
                if(openquote)
                {
                    colon = openquote+1;
                }

                wxChar* closequote = wxStrstr(colon,L"\"");
                if(closequote)
                {
       *closequote = 0;
                }

                CIwTheApp->m_DirList[L"layout"]=colon;
            }
        }
       }*/

    wxLogMessage(L"Preparing UI Builder...");
}

//------------------------------------------------------------------------------
void RegisterModules(CIwASDApp* App)
{
    App->AddModule(FILE_MODULE_ID,new CIwFileModule);
    App->AddModule(UIED_MODULE_ID,new CIwUIEdModule);
}

//------------------------------------------------------------------------------
CIwUIEdModule::~CIwUIEdModule()
{
    if (m_Root!=NULL)
        m_Root->DecRef();
}

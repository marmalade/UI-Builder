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
#include "wx/wupdlock.h"

//--------------------------------------------------------------------------------
void CIwUIEdFileDataSource::Save()
{
    if (m_FileName.empty())
        return;

    wxTextFile fp(m_FileName);

    std::vector<wxString> lines;
    Split(m_Buffer,lines,L"\n\r");

    for (int i=0; i<(int)lines.size(); i++)
    {
        fp.AddLine(lines[i]);
    }

    fp.Write();
}

//--------------------------------------------------------------------------------
void CIwUIEdFileDataSource::Load(bool force)
{
    if (!force && m_Buffer.size() > 0)
        return;

    bool readOnly = true;
    bool noteditable = false;

    m_Buffer = L"<Uninitialized>";

    if (m_FileName.empty())
        return;

    int BufferLen = 0;
    FILE* fp = _wfopen(m_FileName.c_str(), L"rb");
    if (fp == NULL)
        return;

    fseek(fp, 0, SEEK_END);

    BufferLen = ftell(fp);

    char* Buffer = new char[BufferLen+1];
    fseek(fp,0,SEEK_SET);
    fread(Buffer,1,BufferLen,fp);
    Buffer[BufferLen]=0;

    fclose(fp);

    int i, j;
    for (i=0; i<BufferLen; i++)
    {
        if (Buffer[i]<32 && (Buffer[i]!='\t' && Buffer[i]!='\n' && Buffer[i]!='\r'))
            break;
    }

    if (i == BufferLen)
        m_Buffer=wxString(Buffer,wxConvUTF8);
    else if (BufferLen > 0)
    {
        bool Truncate=false;
        if (BufferLen > 32*16)
        {
            Truncate=true;
            BufferLen =32*16;
        }

        m_Buffer.clear();
        for (i=0; i<BufferLen+15; i+=16)
        {
            m_Buffer.append(wxString::Format(L"%08x  ",i));

            for (j=0; j<16; j++)
            {
                if (j+i < BufferLen)
                    m_Buffer.append(wxString::Format(L"%02x ",Buffer[j+i]));
                else
                    m_Buffer.append(L"   ");

                if (j==7)
                    m_Buffer.append(L" ");
            }
            m_Buffer.append(L" ");

            for (j=0; j<16 && j+i<BufferLen; j++)
            {
                if (Buffer[i+j]<32 || Buffer[i+j]>126)
                    m_Buffer.append(L".");
                else
                    m_Buffer.append(1,Buffer[i+j]);
            }
            m_Buffer.append(L"\n");
        }
        if (Truncate)
            m_Buffer.append(L"<File too long to view truncated here...>");

        noteditable=true;
    }

    delete[] Buffer;

    if (noteditable)
        SetState(STATE_LOADED|STATE_NOTEDITABLE,STATE_LOADED|STATE_READWRITE|STATE_NOTEDITABLE);
    else if (!readOnly)
        SetState(STATE_LOADED|STATE_READWRITE,STATE_LOADED|STATE_READWRITE);
    else
        SetState(STATE_LOADED,STATE_LOADED|STATE_READWRITE);
}

//------------------------------------------------------------------------------
CUIEdProjectGroupLine::~CUIEdProjectGroupLine()
{
    if (m_Group!=NULL) delete m_Group;

    if (m_UI!=NULL) delete m_UI;
}

//------------------------------------------------------------------------------
CUIEdProjectGroupLine* CUIEdProjectGroup::GetLine(CUIEdProjectUI* ui)
{
    for (int i=0; i<(int)m_Lines.size(); i++)
    {
        if (m_Lines[i]->m_Group!=NULL)
        {
            CUIEdProjectGroupLine* line=m_Lines[i]->m_Group->GetLine(ui);
            if (line!=NULL)
                return line;
        }

        if (m_Lines[i]->m_UI==ui)
            return m_Lines[i];
    }
    return NULL;
}

//------------------------------------------------------------------------------
bool CUIEdProjectGroup::SaveFiltered(wxString& fileName, int& num)
{
    bool doFilter = false;
    int i;

    fileName = L"";

    for (i=0; i<(int)m_Lines.size(); i++)
    {
        m_Lines[i]->m_AltFileName=L"";

        if (m_Lines[i]->m_FileName.empty())
            continue;

        if (!wxFileExists(m_Lines[i]->m_FileName))
            doFilter=true;
    }

    if (doFilter)
    {
        wxFileName name(m_FileName);
        fileName=wxString::Format(L"/_viewertemp/temp%d.%s",num++,name.GetExt().c_str());

        wxTextFile fp(CIwTheHost.m_Shared.m_Project.m_RootDir+fileName);
        for (i=0; i<(int)m_Lines.size(); i++)
        {
            if (!m_Lines[i]->m_FileName.empty() && !wxFileExists(m_Lines[i]->m_FileName))
                continue;

            wxString line=m_Lines[i]->m_Line;
            if (!m_Lines[i]->m_AltFileName.empty())
                line=m_Lines[i]->m_AltFileName;
            else if (!m_Lines[i]->m_FileName.empty())
                line=L"/"+m_Lines[i]->m_FileName.Mid(CIwTheHost.m_Shared.m_Project.m_RootDir.size());

            if (!m_Lines[i]->m_Quote.empty())
                fp.AddLine(m_Lines[i]->m_Prefix+m_Lines[i]->m_Quote+line+m_Lines[i]->m_Quote+m_Lines[i]->m_Extra);
            else
                fp.AddLine(m_Lines[i]->m_Prefix+line+m_Lines[i]->m_Extra);
        }

        fp.Write();
    }

    return doFilter;
}

//------------------------------------------------------------------------------
void CUIEdProjectGroup::CheckSave(bool force)
{
    int i;
    for (i=0; i<(int)m_Lines.size(); i++)
    {
        if (m_Lines[i]->m_Group!=NULL)
            m_Lines[i]->m_Group->CheckSave(force);

        if (m_Lines[i]->m_UI!=NULL)
            m_Lines[i]->m_UI->CheckSave(force);
    }

    if (!HasChanged() && !force) return;

    wxTextFile fp(m_FileName);
    for (i=0; i<(int)m_Lines.size(); i++)
    {
        if (!m_Lines[i]->m_Quote.empty())
            fp.AddLine(m_Lines[i]->m_Prefix+m_Lines[i]->m_Quote+m_Lines[i]->m_Line+m_Lines[i]->m_Quote+m_Lines[i]->m_Extra);
        else
            fp.AddLine(m_Lines[i]->m_Prefix+m_Lines[i]->m_Line+m_Lines[i]->m_Extra);
    }

    fp.Write();

    SetChanged(false);
}

//------------------------------------------------------------------------------
void CUIEdProjectGroup::GetStrings(wxArrayString& strings,const std::vector<wxString>& exts)
{
    for (int i=0; i<(int)m_Lines.size(); i++)
    {
        if (m_Lines[i]->m_Group!=NULL)
            m_Lines[i]->m_Group->GetStrings(strings,exts);

        if (!m_Lines[i]->m_FileName.empty())
        {
            wxFileName name(m_Lines[i]->m_FileName);
            for (int i=0; i<(int)exts.size(); i++)
            {
                if (exts[i].IsSameAs(name.GetExt(),false))
                {
                    strings.Add(name.GetName());
                    break;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::GetStrings(EIwAttrDialogStringType type,wxArrayString& strings,CIwAttrData* data)
{
    CIwAttrClass* klass=data->m_Member->m_Items[0].m_Class;
    std::vector<wxString> exts;

    switch (type)
    {
    case ATTRSTRING_FILETEXTURE:
        if (m_Project.m_GroupData==NULL)
            break;

        exts.push_back(L"tga");
        exts.push_back(L"bmp");
        exts.push_back(L"png");
        exts.push_back(L"gif");
        m_Project.m_GroupData->GetStrings(strings,exts);
        break;
    case ATTRSTRING_FILEFONT:
        if (m_Project.m_GroupData==NULL)
            break;

        exts.push_back(L"gxfont");
        m_Project.m_GroupData->GetStrings(strings,exts);
        break;
    case ATTRSTRING_PTR:
        if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwPropertySet",false))
            GetPropertySetStrings(strings,data);
        else if (klass!=NULL && klass->m_Name.IsSameAs(L"CIwMaterial",false))
            for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_MaterialDict.begin(); it!=m_MaterialDict.end(); ++it)
            {
                strings.Add(it->first);
            }

        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::GetPropertySetStrings(wxArrayString& strings,CIwAttrData* data)
{
    bool usesContainer=false;
    CIwAttrClass* klass=data->m_Member->m_Items[0].m_Class;

    if (data->m_Member->m_Items.size()>1)
        klass=data->m_Member->m_Items[1].m_Class;
    else
    {
        klass=data->m_Instance->m_Class;
        usesContainer=true;
    }

    GetPropertySetStrings(strings,klass,usesContainer);

    for (klass=data->m_Instance->m_Class; klass!=NULL; klass=klass->m_Parent)
    {
        for (int i=0; i<(int)klass->m_Members.size(); i++)
        {
            if ((klass->m_Members[i]->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_TEMPLATE)
                GetPropertySetStrings(strings,klass->m_Members[i]->m_Items[0].m_Class,false);
        }
    }
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::GetPropertySetStrings(wxArrayString& strings,CIwAttrClass* klass,bool usesContainer)
{
    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_PropSetDict.begin(); it!=m_PropSetDict.end(); ++it)
    {
        if (klass->m_Name.IsSameAs(L"CIwPropertySet",false) || klass->m_Name.IsSameAs(L"CIwUIPropertySet",false))
        {
            strings.Add(it->first);
            continue;
        }

        int i;
        CIwAttrInstance* inst=it->second.m_Inst;
        for (i=0; i<(int)inst->m_Data.size(); i++)
        {
            if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_EXTRACLASS)
            {
                if (usesContainer)
                {
                    CIwAttrClass* klass2=NULL;
                    for (klass2=klass; klass2!=NULL; klass2=klass2->m_Parent)
                    {
                        if (klass2==inst->m_Data[i]->m_Items[0].m_Class)
                            break;
                    }
                    if (klass2!=NULL) break;
                }
                else
                {
                    CIwAttrClass* klass2=NULL;
                    for (klass2=inst->m_Data[i]->m_Items[0].m_Class; klass2!=NULL; klass2=klass2->m_Parent)
                    {
                        if (klass2==klass)
                            break;
                    }
                    if (klass2!=NULL) break;
                }
            }
        }


        if (i<(int)inst->m_Data.size())
            strings.Add(it->first);
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrSharedExtra::GetStrings(EIwAttrDialogStringType Type,wxArrayString& Strings,CIwAttrData* Data)
{
    m_Parent->GetStrings(Type,Strings,Data);
}

//------------------------------------------------------------------------------
wxString CUIEdAttrShared::MakeNew(EIwAttrDialogStringType type,const wxString& klass,const wxString& klass2,const wxString& parent)
{
    wxArrayString strings;
    std::vector<wxString> exts;
    wxString typeName;
    int i,sel=-1;

    switch (type)
    {
    case ATTRSTRING_FILETEXTURE:
        typeName=L"Texture";
        GetFileList(strings,m_Project.m_GroupData,2);
        break;
    case ATTRSTRING_FILEFONT:
        typeName=L"Font";
        GetFileList(strings,m_Project.m_GroupData,2);
        break;
    case ATTRSTRING_PTR:
        if (klass.IsSameAs(L"CIwPropertySet",false))
        {
            //if (m_StyleList.empty()) {
            GetFileList(strings,m_Project.m_GroupData,0);
            typeName=L"Property Set";
            for (i=0; i<(int)strings.size(); i++)
            {
                if (strings[i].IsSameAs(m_Project.m_PropSet,false))
                {
                    sel=i;
                    break;
                }
            }
            //}
            if (!m_StyleList.empty())
            {
                sel=(int)strings.size();
                strings.Add(L"-= current style =-");
            }
        }
        else if (klass.IsSameAs(L"CIwMaterial",false))
        {
            GetFileList(strings,m_Project.m_GroupData,1);
            typeName=L"Material";
            for (i=0; i<(int)strings.size(); i++)
            {
                if (strings[i].IsSameAs(m_Project.m_Material,false))
                {
                    sel=i;
                    break;
                }
            }
        }
        else
            return L"";

        break;
    default:
        break;
    }
    wxString select;
    wxSingleChoiceDialog dlg(CIwTheFrame,L"Which file should the "+typeName+L" be added to?",L"Add "+typeName,strings);
    if (sel!=-1)
        dlg.SetSelection(sel);

    if (strings.size()>1)
    {
        if (dlg.ShowModal()!=wxID_OK)
            return L"";

        select=dlg.GetStringSelection();
    }
    else
        select=strings[0];

    if (type==ATTRSTRING_PTR)
    {
        CIwAttrNote Note;
        std::vector<wxString> argv2;

        wxTextEntryDialog dlg2(CIwTheFrame,L"Choose a name for the new "+typeName,L"Add "+typeName);
        if (dlg2.ShowModal()!=wxID_OK)
            return L"";

        CIwAttrInstance* to=NULL;
        CUIEdProjectUI* file=NULL;
        bool stylesheet=false;

        if (select==L"-= current style =-" && klass.IsSameAs(L"CIwPropertySet",false))
        {
            for (i=0; i<(int)m_StyleList.size(); i++)
            {
                if (m_StyleList[i].m_StyleSheet==m_Project.m_StyleSheet)
                    break;
            }
            if (i==(int)m_StyleList.size())
                i=0;

            to=m_StyleList[i].m_Inst;
            file=m_StyleList[i].m_UI;
            stylesheet=true;
        }
        else
        {
            file=GetFile(select,m_Project.m_GroupData);
            to=file->m_Group.m_Inst;
        }

        if (file==NULL) return L"";

        if (klass.IsSameAs(L"CIwPropertySet",false))
            Note.m_Name=L"CIwUIPropertySet";
        else
            Note.m_Name=klass;

        Note.m_Data=L"{";
        argv2.push_back(Note.m_Data);

        CIwAttrInstance* inst=to->AddFromNote(Note.m_Name,Note,argv2,to);
        if (inst==NULL) return L"";

        for (i=0; i<(int)to->m_Data.size(); i++)
        {
            if (to->m_Data[i]==inst->m_Parent)
                break;
        }

        int offset=i;

        for (i=0; i<(int)to->m_Data.size(); i++)
        {
            if (to->m_Data[i]==inst->m_Parent)
                break;

            if ((to->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD &&
                !to->m_Data[i]->m_Member->m_Name.IsSameAs(L"CIwPropertySet",false))
            {
                to->m_Data.erase(to->m_Data.begin()+offset);
                to->m_Data.insert(to->m_Data.begin()+i,inst->m_Parent);
                break;
            }
        }

        Note.m_Name=L"name";
        Note.m_Data=dlg2.GetValue();
        argv2.clear();
        argv2.push_back(Note.m_Data);

        inst->AddFromNote(Note.m_Name,Note,argv2,to);

        if (klass.IsSameAs(L"CIwPropertySet",false))
        {
            if (!klass2.empty())
            {
                Note.m_Name=L"extraclass";
                Note.m_Data=klass2;
                argv2.clear();
                argv2.push_back(Note.m_Data);

                inst->AddFromNote(Note.m_Name,Note,argv2,to);
            }

            if (!parent.empty())
            {
                Note.m_Name=L"parent";
                Note.m_Data=parent;
                argv2.clear();
                argv2.push_back(Note.m_Data);

                inst->AddFromNote(Note.m_Name,Note,argv2,to);
            }

            m_Project.m_PropSet=select;
            AddPropSet(inst,file);
            if (stylesheet)
            {
                CIwAttrData* name=inst->FindData(L"name");
                if (name!=NULL)
                    m_PropSetDict[name->m_Items[0].m_String].m_StyleSheet=m_Project.m_StyleSheet;
            }

            if (m_UIEdAttrStylePanel!=NULL)
            {
                m_UIEdAttrStylePanel->Reset();
                m_UIEdAttrStylePanel->Select(m_UIEdAttrStylePanel->m_Tree->GetRootItem(),inst);
            }

            SendChanges(inst);
        }
        else
        {
            m_Project.m_Material=select;
            AddMaterial(inst,file);

            if (m_UIEdAttrMaterialPanel!=NULL)
            {
                m_UIEdAttrMaterialPanel->Reset();
                m_UIEdAttrMaterialPanel->Select(m_UIEdAttrMaterialPanel->m_Tree->GetRootItem(),inst);
            }

            SendChanges(inst);
        }

        return dlg2.GetValue();
    }
    else
    {
        CUIEdProjectGroup* group=GetFileGroup(select,m_Project.m_GroupData);
        if (group==NULL) return L"";

        wxString wildcard;
        if (type==ATTRSTRING_FILETEXTURE)
            wildcard=L"Texture Files (*.tga;*.bmp;*.png;*.gif)|*.tga;*.bmp;*.png;*.gif|All Files (*.*)|*.*";
        else
            wildcard=L"IwUI Font Files (*.gxfont)|*.gxfont|All Files (*.*)|*.*";

        wxFileDialog dlg2(CIwTheFrame,L"Please choose a "+typeName+L" file",m_Project.m_RootDir,L"",wildcard,wxOPEN|wxFILE_MUST_EXIST);
        if (dlg2.ShowModal()!=wxID_OK)
            return L"";

        wxString mainDir=m_Project.m_RootDir;
        wxString file=dlg2.GetPath();

        wxFileName name(file);

        if (!CIwTheHost.StartsWith(file,mainDir))
        {
            wxMessageBox(L"File must be under the Root Directory",L"Add File");
            return L"";
        }

        wxString line=dlg2.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");
        if (!group->HasFile(line))
        {
            char text[256];
            wxFileName name(group->m_FileName);
            wxFileName name2(line);

            group->AddLineStart(L"\t\""+line+L"\"");

            wxTextFile fp2(CIwTheHost.m_Shared.m_Project.m_RootDir+L"//_viewertemp//fromui.group");
            fp2.AddLine(L"CIwResGroup");
            fp2.AddLine(L"{");
            fp2.AddLine(wxString::Format(L"\tname \"_viewertemp\""));
            fp2.AddLine(L"\t\""+line+L"\"");
            fp2.AddLine(L"}");
            fp2.Write();

            //sprintf(text,"\"%s\" \"%s\" \"\" 3",name.GetName().mb_str(),line.mb_str());
            sprintf(text,"\"%s\" \"//_viewertemp//fromui.group\" \"%s\" 3",name.GetName().mb_str().data(),name2.GetName().mb_str().data());

            if (CIwTheHost.m_Link!=NULL)
                CIwTheHost.m_Link->SetData(CIwViewerUI::UPDATED_ELEMENTS,text);
        }

        m_ProjectFrame->Reset();

        return name.GetName();
    }

    return L"";
}

static const wxChar* iconList[]={
    L"edit_change16x16.png;Change",
    L"override16x16.png;Override",
    L"reset16x16.png;Reset",
    L"goto16x16.png;Go To",
    L"remove16x16.png;Remove",
    //L"level.png;1->2->4",
    L"add16x16.png;Add",
    NULL
};

//------------------------------------------------------------------------------
void CUIEdAttrShared::SetupIcons()
{
    wxString dir=CIwTheApp->MakeAbsoluteFilename(L"{viewer}Icons/16x16/");

    for (int i=0; iconList[i]!=NULL; i++)
    {
        wxString file=dir+wxString(iconList[i]).BeforeFirst(';');
        file.Replace(L"\\",L"/");
        m_IconList.push_back(wxBitmap(file,wxBITMAP_TYPE_PNG));
    }
}

//------------------------------------------------------------------------------
wxBitmap* CUIEdProjectUI::Group::GetIcon(const wxString& textName)
{
    for (int k=0; iconList[k]!=NULL; k++)
    {
        wxString type=wxString(iconList[k]).AfterLast(';');

        if (type.IsSameAs(textName,false))
            return &CIwTheHost.m_Shared.m_IconList[k];
    }
    return NULL;
}

//------------------------------------------------------------------------------
void CUIEdProject::Init()
{
    wxString data;
    if (CIwTheApp->argc>1)
        if (Load(CIwTheApp->argv[1]))
            return;

    if (!Project.Get(L"uiproject",data))
        Project.GetFile(L"data",m_RootDir);
    else
        Load(data);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::LoadUI(CUIEdProjectGroupLine* line,bool uiFile)
{
    Reset();

    if (line->m_UI==NULL)
    {
        line->m_UI=new CUIEdProjectUI;
        line->m_UI->m_FileName=line->m_FileName;
        line->m_UI->Load();
        line->m_UI->SetupElements();

        AddPropSets(line->m_UI);
    }

    if (uiFile)
    {
        wxFileName name(m_Project.m_Project);
        CIwTheApp->m_DirList[L"projectdetails"]=L"Project: "+name.GetName()+L" File: "+line->m_FileName;
        CIwTheFrame->SetMainTitle();

        m_CurrUI=line->m_UI;
        RefreshUI(true);
        SetSelection(NULL,SELSOURCE_START);
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::UnLoadUI()
{
    wxFileName name(m_Project.m_Project);
    CIwTheApp->m_DirList[L"projectdetails"]=L"Project: "+name.GetName();
    CIwTheFrame->SetMainTitle();

    m_CurrUI=NULL;
    RefreshUI(true);
    SetSelection(NULL,SELSOURCE_DATA);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SendSelection(std::vector<CIwAttrInstance*>& add,std::vector<CIwAttrInstance*>& remove)
{
    int i;
    char text[256];
    if (CIwTheHost.m_Link==NULL)
        return;

    for (i=0; i<(int)add.size(); i++)
    {
        strcpy(text,GetFullName(add[0]).mb_str());
        CIwTheHost.m_Link->SetData(CIwViewerUI::ADD_SELECTION,text);
    }

    for (i=0; i<(int)remove.size(); i++)
    {
        strcpy(text,GetFullName(remove[0]).mb_str());
        CIwTheHost.m_Link->SetData(CIwViewerUI::REMOVE_SELECTION,text);
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SendChanges(CIwAttrInstance* inst)
{
    int mode=0;
    if (m_UIEdAttrStylePanel!=NULL && m_UIEdAttrStylePanel->m_Sect==inst)
        mode=1;

    if (m_UIEdAttrMaterialPanel!=NULL && m_UIEdAttrMaterialPanel->m_Sect==inst)
        mode=2;

    while (inst!=NULL && mode==0) {
        if (inst->m_Parent==NULL || inst->m_Parent->m_Instance==m_CurrUI->m_Group.m_Inst)
            break;

        inst=inst->m_Parent->m_Instance;
    }

    if (inst==NULL)
    {
        char text[256];
        sprintf(text,"\"\" \"\" %s 0",(const char*)GetFullName(m_SelElem).mb_str());
        if (CIwTheHost.m_Link!=NULL)
            CIwTheHost.m_Link->SetData(CIwViewerUI::UPDATED_ELEMENTS,text);

        return;
    }

    CIwAttrData* nameData=inst->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
    if (nameData==NULL) return;

    static int num=0;
    num=(num+1)%4;
    wxString file=wxString::Format(L"//_viewertemp//fromui%d.ui",num);
    wxTextFile fp(m_Project.m_RootDir+file);
    inst->SaveExtra(fp,mode==0,true);

    char text[256];
    CUIEdAttrPropSet* set=NULL;
    if (mode==2)
    {
        CIwAttrData* data=inst->FindData(L"name",0);
        if (data!=NULL && m_MaterialDict.find(data->m_Items[0].m_String)!=m_MaterialDict.end())
            set=&m_MaterialDict[data->m_Items[0].m_String];
    }

    if (set!=NULL)
        sprintf(text,"\"%s\" \"%s\" \"%s\" %d \"%s\"",nameData->m_Items[0].m_String.mb_str().data(),file.mb_str().data(),GetFullName(m_SelElem).mb_str().data(),mode,
                set->m_UI->m_FileName.Mid(m_Project.m_RootDir.size()).BeforeLast('/').mb_str().data());
    else
        sprintf(text,"\"%s\" \"%s\" \"%s\" %d",nameData->m_Items[0].m_String.mb_str().data(),file.mb_str().data(),GetFullName(m_SelElem).mb_str().data(),mode);

    if (CIwTheHost.m_Link!=NULL)
        CIwTheHost.m_Link->SetData(CIwViewerUI::UPDATED_ELEMENTS,text);
}

//------------------------------------------------------------------------------
wxString CUIEdAttrShared::GetFullName(CIwAttrInstance* inst)
{
    if (inst==NULL) return L"\"\"";

    wxString name;
    if (inst->m_Parent!=NULL && (inst->m_Parent->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
    {
        name=L"^"+inst->m_Parent->m_Member->m_Name;
        if (inst->m_Parent->m_Member->m_Type&ATTRMEMBER_LIST)
        {
            for (int i=0; i<(int)inst->m_Parent->m_Items.size(); i++)
            {
                if (inst->m_Parent->m_Items[i].m_Inst==inst)
                {
                    name+=wxString::Format(L"[%d]",i);
                    break;
                }
            }
        }
    }
    else
    {
        CIwAttrData* nameData=inst->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
        if (nameData!=NULL)
            name=nameData->m_Items[0].m_String;
    }

    while (inst!=NULL) {
        if (inst->m_Parent==NULL)
            break;

        inst=inst->m_Parent->m_Instance;

        CIwAttrData* nameData=inst->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
        if (nameData!=NULL)
            name=nameData->m_Items[0].m_String+L"|"+name;
    }
    return name;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::GetFromFullName(const wxString& name)
{
    std::vector<wxString> args;
    if (SuperSplit(name,args,L"|")<1)
        return NULL;

    int i,j;
    CIwAttrInstance* inst=m_CurrUI->m_Group.m_Inst;

    for (i=0; i<(int)args.size(); i++)
    {
        bool found=false;
        for (j=0; j<(int)inst->m_Data.size(); j++)
        {
            if ((inst->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD ||
                (inst->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
            {
                CIwAttrInstance* inst2=inst->m_Data[j]->m_Items[0].m_Inst;
                CIwAttrData* data=inst2->FindData(L"name",0);
                if (data==NULL) continue;

                if (data->m_Items[0].m_String.IsSameAs(args[i],false))
                {
                    inst=inst2;
                    found=true;
                    break;
                }
            }

            if ((inst->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
            {
                wxString name=args[i];
                if (name[0]=='^')
                    name=name.Mid(1);

                int offset=name.Find(L"[");
                int num=0;
                if (offset!=-1)
                {
                    num=atoi(name.Mid(offset+1,name.size()-2-offset).mb_str());
                    name=name.Mid(0,offset);
                }

                if (name.IsSameAs(inst->m_Data[j]->m_Member->m_Name,false))
                {
                    inst=inst->m_Data[j]->m_Items[num].m_Inst;
                    found=true;
                    break;
                }
            }
        }
        if (inst==NULL || !found)
            return NULL;
    }
    return inst;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::UpdateFromViewer(const wxString& name,const wxString& file,std::vector<wxString>& sel)
{
    // May already have update in progress
    if (m_UpdateName.empty())
    {
        m_UpdateName=name;
        m_UpdateFile=file;
        m_UpdateSel=sel;
        wxWakeUpIdle();
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::AddPropSets(CUIEdProjectUI* ui)
{
    int i;
    for (i=0; i<(int)ui->m_Group.m_Inst->m_Data.size(); i++)
    {
        if ((ui->m_Group.m_Inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        CIwAttrInstance* inst2=ui->m_Group.m_Inst->m_Data[i]->m_Items[0].m_Inst;
        AddPropSet(inst2,ui);
        AddMaterial(inst2,ui);
        AddStyleSheet(inst2,ui);
    }

    for (i=0; i<(int)ui->m_Group.m_Inst->m_Data.size(); i++)
    {
        if ((ui->m_Group.m_Inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        CIwAttrInstance* inst2=ui->m_Group.m_Inst->m_Data[i]->m_Items[0].m_Inst;
        for (int j=0; j<(int)inst2->m_Data.size(); j++)
        {
            if ((inst2->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
                if (inst2->m_Data[j]->m_Items[0].m_Inst==NULL)
                    inst2->m_Data[j]->m_Items[0].m_Inst=ui->m_Group.TryGetPtrItem(inst2->m_Data[j]->m_Items[0].m_String,inst2->m_Data[j]);

        }
    }
}

//------------------------------------------------------------------------------
bool CUIEdAttrWriteOverrideStylesheetRoot::OverrideDefault(CIwAttrData* data,int num,bool initial)
{
    if (m_SaveDefault)
        return initial;

    return data->m_FromDefault;
}

//------------------------------------------------------------------------------
bool CUIEdAttrWriteOverrideStylesheetInclude::Override(CIwAttrData* data,int num,int Indent,wxString& Notes)
{
    if (data->m_Instance->m_Parent!=NULL && data->m_Instance->m_Parent->m_Member->m_Name.IsSameAs(L"style",false))
        if (data->m_Member->m_Name.IsSameAs(L"name",false))
            return true;

    return !m_SaveDefault;
}

//------------------------------------------------------------------------------
bool CUIEdAttrWriteOverrideStylesheetRoot::Override(CIwAttrData* data,int num,int Indent,wxString& Notes)
{
    if (m_SaveDefault)
    {
        if (data->m_Member->m_Name.IsSameAs(L"style",false))
            return false;

        return true;
    }

    if (data->m_Member->m_Name.IsSameAs(L"style",false) && !data->m_Items.empty() && data->m_Items[0].m_Inst!=NULL &&
        data->m_Items[0].m_Inst->m_File!=data->m_Instance->m_File)
        return true;

    if (data->m_Member->m_Name.IsSameAs(L"tweak",false))
    {

        CIwAttrData* name=data->m_Items[0].m_Inst->FindData(L"name",0);
        if (name==NULL)
            return false;

        if (CIwTheHost.m_Shared.m_PropSetDict.find(name->m_Items[0].m_String)==CIwTheHost.m_Shared.m_PropSetDict.end())
            return false;

        CUIEdAttrPropSet& set=CIwTheHost.m_Shared.m_PropSetDict[name->m_Items[0].m_String];

        return (set.m_Inst==set.m_OrigInst);
    }

    if (data->m_Member->m_Name.IsSameAs(L"extraclass",false))
        return false;

    if (data->m_Instance->m_Parent!=NULL && data->m_Instance->m_Parent->m_Member->m_Name.IsSameAs(L"tweak",false))
    {
        if (data->m_Member->m_Name.IsSameAs(L"name",false))
            return true;

        CIwAttrData* name=data->m_Instance->FindData(L"name",0);
        if (name==NULL)
            return false;

        if (CIwTheHost.m_Shared.m_PropSetDict.find(name->m_Items[0].m_String)==CIwTheHost.m_Shared.m_PropSetDict.end())
            return false;

        CUIEdAttrPropSet& set=CIwTheHost.m_Shared.m_PropSetDict[name->m_Items[0].m_String];
        if (set.m_Inst==set.m_OrigInst)
            return false;

        CIwAttrData* data2=set.m_OrigInst->FindData(data->m_Member->m_Name,0);
        if (data2==NULL)
            return false;

        wxString line1,line2;
        if (data->ToString2(line1)!=NULL)
            return false;

        if (data2->ToString2(line2)!=NULL)
            return false;

        return line1==line2;
    }

    return false;
}

//------------------------------------------------------------------------------
void CUIEdAttrWriteOverrideStylesheetRoot::Append(CIwAttrData* data,int num,int Indent,wxString& Notes)
{
    if (m_SaveDefault)
        return;

    if (data->m_Member->m_Name.IsSameAs(L"name",false) && data->m_Instance->m_Class->m_Name.IsSameAs(L"CIwUIStylesheet",false))
    {
        for (int i=0; i<(int)data->m_Instance->m_File->m_SubGroups.size(); i++)
        {
            CIwAttrFileSubGroup* group=(CIwAttrFileSubGroup*)data->m_Instance->m_File->m_SubGroups[i];
            for (int k=0; k<Indent; k++)
            {
                Notes+=wxT("\t");
            }
            Notes+=L"include \"";
            Notes+=group->m_FileName.Mid(data->m_Mgr->m_BaseDir.size()+1);
            Notes+=L"\"\n";
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::AddStyleSheet(CIwAttrInstance* inst2,CUIEdProjectUI* ui)
{
    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwUIStylesheet");

    if (inst2==NULL) return;

    CIwAttrClass* klass2;
    for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
    {
        if (klass2==klass)
            break;
    }
    if (klass2==NULL)
        return;

    CIwAttrData* name=inst2->FindData(L"name");
    if (name==NULL) return;

    m_StyleList.push_back(CUIEdAttrPropSet(inst2,ui));
    m_StyleList.back().m_StyleSheet=name->m_Items[0].m_String;

    //ui->m_Group.m_Override=&m_WriteOverrideRoot;
    //for (int i=0;i<(int)ui->m_Group.m_SubGroups.size();i++)
    //    ui->m_Group.m_SubGroups[i]->m_Override=&m_WriteOverrideInclude;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::AddPropSet(CIwAttrInstance* inst2,CUIEdProjectUI* ui)
{
    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwUIPropertySet");

    if (inst2==NULL)
        return;

    CIwAttrClass* klass2;
    for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
    {
        if (klass2==klass)
            break;
    }
    if (klass2==NULL)
        return;

    CIwAttrData* name=inst2->FindData(L"name");
    CIwAttrData* parent=inst2->FindData(L"parent");
    if (name==NULL) return;

    if (m_PropSetDict.find(name->m_Items[0].m_String)!=m_PropSetDict.end())
        if (m_PropSetDict[name->m_Items[0].m_String].m_UI==ui)
            return;

    m_PropSetDict[name->m_Items[0].m_String]=CUIEdAttrPropSet(inst2,ui);
    if (parent!=NULL)
        m_PropSetDict[name->m_Items[0].m_String].m_Parent=parent->m_Items[0].m_String;
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::AddMaterial(CIwAttrInstance* inst2,CUIEdProjectUI* ui)
{
    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwMaterial");

    if (inst2==NULL)
        return;

    CIwAttrClass* klass2;
    for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
    {
        if (klass2==klass)
            break;
    }
    if (klass2==NULL)
        return;

    CIwAttrData* name=inst2->FindData(L"name");
    if (name==NULL) return;

    if (m_MaterialDict.find(name->m_Items[0].m_String)!=m_MaterialDict.end())
        return;

    m_MaterialDict[name->m_Items[0].m_String]=CUIEdAttrPropSet(inst2,ui);

    CIwAttrData* data=inst2->FindData(L"texture0");
    if (data!=NULL)
    {
        wxFileName name2(ui->m_FileName);
        wxFileName name1(m_Project.m_RootDir);
        wxString path;
        if (data->m_Items[0].m_String[0]=='.')
            path=name2.GetPath()+data->m_Items[0].m_String.Mid(1);
        else if (data->m_Items[0].m_String[0]=='\\' || data->m_Items[0].m_String[0]=='/')
            path=m_Project.m_RootDir+data->m_Items[0].m_String;
        else
            path=name2.GetPath()+data->m_Items[0].m_String;

        wxImage img(path);
        if (img.IsOk())
        {
            img.Rescale(20,20,wxIMAGE_QUALITY_HIGH);
            m_MaterialDict[name->m_Items[0].m_String].m_Bmp=wxBitmap(img);
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::RemovePropSets(CUIEdProjectUI* ui)
{
    for (std::vector<CUIEdAttrPropSet>::iterator it3=m_StyleList.begin(); it3!=m_StyleList.end(); )
    {
        if (it3->m_UI==ui)
            it3=m_StyleList.erase(it3);
        else
            ++it3;
    }

    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_PropSetDict.begin(); it!=m_PropSetDict.end(); )
    {
        if (it->second.m_UI==ui)
            m_PropSetDict.erase(it++);
        else
            it++;
    }
    for (std::map<wxString,CUIEdAttrPropSet>::iterator it1=m_MaterialDict.begin(); it1!=m_MaterialDict.end(); )
    {
        if (it1->second.m_UI==ui)
            m_MaterialDict.erase(it1++);
        else
            it1++;
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::GetFileList(wxArrayString& strings,CUIEdProjectGroup* group,int mode)
{
    if (mode==2)
        strings.Add(group->m_FileName.Mid(m_Project.m_RootDir.size()));

    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (group->m_Lines[i]->m_Group!=NULL)
            GetFileList(strings,group->m_Lines[i]->m_Group,mode);

        if (group->m_Lines[i]->m_UI!=NULL)
        {
            if (mode==0 && !group->m_Lines[i]->m_Name.EndsWith(L"mtl"))
                strings.Add(group->m_Lines[i]->m_FileName.Mid(m_Project.m_RootDir.size()));
            else if (mode==1 && group->m_Lines[i]->m_Name.EndsWith(L"mtl"))
                strings.Add(group->m_Lines[i]->m_FileName.Mid(m_Project.m_RootDir.size()));
        }
    }
}

//------------------------------------------------------------------------------
CUIEdProjectUI* CUIEdAttrShared::GetFile(const wxString& string,CUIEdProjectGroup* group)
{
    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (group->m_Lines[i]->m_UI!=NULL && group->m_Lines[i]->m_FileName.Mid(m_Project.m_RootDir.size()).IsSameAs(string,false))
            return group->m_Lines[i]->m_UI;

        if (group->m_Lines[i]->m_Group!=NULL)
        {
            CUIEdProjectUI* file=GetFile(string,group->m_Lines[i]->m_Group);
            if (file!=NULL)
                return file;
        }
    }
    return NULL;
}
//------------------------------------------------------------------------------
wxBitmap* CUIEdAttrShared::GetFileBitmap(const wxString& string,CUIEdProjectGroup* group)
{
    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (group->m_Lines[i]->m_Bmp.IsOk() && group->m_Lines[i]->m_Name.BeforeLast('.').IsSameAs(string,false))
            return &group->m_Lines[i]->m_Bmp;

        if (group->m_Lines[i]->m_Group!=NULL)
        {
            wxBitmap* bmp=GetFileBitmap(string,group->m_Lines[i]->m_Group);
            if (bmp!=NULL)
                return bmp;
        }
    }
    return NULL;
}
//------------------------------------------------------------------------------
CUIEdProjectGroup* CUIEdAttrShared::GetFileGroup(const wxString& string,CUIEdProjectGroup* group)
{
    if (group->m_FileName.Mid(m_Project.m_RootDir.size()).IsSameAs(string,false))
        return group;

    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (group->m_Lines[i]->m_Group!=NULL)
        {
            CUIEdProjectGroup* file=GetFileGroup(string,group->m_Lines[i]->m_Group);
            if (file!=NULL)
                return file;
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::ResetPtrs(CUIEdProjectGroup* group)
{
    if (group==NULL) return;

    for (int i=0; i<(int)group->m_Lines.size(); i++)
    {
        if (group->m_Lines[i]->m_Group!=NULL)
            ResetPtrs(group->m_Lines[i]->m_Group);

        if (group->m_Lines[i]->m_UI!=NULL)
            ResetPtrs(group->m_Lines[i]->m_UI->m_Group.m_Inst);
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::ResetPtrs(CIwAttrInstance* inst)
{
    for (int i=0; i<(int)inst->m_Data.size(); i++)
    {
        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS || (inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD)
            ResetPtrs(inst->m_Data[i]->m_Items[0].m_Inst);

        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR && inst->m_File!=NULL)
        {
            for (int j=0; j<(int)inst->m_Data[i]->m_Items.size(); j++)
            {
                if ((inst->m_Data[i]->m_Items[j].m_Flags&ATTRITEM_ALLOCED_F)==0)
                    inst->m_Data[i]->m_Items[j].m_Inst=inst->m_File->TryGetPtrItem(inst->m_Data[i]->m_Items[j].m_String,inst->m_Data[i]);
                else if (inst->m_Data[i]->m_Items[j].m_Inst!=NULL)
                    ResetPtrs(inst->m_Data[i]->m_Items[j].m_Inst);
            }
        }
    }
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::AddPropSetsFromStylesheet(const wxString& styleSheet)
{
    int i,j;

    for (i=0; i<(int)m_StyleList.size(); i++)
    {
        if (m_StyleList[i].m_StyleSheet==styleSheet)
        {
            CIwAttrData* parent=m_StyleList[i].m_Inst->FindData(L"parent",0);
            if (parent!=NULL)
                AddPropSetsFromStylesheet(parent->m_Items[0].m_String);

            for (j=0; j<(int)m_StyleList[i].m_Inst->m_Data.size(); j++)
            {
                if ((m_StyleList[i].m_Inst->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
                    continue;

                CIwAttrInstance* inst2=m_StyleList[i].m_Inst->m_Data[j]->m_Items[0].m_Inst;
                if (inst2==NULL) continue;

                CIwAttrData* name=inst2->FindData(L"name");
                parent=inst2->FindData(L"parent");
                if (name==NULL) continue;

                if (m_PropSetDict.find(name->m_Items[0].m_String)!=m_PropSetDict.end())
                    continue;

                m_PropSetDict[name->m_Items[0].m_String]=CUIEdAttrPropSet(inst2,m_StyleList[i].m_UI);
                m_PropSetDict[name->m_Items[0].m_String].m_StyleSheet=styleSheet;

                if (parent!=NULL)
                    m_PropSetDict[name->m_Items[0].m_String].m_Parent=parent->m_Items[0].m_String;
            }

            break;
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SetStyleSheet(const wxString& styleSheet)
{
    if (!m_Project.m_StyleSheet.empty())
        RemovePropSets(m_Project.m_StyleSheet);

    AddPropSetsFromStylesheet(styleSheet);

    m_Project.m_StyleSheet=styleSheet;
    ResetPtrs(m_Project.m_GroupData);
    if (m_Project.m_Palette.m_Group.m_Inst!=NULL)
        ResetPtrs(m_Project.m_Palette.m_Group.m_Inst);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SaveDefault(CIwAttrInstance* first)
{
    if (first==NULL) return;

    //m_WriteOverrideRoot.m_SaveDefault=true;
    //m_WriteOverrideInclude.m_SaveDefault=true;

    wxTextFile fp(first->m_File->m_FileName);
    first->m_File->m_Inst->SaveExtra(fp);

    //m_WriteOverrideRoot.m_SaveDefault=false;
    //m_WriteOverrideInclude.m_SaveDefault=false;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::ToDefault(const wxString& fromStyle)
{
    CIwAttrInstance* first=NULL;

    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_PropSetDict.begin(); it!=m_PropSetDict.end(); ++it)
    {
        CUIEdAttrPropSet& set=it->second;
        if (set.m_StyleSheet!=fromStyle)
            continue;

        CIwAttrInstance* inst2=ToDefault(fromStyle,it->first,set.m_Inst);

        if (first==NULL)
            first=inst2;
    }
    return first;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::ToDefault(const wxString& fromStyle,const wxString& name,CIwAttrInstance* inst)
{
    CIwAttrInstance* first=NULL;
    std::vector<CIwAttrNote> Notes;
    inst->FillNotes(Notes,false,0,true);

    for (int i=0; i<(int)m_StyleList.size(); i++)
    {
        for (int j=0; j<(int)m_StyleList[i].m_Inst->m_Data.size(); j++)
        {
            if (m_StyleList[i].m_Inst->m_Data[j]->m_Member->m_Name!=L"style")
                continue;

            CIwAttrInstance* inst2=m_StyleList[i].m_Inst->m_Data[j]->m_Items[0].m_Inst;
            if (inst2==NULL) continue;

            CIwAttrData* data=inst2->FindData(L"name",0);
            if (data==NULL || data->m_Items[0].m_String!=name) continue;

            if (first==NULL)
                first=inst2;

            for (int k=0; k<(int)inst2->m_Data.size(); k++)
            {
                delete inst2->m_Data[k];
            }
            inst2->m_Data.clear();
            inst2->m_ExtraData.clear();
            inst2->AddDefaults(inst2->m_Class);
            inst2->GetFromNotes(Notes);
        }
    }
    return first;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::RemovePropSets(const wxString& styleSheet)
{
    for (std::map<wxString,CUIEdAttrPropSet>::iterator it=m_PropSetDict.begin(); it!=m_PropSetDict.end(); )
    {
        std::map<wxString,CUIEdAttrPropSet>::iterator it2=it++;
        if (it2->second.m_StyleSheet==styleSheet)
            m_PropSetDict.erase(it2);
    }

    for (int i=0; i<(int)m_StyleList.size(); i++)
    {
        if (m_StyleList[i].m_StyleSheet==styleSheet)
        {
            CIwAttrData* parent=m_StyleList[i].m_Inst->FindData(L"parent",0);
            if (parent!=NULL && !parent->m_Items[0].m_String.empty())
                RemovePropSets(parent->m_Items[0].m_String);
        }
    }
}

//------------------------------------------------------------------------------
bool CUIEdProject::Load(const wxString& file)
{
    wxTextFile fp(file);
    if (!fp.Exists() || !fp.Open())
        return false;

    wxFileName name(file);
    bool gotStyle=false;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> args;
        if (SuperSplit(fp[i],args,L" \t\r\n")<2)
            continue;

        if (args[0].IsSameAs(L"dir",false))
        {
            wxFileName dir(args[1]);
            dir.Normalize(wxPATH_NORM_ALL,name.GetPath());

            m_RootDir=dir.GetFullPath();

            if (m_RootDir.EndsWith(L" "))
                m_RootDir.RemoveLast();

            if (!m_RootDir.EndsWith(L"/") && !m_RootDir.EndsWith(L"\\"))
                m_RootDir+=L"/";

            m_Project=file.Mid(m_RootDir.size());
        }
        else if (args[0].IsSameAs(L"group",false))
            m_Group=args[1];
        else if (args[0].IsSameAs(L"palette",false))
            m_Palette.m_FileName=m_RootDir+args[1];
        else if (args[0].IsSameAs(L"material",false))
            m_Material=args[1];
        else if (args[0].IsSameAs(L"propset",false))
            m_PropSet=args[1];
        else if (args[0].IsSameAs(L"restemplate",false))
            m_ResTemplate=args[1];
        else if (args[0].IsSameAs(L"autoviewer",false))
            m_AutoStartViewer=false;
        else if (args[0].IsSameAs(L"outlinehierarchy",false))
            m_OutlineHierarchy=true;
        else if (args[0].IsSameAs(L"outlinesiblings",false))
            m_OutlineSiblings=true;
        else if (args[0].IsSameAs(L"outlinemargins",false))
            m_OutlineMargins=true;
        else if (args[0].IsSameAs(L"metatemplate",false))
            m_MetaTemplate=args[1];
        else if (args[0].IsSameAs(L"localisationdir",false))
            m_LocalisationDir=args[1];
        else if (args[0].IsSameAs(L"localisationfile",false))
            m_LocalisationFile=args[1];
        else if (args[0].IsSameAs(L"stylesheet",false))
        {
            m_StyleSheet=args[1];
            gotStyle=true;
        }
        else if (args[0].IsSameAs(L"hwswmode",false))
            m_HWSWMode=atoi(args[1].mb_str());
        else if (args[0].IsSameAs(L"size",false))
        {
            if (args.size()<3) continue;

            m_Width=atoi(args[1].mb_str());
            m_Height=atoi(args[2].mb_str());
        }
    }

    Setup(m_RootDir,m_Project,m_Group);
    m_GroupData=LoadGroup(m_RootDir+m_Group);
    m_Palette.Load();
    m_Palette.SetupElements();

    if (!gotStyle && CIwTheHost.m_Shared.m_StyleList.size()>0)
        m_StyleSheet=CIwTheHost.m_Shared.m_StyleList[CIwTheHost.m_Shared.m_StyleList.size()-1].m_StyleSheet;

    CIwTheHost.m_Shared.SetStyleSheet(m_StyleSheet);

    Project.SetOrAdd(L"uiproject",m_RootDir+m_Project,false,false);
    Project.Save(false);
    return true;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::CheckUsage(CIwAttrInstance* inst)
{
    std::map<wxString,CUIEdAttrPropSet>::iterator it;
    CIwAttrClass* klass1=CIwTheFileMetaMgr.GetClass(L"CIwPropertySet");
    CIwAttrClass* klass2=CIwTheFileMetaMgr.GetClass(L"CIwMaterial");

    for (int i=0; i<(int)inst->m_Data.size(); i++)
    {
        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD || (inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CLASS)
        {
            if (inst->m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name.IsSameAs(L"CIwPropertySet") ||
                inst->m_Data[i]->m_Items[0].m_Inst->m_Class->m_Name.IsSameAs(L"CIwUIPropertySet"))
                continue;

            CheckUsage(inst->m_Data[i]->m_Items[0].m_Inst);
        }

        if ((inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR)
        {
            for (int j=0; j<(int)inst->m_Data[i]->m_Items.size(); j++)
            {
                if (inst->m_Data[i]->m_Items[j].m_Inst!=NULL)
                {
                    if (inst->m_Data[i]->m_Member->m_Items[0].m_Class==klass1)
                    {
                        for (it=m_PropSetDict.begin(); it!=m_PropSetDict.end(); ++it)
                        {
                            if (it->second.m_Inst==inst->m_Data[i]->m_Items[j].m_Inst)
                            {
                                it->second.m_Temp++;
                                break;
                            }
                        }
                    }

                    if (inst->m_Data[i]->m_Member->m_Items[0].m_Class==klass2)
                    {
                        for (it=m_MaterialDict.begin(); it!=m_MaterialDict.end(); ++it)
                        {
                            if (it->second.m_Inst==inst->m_Data[i]->m_Items[j].m_Inst)
                            {
                                it->second.m_Temp++;
                                break;
                            }
                        }
                    }

                    CheckUsage(inst->m_Data[i]->m_Items[j].m_Inst);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdProjectGroup::CheckUsage()
{
    for (int i=0; i<(int)m_Lines.size(); i++)
    {
        if (m_Lines[i]->m_Group!=NULL)
            m_Lines[i]->m_Group->CheckUsage();

        if (m_Lines[i]->m_UI!=NULL)
            CIwTheHost.m_Shared.CheckUsage(m_Lines[i]->m_UI->m_Group.m_Inst);
    }
}

//------------------------------------------------------------------------------
bool CUIEdProjectGroup::HasFile(const wxString& line)
{
    wxString line2=MakeFileName(line);

    for (int i=0; i<(int)m_Lines.size(); i++)
    {
        if (line2.IsSameAs(m_Lines[i]->m_FileName,false))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
wxString CUIEdProjectGroup::MakeFileName(const wxString& line)
{
    wxFileName name(m_FileName);
    wxString res;
    if (line[0]=='/' || line[0]=='\\')
        res=m_Parent->m_RootDir+line.Mid(1);
    else if (line[0]=='.')
        res=name.GetPath()+line.Mid(1);
    else
        res=m_Parent->m_RootDir+line;

    res.Replace(L"\\",L"/");

    return res;
}

//------------------------------------------------------------------------------
bool CUIEdProjectGroup::AddLine(const wxString& str,bool atEnd)
{
    wxFileName name(m_FileName);

    CUIEdProjectGroupLine* line=new CUIEdProjectGroupLine;
    line->m_Parent=this;
    if (atEnd || m_Insert>=(int)m_Lines.size())
        m_Lines.push_back(line);
    else
    {
        m_Lines.insert(m_Lines.begin()+m_Insert,line);
        m_Insert++;
    }

    int mode=0;
    for (int i=0; i<(int)str.size(); i++)
    {
        switch (mode)
        {
        case 0:
            if (str[i]==' ' || str[i]=='\t')
                line->m_Prefix+=str[i];
            else if (str[i]=='\"')
            {
                mode=2;
                line->m_Quote=L"\"";
            }
            else
            {
                line->m_Line+=str[i];
                mode=1;
            }

            break;
        case 1:
            if (str[i]==' ' || str[i]=='\t')
            {
                line->m_Extra+=str[i];
                mode=4;
            }
            else
                line->m_Line+=str[i];

            break;
        case 2:
            if (str[i]=='\\')
                mode=3;
            else if (str[i]=='\"')
                mode=4;
            else
                line->m_Line+=str[i];

            break;
        case 3:
            mode=2;
            line->m_Line+=str[i];
            break;
        case 4:
            line->m_Extra+=str[i];
            break;
        }
    }

    if (line->m_Line[0]=='#' || (line->m_Line[0]=='/' && line->m_Line[1]=='/'))
    {
        line->m_Extra=line->m_Line+line->m_Extra;
        line->m_Line=L"";
    }

    wxString extra=line->m_Extra;
    extra.Trim(false);

    if ((extra.empty() || extra[0]=='/' || extra[0]=='#' || extra[0]=='{') && line->m_Line.Contains(L"."))
    {
        line->m_FileName=MakeFileName(line->m_Line);

        wxFileName name2(line->m_FileName);
        line->m_Name=name2.GetFullName();

        if (atEnd)
            m_Insert=m_Lines.size();
    }
    else if (line->m_Line==L"}")
        m_Insert=m_Lines.size()-1;

    if (line->m_Line==L"{" || line->m_Line.StartsWith(L"name"))
        m_First=m_Lines.size();

    if (line->m_Line.EndsWith(L".group"))
    {
        line->m_Group=m_Parent->LoadGroup(line->m_FileName);
        if (line->m_Group!=NULL)
            line->m_Group->m_ParentLine=line;
        else
            wxMessageBox(L"Warning: Project data directory may not be set correctly (should be <project name>/data)",L"Loading project");
    }

    if (line->m_Line.EndsWith(L".ui"))
        CIwTheHost.m_Shared.LoadUI(line,false);

    if (line->m_Line.EndsWith(L".itx") || line->m_Line.EndsWith(L".mtl"))
    {
        CIwTheHost.m_Shared.LoadUI(line,false);
        if (line->m_UI!=NULL)
            m_Parent->SetFile(line->m_UI);
    }

    if (line->m_Line.EndsWith(L".tga") || line->m_Line.EndsWith(L".png") || line->m_Line.EndsWith(L".bmp") || line->m_Line.EndsWith(L".gif"))
    {
        wxImage img(line->m_FileName);
        if (img.IsOk())
        {
            img.Rescale(20,20,wxIMAGE_QUALITY_HIGH);
            line->m_Bmp=wxBitmap(img);
        }
    }

    if (line->m_Line.EndsWith(L".gxfont"))
    {
        wxFileName name(line->m_FileName);
        name.SetExt(L"tga");
        wxImage img(name.GetFullPath());
        if (img.IsOk())
        {
            for (int i=0; i<img.GetHeight(); i++)
            {
                for (int j=0; j<img.GetWidth(); j++)
                {
                    img.SetRGB(j,i,img.GetAlpha(j,i),img.GetAlpha(j,i),img.GetAlpha(j,i));
                }
            }

            img.Resize(wxSize(40,20),wxPoint(0,0),0,0,0);
            img=img.GetSubImage(wxRect(20,0,20,20));
            line->m_Bmp=wxBitmap(img);
        }
    }

    SetChanged(true);
    return true;
}
//------------------------------------------------------------------------------
bool CUIEdProjectGroup::AddLineStart(const wxString& str,int lineNum)
{
    wxFileName name(m_FileName);

    if (lineNum==-1)
        lineNum=m_First;

    CUIEdProjectGroupLine* line=new CUIEdProjectGroupLine;
    line->m_Parent=this;
    m_Lines.insert(m_Lines.begin()+lineNum,line);

    int mode=0;
    for (int i=0; i<(int)str.size(); i++)
    {
        switch (mode)
        {
        case 0:
            if (str[i]==' ' || str[i]=='\t')
                line->m_Prefix+=str[i];
            else if (str[i]=='\"')
            {
                mode=2;
                line->m_Quote=L"\"";
            }
            else
            {
                line->m_Line+=str[i];
                mode=1;
            }

            break;
        case 1:
            if (str[i]==' ' || str[i]=='\t')
            {
                line->m_Extra+=str[i];
                mode=4;
            }
            else
                line->m_Line+=str[i];

            break;
        case 2:
            if (str[i]=='\\')
                mode=3;
            else if (str[i]=='\"')
                mode=4;
            else
                line->m_Line+=str[i];

            break;
        case 3:
            mode=2;
            line->m_Line+=str[i];
            break;
        case 4:
            line->m_Extra+=str[i];
            break;
        }
    }

    if (line->m_Line[0]=='#' || (line->m_Line[0]=='/' && line->m_Line[1]=='/'))
    {
        line->m_Extra=line->m_Line+line->m_Extra;
        line->m_Line=L"";
    }

    wxString extra=line->m_Extra;
    extra.Trim(false);

    if ((extra.empty() || extra[0]=='/' || extra[0]=='#' || extra[0]=='{') && line->m_Line.Contains(L"."))
    {
        line->m_FileName=MakeFileName(line->m_Line);

        wxFileName name2(line->m_FileName);
        line->m_Name=name2.GetFullName();
    }

    if (line->m_Line.EndsWith(L".group"))
    {
        line->m_Group=m_Parent->LoadGroup(line->m_FileName);
        line->m_Group->m_ParentLine=line;
    }

    if (line->m_Line.EndsWith(L".ui"))
        CIwTheHost.m_Shared.LoadUI(line,false);

    if (line->m_Line.EndsWith(L".itx") || line->m_Line.EndsWith(L".mtl"))
    {
        CIwTheHost.m_Shared.LoadUI(line,false);
        if (line->m_UI!=NULL)
            m_Parent->SetFile(line->m_UI);
    }

    SetChanged(true);
    return true;
}

//------------------------------------------------------------------------------
void CUIEdProject::SetFile(CUIEdProjectUI* file)
{
    if (file->m_FileName.EndsWith(L".itx"))
    {
        if (m_PropSet.empty())
            m_PropSetData=file;
        else
        {
            wxString from=m_RootDir+m_PropSet;
            wxString to=file->m_FileName;
            from.Replace(L"\\",L"/");
            to.Replace(L"\\",L"/");
            if (from.IsSameAs(to,false))
                m_PropSetData=file;
        }
    }

    if (file->m_FileName.EndsWith(L".mtl"))
    {
        if (m_Material.empty())
            m_MaterialData=file;
        else
        {
            wxString from=m_RootDir+m_Material;
            wxString to=file->m_FileName;
            from.Replace(L"\\",L"/");
            to.Replace(L"\\",L"/");
            if (from.IsSameAs(to,false))
                m_MaterialData=file;
        }
    }
}

//------------------------------------------------------------------------------
CUIEdProjectGroup* CUIEdProject::LoadGroup(const wxString& file)
{
    wxFileName name(file);
    wxTextFile fp(file);
    if (!fp.Exists() || !fp.Open())
        return NULL;

    CUIEdProjectGroup* group=new CUIEdProjectGroup(file,this);

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        group->AddLine(fp[i],true);
    }

    group->SetChanged(false);
    return group;
}

//------------------------------------------------------------------------------
void CUIEdProject::Setup(const wxString& rootDir,const wxString& project,const wxString& group)
{
    m_RootDir=rootDir;
    m_Project=project;
    m_Group=group;

    wxFileName name(project);
    CIwTheApp->m_DirList[L"projectdetails"]=L"Project: "+name.GetName();
    CIwTheFrame->SetMainTitle();

    if (m_GroupData!=NULL)
        delete m_GroupData;

    m_GroupData=NULL;
    m_PropSetData=NULL;
    m_MaterialData=NULL;

    if (m_Palette.m_Group.m_Inst!=NULL)
        delete m_Palette.m_Group.m_Inst;

    m_Palette.m_Group.m_Inst=NULL;

    if (!wxDirExists(m_RootDir+L"/_viewertemp"))
        wxMkdir(m_RootDir+L"/_viewertemp");

    CIwTheFileMetaMgr.m_BaseDir=m_RootDir;

    wxString Data;
    if (!m_MetaTemplate.empty())
        Data=CIwTheApp->MakeAbsoluteFilename(m_RootDir+m_MetaTemplate);

    if (wxFileName::FileExists(Data))
    {
        CIwTheFileMetaMgr.Load(Data,1);
        CIwTheFileMetaMgr.Load(L"",2);
    }
    else
    {
        // load the project specific settings from uimetatemplate.txt
        Data=m_RootDir+METAUI_FILENAME;
        if (!wxFileName::FileExists(Data))
        {
            // create default user version of file
            CIwTextFile uimetadata(Data);
            uimetadata.Create();
            uimetadata.AddLine(L"// Detail custom classes below");
            uimetadata.Write();
        }

        CIwTheFileMetaMgr.Load(Data,2);
    }

    // write out mapping for classes viewer doesn't understand
    WriteCustomClassMap(2, m_RootDir+L"/_viewertemp/classmap.txt");

    if (m_HWSWMode==-1)
    {
        wxString Data2;
        m_HWSWMode=2;
        if (Project.Get(L"hwswmode",Data2))
            m_HWSWMode=atoi(Data2.mb_str());

        SetChanged(true);
    }
}
//------------------------------------------------------------------------------
void CUIEdProject::WriteCustomClassMap(int Slot, const wxString& fileName)
{
    std::vector<CIwAttrDescMgr::ClassBaseStruct> mapping;

    CIwTheFileMetaMgr.FindClassBases(Slot, mapping);

    CIwTextFile mf(fileName);
    mf.Create();

    for (int i=0; i<(int)mapping.size(); ++i)
    {
        CIwAttrDescMgr::ClassBaseStruct& cb = mapping[i];

        if (cb.m_Class && cb.m_Base)
            mf.AddLine(wxString::Format(L"%s %s",
                                        cb.m_Class->m_Name, cb.m_Base->m_Name));
    }

    mf.Write();
}
//------------------------------------------------------------------------------
void CUIEdProject::CreateGroup(const wxString& file,wxString styleGroup)
{
    wxFileName name(file);

    styleGroup.Replace(L"\\",L"/");

    wxTextFile fp2(m_RootDir+file);
    fp2.AddLine(L"// IwUI Group file created by the tool");
    fp2.AddLine(L"");
    fp2.AddLine(L"CIwResGroup");
    fp2.AddLine(L"{");
    fp2.AddLine(wxString::Format(L"\tname \"%s\"",name.GetName().c_str()));
    if (!styleGroup.empty())
    {
        fp2.AddLine(L"\t");
        fp2.AddLine(wxString::Format(L"\t\"%s\"",styleGroup.c_str()));
    }

    fp2.AddLine(L"");
    fp2.AddLine(L"}");
    fp2.Write();
}

//------------------------------------------------------------------------------
void CUIEdProject::CreateUI(const wxString& file)
{
    wxFileName name(file);
    wxTextFile fp2(m_RootDir+file);

    fp2.AddLine(L"// IwUI file created by the tool");
    fp2.AddLine(L"");
    fp2.AddLine(L"CIwUIElement");
    fp2.AddLine(L"{");
    fp2.AddLine(wxString::Format(L"\tname \"%s\"",name.GetName().c_str()));
    fp2.AddLine(L"}");
    fp2.Write();
}

//------------------------------------------------------------------------------
void CUIEdProject::CreateStyleSheet(const wxString& file,const wxString& parent)
{
    wxFileName name(file);

    wxTextFile fp2(m_RootDir+file);
    fp2.AddLine(L"// IwUI file created by the tool");
    fp2.AddLine(L"");
    fp2.AddLine(L"CIwUIStylesheet");
    fp2.AddLine(L"{");
    fp2.AddLine(wxString::Format(L"\tname \"%s\"",name.GetName().c_str()));
    if (!parent.empty())
        fp2.AddLine(wxString::Format(L"\tparent \"%s\"",parent.c_str()));

    fp2.AddLine(L"}");

    fp2.Write();
}
//------------------------------------------------------------------------------
void CUIEdProject::SyncFlags(bool setChanged)
{
    if (setChanged)
        SetChanged(true);

    if (CIwTheHost.m_Link)
    {
        int val=0;
        val|=GetOutlineElements() ? 1 : 0;
        val|=GetOutlineLayouts() ? 2 : 0;
        val|=GetOutlineSiblings() ? 4 : 0;
        val|=GetOutlineHierarchy() ? 8 : 0;
        val|=GetOutlineMargins() ? 16 : 0;
        char text[256];
        sprintf(text,"%d",val);
        CIwTheHost.m_Link->SetData(CIwViewerUI::OUTLINE_FLAGS,text);
    }
}
//------------------------------------------------------------------------------
void CUIEdProject::SetLocalisationFile(const wxString& file)
{
    SetChanged(true);

    m_LocalisationFile = file;

    SyncLocalisation();
}
//------------------------------------------------------------------------------
void CUIEdProject::SyncStyleSheet()
{
    if (CIwTheHost.m_Link)
        CIwTheHost.m_Link->SetData(CIwViewerUI::SET_STYLESHEET,
                                   m_StyleSheet.mb_str());
}
//------------------------------------------------------------------------------
void CUIEdProject::SyncLocalisation()
{
    if (CIwTheHost.m_Link)
        CIwTheHost.m_Link->SetData(CIwViewerUI::LOCALISATION_FILE,
                                   m_LocalisationFile.mb_str());
}
//------------------------------------------------------------------------------
class Traverser2 : public wxDirTraverser
{
public:
    wxArrayString& list;
    wxArrayString& dirList;
    wxString root;

    Traverser2(wxArrayString& _list,wxArrayString& _dirList,const wxString& _root) : list(_list),dirList(_dirList),root(_root) {}
    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        list.Add(filename.Mid(root.size()));
        return wxDIR_CONTINUE;
    }
    virtual wxDirTraverseResult OnDir(const wxString& dirname)
    {
        dirList.Add(dirname.Mid(root.size()));
        return wxDIR_CONTINUE;
    }
};

//------------------------------------------------------------------------------
void CUIEdProject::Create(bool fromGroup,const wxString& styleDir)
{
    wxString styleGroup;

    if (!styleDir.empty())
    {
        int i;
        wxArrayString files;
        wxArrayString dirs;
        wxFileName name(styleDir);
        wxString to=m_RootDir+name.GetName();

        Traverser2 traverser(files,dirs,styleDir);
        wxDir dir(styleDir);
        dir.Traverse(traverser);

        if (!wxDirExists(to))
            wxMkdir(to);

        for (i=0; i<(int)dirs.size(); i++)
        {
            if (!wxDirExists(to+dirs[i]))
                wxMkdir(to+dirs[i]);
        }


        for (i=0; i<(int)files.size(); i++)
        {
            //files[i].LowerCase();

            if (wxFileExists(to+files[i]))
            {
#ifdef I3D_OS_WINDOWS
                SetFileAttributesW((to+files[i]).c_str(),FILE_ATTRIBUTE_NORMAL);
#else
                chmod((to+files[i]).mb_str(),S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif
                wxRemoveFile((to+files[i]).c_str());
            }

            wxCopyFile(styleDir+files[i],to+files[i]);
#ifdef I3D_OS_WINDOWS
            SetFileAttributesW((to+files[i]).c_str(),FILE_ATTRIBUTE_NORMAL);
#else
            chmod((to+files[i]).mb_str(),S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif

            if (files[i].EndsWith(L"style.group"))
                styleGroup=name.GetName()+files[i];

            if (files[i].EndsWith(L"expmetapalette.ui"))
                m_Palette.m_FileName=to+files[i];

            if (files[i].EndsWith(L"resource_templates.itx"))
                m_ResTemplate=(to+files[i]).Mid(m_RootDir.size());
        }
    }

    if (m_Palette.m_FileName.empty())
        m_Palette.m_FileName=m_RootDir+L"ExpMetaPalette.ui";

    Project.SetOrAdd(L"uiproject",m_RootDir+m_Project,false,false);
    Project.Save(false);

    if (!fromGroup)
        CreateGroup(m_Group,styleGroup);

    m_GroupData=LoadGroup(m_RootDir+m_Group);
    m_Palette.Load();
    m_Palette.SetupElements();

    if (CIwTheHost.m_Shared.m_StyleList.empty())
        CIwTheHost.m_Shared.SetStyleSheet(L"");
    else
        CIwTheHost.m_Shared.SetStyleSheet(CIwTheHost.m_Shared.m_StyleList[CIwTheHost.m_Shared.m_StyleList.size()-1].m_StyleSheet);

    CheckSave(true,false);
}

//------------------------------------------------------------------------------
void CUIEdProject::CheckSave(bool force,bool saveData)
{
    if (m_GroupData!=NULL && saveData)
    {
        m_GroupData->CheckSave(force);

        int num=0;
        m_GroupData->SaveFiltered(CIwTheHost.m_AltGroupFile,num);
    }

    if (!HasChanged() && !force) return;

    wxTextFile fp(m_RootDir+m_Project);
    fp.AddLine(L"// IwUI Project file");
    fp.AddLine(L"");

    wxFileName file(m_RootDir+m_Project);
    wxFileName dir(m_RootDir);
    dir.MakeRelativeTo(file.GetPath());

    fp.AddLine(wxString::Format(L"dir \"%s \"",dir.GetFullPath().c_str()));
    fp.AddLine(wxString::Format(L"group \"%s\"",m_Group.c_str()));
    fp.AddLine(wxString::Format(L"palette \"%s\"",m_Palette.m_FileName.Mid(m_RootDir.size()).c_str()));
    if (m_MaterialData!=NULL)
        fp.AddLine(wxString::Format(L"material \"%s\"",m_MaterialData->m_FileName.Mid(m_RootDir.size()).c_str()));

    if (m_PropSetData!=NULL)
        fp.AddLine(wxString::Format(L"propset \"%s\"",m_PropSetData->m_FileName.Mid(m_RootDir.size()).c_str()));

    if (!m_ResTemplate.empty())
        fp.AddLine(wxString::Format(L"restemplate \"%s\"",m_ResTemplate.c_str()));

    fp.AddLine(wxString::Format(L"stylesheet \"%s\"",m_StyleSheet.c_str()));
    if (m_HWSWMode!=-1)
        fp.AddLine(wxString::Format(L"hwswmode %d",m_HWSWMode));

    fp.AddLine(wxString::Format(L"size %d %d",m_Width,m_Height));
    if (!m_AutoStartViewer)
        fp.AddLine(L"autoviewer false");

    if (m_OutlineHierarchy)
        fp.AddLine(L"outlinehierarchy true");

    if (m_OutlineSiblings)
        fp.AddLine(L"outlinesiblings true");

    if (m_OutlineMargins)
        fp.AddLine(L"outlinemargins true");

    if (!m_MetaTemplate.empty())
        fp.AddLine(wxString::Format(L"metatemplate \"%s\"",m_MetaTemplate.c_str()));

    if (!m_LocalisationDir.empty())
        fp.AddLine(wxString::Format(L"localisationdir \"%s\"",m_LocalisationDir.c_str()));

    if (!m_LocalisationFile.empty())
        fp.AddLine(wxString::Format(L"localisationfile \"%s\"",m_LocalisationFile.c_str()));

    fp.Write();

    SetChanged(false);
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::ResetUI()
{
    if (m_CurrUI==NULL)
        return;

    RemovePropSets(m_CurrUI);

    m_CurrUI->Load();
    m_CurrUI->SetupElements();

    AddPropSets(m_CurrUI);

    RefreshUI(true);
    SetSelection(NULL,SELSOURCE_START);
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::Refresh()
{
    if (m_ProjectFrame!=NULL)
        m_ProjectFrame->Reset();

    if (m_PaletteFrame!=NULL)
        m_PaletteFrame->Reset();

    if (m_PaletteFrame2!=NULL)
        m_PaletteFrame2->Reset();

    if (m_MediaFrame!=NULL)
        m_MediaFrame->Reset();
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::StartViewer()
{
    if (m_CurrUI==NULL)
    {
        wxMessageBox(L"Please Load a UI file first",L"Reset Viewer");
        return;
    }

    m_ViewerFrame->LoadVia();
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::RefreshUI(bool resetViewer)
{
    if (resetViewer && m_Project.m_AutoStartViewer && m_ViewerFrame!=NULL)
        m_ViewerFrame->LoadVia();

    if (m_UIEdAttrStylePanel!=NULL)
        m_UIEdAttrStylePanel->Reset();

    if (m_UIEdAttrMaterialPanel!=NULL)
        m_UIEdAttrMaterialPanel->Reset();
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::RefreshCurr()
{
    m_UIEdAttrUIPanel->Reset();
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::NewProject(bool fromGroup)
{
    if (!m_ProjectFrame->MakeProject(fromGroup))
        return;

    Refresh();
    RefreshUI(false);
    SetSelection(NULL);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::LoadProject()
{
    wxFileDialog dlg(CIwTheFrame,L"Load Project File...",m_Project.m_Group,m_Project.m_Project,
                     L"IwUI Project File (*.uip)|*.uip|All Files (*.*)|*.*",wxOPEN|wxFILE_MUST_EXIST);
    if (dlg.ShowModal()!=wxID_OK) return;

    Clear();
    m_Project.Load(dlg.GetPath());

    Refresh();
    RefreshUI(false);
    SetSelection(NULL);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::Clear()
{
    m_StyleList.clear();
    m_PropSetDict.clear();
    m_MaterialDict.clear();

    Reset();
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::Reset()
{
    m_CurrUI=NULL;
    m_DragInst=NULL;
    m_SelElem=NULL;
    m_SelList.clear();
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::SaveProject()
{
    m_Project.CheckSave(false);
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::MakeUI(CUIEdProjectGroup* group)
{
    if (group==NULL)
        group=m_Project.m_GroupData;

    if (group==NULL)
        return;

    wxFileDialog dlg(CIwTheFrame,L"Make UI File...",m_Project.m_RootDir,L"*.ui",
                     L"Iw UI File (*.ui)|*.ui|All Files (*.*)|*.*",wxSAVE|wxOVERWRITE_PROMPT);
    if (dlg.ShowModal()!=wxID_OK) return;

    wxString mainDir=m_Project.m_RootDir;
    wxString file=dlg.GetPath();

    if (!CIwTheHost.StartsWith(file,mainDir))
        wxMessageBox(L"File must be under the Root Directory",L"Make UI File");
    else
    {
        wxString line=dlg.GetPath().Mid(mainDir.size());
        line.Replace(L"\\",L"/");

        if (!group->HasFile(line))
        {
            m_Project.CreateUI(line);
            group->AddLine(L"\t\""+line+L"\"");
        }
        else
            wxMessageBox(L"File already exists",L"Make UI File");
    }

    if (m_ProjectFrame!=NULL)
        m_ProjectFrame->Reset();
}

//------------------------------------------------------------------------------
bool CUIEdAttrShared::Select(wxTreeCtrl* tree,wxTreeItemId parent,CIwAttrInstance* inst)
{
    if (!parent.IsOk()) return false;

    CIwAttrTreeItem *Data=(CIwAttrTreeItem*)tree->GetItemData(parent);
    if (Data!=NULL && Data->m_Inst==inst)
    {
        tree->SelectItem(parent);
        for (wxTreeItemId id=parent; id!=tree->GetRootItem(); id=tree->GetItemParent(id))
        {
            tree->Expand(id);
        }
        return true;
    }

    wxTreeItemIdValue cookie;
    wxTreeItemId id=tree->GetFirstChild(parent,cookie);

    while (id.IsOk())
    {
        if (Select(tree,id,inst))
            return true;

        id=tree->GetNextChild(id,cookie);
    }
    return false;
}
//------------------------------------------------------------------------------
void CUIEdAttrShared::UpdateFromViewer2(std::vector<CIwAttrNote>& notesnew,std::vector<CIwAttrNote>& notesold,int oldnum,int newnum)
{
    int i;
    std::map<wxString,int> count;

    for (i=0; i<(int)notesnew.size(); i++)
    {
        if (notesnew[i].m_ID!=newnum)
            continue;

        if (notesnew[i].m_Data[0]=='{')
        {
        }
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::UpdateFromViewer2()
{
    CIwAttrInstance* inst;
    CUIEdProjectUI* temp=new CUIEdProjectUI;
    temp->m_FileName=m_Project.m_RootDir+m_UpdateFile;
    temp->Load();
    inst=temp->m_Group.m_Inst->FindChild(m_UpdateName);

    if (inst!=NULL)
        Remove(inst);

    delete temp;

    if (m_CurrUI==NULL) return;

    CIwAttrInstance* old=m_CurrUI->m_Group.m_Inst->FindChild(m_UpdateName);

    if (inst==NULL && old!=NULL)
        m_CurrUI->Delete(old);
    else if (old==NULL)
        m_CurrUI->Add2(inst,m_CurrUI->m_Group.m_Inst);
    else
    {
        m_CurrUI->Delete(old);
        m_CurrUI->Add2(inst,m_CurrUI->m_Group.m_Inst);
    }

    if (inst!=NULL)
        m_CurrUI->SetupElements(inst,&m_CurrUI->m_BaseElement);

    m_Selecting=true;
    bool first=true;
    int i;
    m_SelList.clear();
    for (i=0; i<(int)m_UpdateSel.size(); i++)
    {
        inst=GetFromFullName(m_UpdateSel[i]);
        if (inst==NULL) continue;

        m_SelList.push_back(inst);
    }
    m_UpdateSel.clear();

    m_Selecting=false;

    if (m_SelList.size()==1)
        SetSelection(m_SelList[0],SELSOURCE_VUPDATE);
    else
        SetSelection(NULL,SELSOURCE_VUPDATE);

    for (i=0; i<(int)m_SelList.size(); i++)
    {
        if (m_UIEdAttrUIPanel!=NULL)
            m_UIEdAttrUIPanel->Select(m_UIEdAttrUIPanel->m_Tree->GetRootItem(),m_SelList[i],first);

        first=false;
    }
    if (m_UIEdAttrUIPanel!=NULL && first)
        m_UIEdAttrUIPanel->Select(m_UIEdAttrUIPanel->m_Tree->GetRootItem(),NULL,first);

    m_CurrUI->SetChanged(true);
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::Idle()
{
    if (scheduleSource!=0)
    {
        switch (scheduleSource&SELSOURCE_MASK)
        {
        case SELSOURCE_DATA:
        case SELSOURCE_CHANGE:
        case SELSOURCE_UIED:
        case SELSOURCE_LAYOUT:
        case SELSOURCE_START:
        case SELSOURCE_VIEWER:
        case SELSOURCE_VUPDATE:
            SetSelection(scheduleInst,scheduleSource&~SELSOURCE_SCHEDULE);
            break;

        case CHANGETYPE_ADD:
        case CHANGETYPE_GROUP:
        case CHANGETYPE_SETLAYOUT:
        case CHANGETYPE_SIZERPOLICY:
        case CHANGETYPE_MOVE:
        case CHANGETYPE_REMOVE:
        case CHANGETYPE_BIG:
            Change(scheduleInst,scheduleSource&~SELSOURCE_SCHEDULE,scheduleInstParent);
            break;
        case CHANGETYPE_STYLE:
        case CHANGETYPE_MAT:
            Signal(scheduleSource&~SELSOURCE_SCHEDULE);
            break;
        }
        scheduleSource=0;
    }

    if (!m_UpdateName.empty())
    {
        wxWindowUpdateLocker noUpdates(m_UIEdAttrUIPanel);

        UpdateFromViewer2();
        m_UpdateName.clear();
    }
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::GetFirst()
{
    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwUIElement");

    if (m_CurrUI==NULL || m_CurrUI->m_Group.m_Inst==NULL) return NULL;

    for (int i=0; i<(int)m_CurrUI->m_Group.m_Inst->m_Data.size(); i++)
    {
        if ((m_CurrUI->m_Group.m_Inst->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        CIwAttrInstance* inst2=m_CurrUI->m_Group.m_Inst->m_Data[i]->m_Items[0].m_Inst;
        if (inst2==NULL)
            continue;

        CIwAttrClass* klass2;

        for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
        {
            if (klass2==klass)
                break;
        }
        if (klass2==NULL)
            continue;

        return inst2;
    }
    return NULL;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SetCurrent(CIwAttrInstance* inst,bool reset,bool fromViewer)
{
    if (inst==NULL)
        inst=GetFirst();

    m_SelElem=inst;

    pending=reset ? 2 : 1;
    m_FromViewer=fromViewer;
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::GetSelectedElem(const wxString& name2,bool checkCurr,CIwAttrInstance* base)
{
    if (base==NULL)
    {
        base=m_SelElem;
        if (base!=NULL && base->m_Parent!=NULL && base->m_Parent->m_Instance!=NULL)
            base=base->m_Parent->m_Instance;
    }
    else
    {
        while (base!=NULL && base->m_Parent!=NULL)
        {
            if (base->m_Parent->m_Member->m_Name.IsSameAs(L"CIwUIElement",false))
                break;

            base=base->m_Parent->m_Instance;
        }
    }

    if (base==NULL) return NULL;

    CIwAttrClass* klass=CIwTheFileMetaMgr.GetClass(L"CIwUILayout");
    for (int i=0; i<(int)base->m_Data.size(); i++)
    {
        if ((base->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_CHILD)
            continue;

        CIwAttrInstance* inst2=base->m_Data[i]->m_Items[0].m_Inst;
        if (inst2==NULL)
            continue;

        CIwAttrClass* klass2;

        for (klass2=inst2->m_Class; klass2!=NULL; klass2=klass2->m_Parent)
        {
            if (klass2==klass)
                break;
        }
        if (klass2!=NULL)
            continue;

        CIwAttrData* name1=inst2->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
        if (name1!=NULL && name1->m_Items[0].m_String.IsSameAs(name2,false))
            return inst2;
    }

    if (checkCurr)
    {
        CIwAttrData* name1=base->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
        if (name1!=NULL && name1->m_Items[0].m_String.IsSameAs(name2,false))
            return base;
    }

    return NULL;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SetSelected(CIwAttrInstance* inst,bool reset,bool fromViewer)
{
    if (inst==NULL)
        inst=m_SelElem;

    m_FromViewer=fromViewer;
    pending2=reset ? 4 : 3;
    wxWakeUpIdle();
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::SetSelection(CIwAttrInstance* inst,int source)
{
    if (source&SELSOURCE_SCHEDULE)
    {
        scheduleSource=source;
        scheduleInst=inst;
        wxWakeUpIdle();
        return;
    }

    source&=SELSOURCE_MASK;

    bool old=m_Selecting;
    m_Selecting=true;

    if (m_CurrUI!=NULL && inst==m_CurrUI->m_Group.m_Inst)
        inst=NULL;

    bool selChanged=false;

    if (source==SELSOURCE_START)
    {
        inst=m_SelElem;
        if (inst==NULL)
            inst=GetFirst();
    }

    selChanged=(m_SelElem!=inst);
    m_SelElem=inst;

    if (m_SelElem!=NULL)
    {
        m_SelList.clear();
        m_SelList.push_back(m_SelElem);
    }

    if (source==SELSOURCE_START || source==SELSOURCE_VUPDATE || source==SELSOURCE_DATA)
        selChanged=true;

    if (m_UIEdAttrUIPanel!=NULL && selChanged)
    {
        if (source!=SELSOURCE_UIED)
            m_UIEdAttrUIPanel->Reset();

        if (source!=SELSOURCE_UIED)
            m_UIEdAttrUIPanel->Select(m_UIEdAttrUIPanel->m_Tree->GetRootItem(),m_SelElem);

        m_UIEdAttrUIPanel->SetCurrent(m_SelElem);
    }

    if (selChanged && source!=SELSOURCE_CHANGE && source!=SELSOURCE_VIEWER && source!=SELSOURCE_VUPDATE)
        SendChanges(inst);

    m_Selecting=old;
}

//------------------------------------------------------------------------------
CIwAttrInstance* CUIEdAttrShared::Remove(CIwAttrInstance* inst)
{
    int i;
    CIwAttrInstance* parent=NULL;
    if (inst->m_Parent!=NULL)
    {
        CIwAttrData* data=inst->m_Parent;
        if (data->m_Instance!=NULL)
        {
            parent=data->m_Instance;

            for (i=0; i<(int)data->m_Instance->m_Data.size(); i++)
            {
                if (data->m_Instance->m_Data[i]==data)
                {
                    data->m_Instance->m_Data.erase(data->m_Instance->m_Data.begin()+i);
                    break;
                }
            }
        }

        if (data->m_Member->m_Type&&ATTRMEMBER_LIST)
        {
            for (i=0; i<(int)data->m_Items.size(); i++)
            {
                if (data->m_Items[i].m_Inst==inst)
                {
                    data->m_Items.erase(data->m_Items.begin()+i);
                    break;
                }
            }
        }
        else
        {
            data->m_Items[0].m_Inst=NULL;
            data->m_Items[0].m_Flags=0;
            delete data;
        }
    }

    return parent;
}

//-----------------------------------------------------------------------------
void CUIEdAttrShared::TrimmedSelList(std::vector<CIwAttrInstance*>& list)
{
    int i,j;
    list.clear();

    for (i=0; i<(int)m_SelList.size(); i++)
    {
        for (j=0; j<(int)m_SelList.size(); j++)
        {
            if (i==j) continue;

            CIwAttrInstance* elem1=m_SelList[i];
            bool found=false;

            for (; elem1!=NULL && elem1!=m_CurrUI->m_Group.m_Inst && elem1->m_Parent!=NULL; elem1=elem1->m_Parent->m_Instance)
            {
                if (elem1==m_SelList[j])
                {
                    found=true;
                    break;
                }
            }

            if (found) break;
        }
        if (j==(int)m_SelList.size())
            list.push_back(m_SelList[i]);
    }
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::Change(CIwAttrInstance* inst,int type,CIwAttrInstance* parent)
{
    if (type&SELSOURCE_SCHEDULE)
    {
        scheduleSource=type;
        scheduleInst=inst;
        scheduleInstParent=parent;
        wxWakeUpIdle();
        return;
    }

    bool needSetupElements=false;
    CIwAttrInstance* inst1=NULL;
    CIwAttrInstance* inst2=NULL;

    int i;
    std::vector<CIwAttrInstance*> list;
    TrimmedSelList(list);

    for (i=0; i<(int)list.size(); i++)
    {
        if (list[i]==inst)
            break;
    }
    if (i==(int)list.size())
    {
        list.clear();
        list.push_back(inst);
    }

    if (m_CurrUI==NULL) return;

    int actionModifier=0;
    if ((type&SELSOURCE_INSERT)==SELSOURCE_INSERT)
        actionModifier=ACTIONMODIFIER_INSERT;

    m_Selecting=true;
    switch (type&SELSOURCE_MASK)
    {
    case CHANGETYPE_ADD:
        for (i=0; i<(int)list.size(); )
        {
            if (m_CurrUI->IsParent(list[i],parent))
            {
                if (list[i]==inst)
                    inst1=m_CurrUI->Add(list[i],parent,actionModifier);
                else
                    m_CurrUI->Add(list[i],parent,actionModifier);

                list.erase(list.begin()+i);
            }
            else
                i++;
        }
        for (i=0; i<(int)list.size(); i++)
        {
            if (list[i]==inst)
                inst1=m_CurrUI->Add(list[i],parent,actionModifier);
            else
                m_CurrUI->Add(list[i],parent,actionModifier);
        }
        inst=inst1;
        if (inst==NULL) { m_Selecting=false; return; }

        break;
    case CHANGETYPE_GROUP:
        list=m_SelList;
        inst=m_CurrUI->Add(inst,list[0],ACTIONMODIFIER_INSERT|ACTIONMODIFIER_TAKEALL);
        if (inst==NULL) { m_Selecting=false; return; }

        for (i=0; i<(int)list.size(); i++)
        {
            m_CurrUI->Move(list[i],inst);
        }
        parent=NULL;
        break;
    case CHANGETYPE_SETLAYOUT:
        inst=m_CurrUI->SetLayout(inst,parent);
        if (inst==NULL) { m_Selecting=false; return; }

        break;
    case CHANGETYPE_SIZERPOLICY:
        inst=m_CurrUI->SizerPolicy(inst,parent);
        if (inst==NULL) { m_Selecting=false; return; }

        break;
    case CHANGETYPE_MOVE:
        for (i=0; i<(int)list.size(); )
        {
            if (m_CurrUI->IsParent(list[i],parent))
                list.erase(list.begin()+i);
            else
                i++;
        }
        for (i=0; i<(int)list.size(); i++)
        {
            if (list[i]==inst)
                inst1=m_CurrUI->Move(list[i],parent,actionModifier);
            else
                m_CurrUI->Move(list[i],parent,actionModifier);
        }
        inst=inst1;
        if (inst==NULL) { m_Selecting=false; return; }

        parent=NULL;
        break;
    case CHANGETYPE_REMOVE:
        if (inst==m_SelElem)
        {
            if (m_SelElem->m_Parent!=NULL)
                m_SelElem=m_SelElem->m_Parent->m_Instance;
            else
                m_SelElem=NULL;
        }

        if (m_UIEdAttrStylePanel!=NULL && inst==m_UIEdAttrStylePanel->m_Sect)
        {
            m_UIEdAttrStylePanel->Remove(inst);
            m_Selecting=false;
            return;
        }

        if (m_UIEdAttrMaterialPanel!=NULL && inst==m_UIEdAttrMaterialPanel->m_Sect)
        {
            m_UIEdAttrMaterialPanel->Remove(inst);
            m_Selecting=false;
            return;
        }

        for (i=0; i<(int)list.size(); i++)
        {
            m_CurrUI->Delete(list[i]);
        }
        inst=m_SelElem;
        break;
    case CHANGETYPE_BIG:
        m_CurrUI->SetupElements(inst);
        if (m_UIEdAttrUIPanel!=NULL && inst==m_UIEdAttrUIPanel->m_Sect)
        {
            m_UIEdAttrUIPanel->Reset();
            m_UIEdAttrUIPanel->Select(m_UIEdAttrUIPanel->m_Tree->GetRootItem(),inst);

            SetSelection(inst,SELSOURCE_CHANGE);
            m_UIEdAttrUIPanel->SetCurrent(inst);
        }

        if (m_UIEdAttrStylePanel!=NULL && inst==m_UIEdAttrStylePanel->m_Sect)
            m_UIEdAttrStylePanel->ResetProp();

        if (m_UIEdAttrMaterialPanel!=NULL && inst==m_UIEdAttrMaterialPanel->m_Sect)
            m_UIEdAttrMaterialPanel->ResetProp();

        SendChanges(inst);
        m_Selecting=false;
        return;
    }

    SetSelection(inst,SELSOURCE_DATA);

    if (parent!=NULL)
        SendChanges(parent);
    else
        SendChanges(inst);

    m_Selecting=false;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::Signal(int type)
{
    if (type&SELSOURCE_SCHEDULE)
    {
        scheduleSource=type;
        wxWakeUpIdle();
        return;
    }

    type&=SELSOURCE_MASK;

    m_Selecting=true;
    if (type==CHANGETYPE_STYLE && m_UIEdAttrStylePanel!=NULL)
    {
        m_UIEdAttrStylePanel->Reset();
        m_UIEdAttrStylePanel->ResetProp();
    }

    if (type==CHANGETYPE_MAT && m_UIEdAttrMaterialPanel!=NULL)
    {
        m_UIEdAttrMaterialPanel->Reset();
        m_UIEdAttrMaterialPanel->ResetProp();
    }

    m_Selecting=false;
}

//------------------------------------------------------------------------------
void CUIEdAttrShared::ChangeLocalisation()
{
    if (m_Project.m_LocalisationDir.empty())
    {
        wxMessageBox(L"Please add a line 'localisationdir <dir>' in your uip file"
                     L"\n(dir is relative to your project root)",L"Set Localisation");
        return;
    }

    wxString Name=CIwTheApp->MakeAbsoluteFilename(m_Project.m_RootDir+m_Project.m_LocalisationDir);
    wxArrayString strings;
    wxDir::GetAllFiles(Name,&strings,L"*.dat",wxDIR_FILES);

    wxSingleChoiceDialog dlg(CIwTheFrame,L"Choose Localisation File",L"Localisation File",strings);

    dlg.SetSelection(strings.Index(m_Project.m_LocalisationFile));

    if (dlg.ShowModal()!=wxID_OK)
        return;

    m_Project.SetLocalisationFile(dlg.GetStringSelection());
}

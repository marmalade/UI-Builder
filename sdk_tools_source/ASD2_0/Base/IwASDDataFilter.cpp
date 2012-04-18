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
class CIwLayoutDataFilter : public CIwStyleDialog
{
    struct Check
    {
        bool                m_Check;
        int                 m_Offset;
        Check(int offset) : m_Check(true),m_Offset(offset) {}
    };
    enum
    {
        CTRLID_FILES,
        CTRLID_FILTERS,
        CTRLID_SELALL,
        CTRLID_SELNONE,
        CTRLID_SELINV,
        CTRLID_ADDFILTER,
        CTRLID_DELFILTER,
        CTRLID_FILTER=100,
        CTRLID_FILTEREND=1000
    };

    std::vector<CIwASDData*>& m_DataList;
    std::vector<CIwLayoutData::ExpandType>& m_ExpandTypes;

    wxCheckListBox* m_Files;
    wxScrolledWindow* m_FiltersPanel;
    wxSizer* m_FiltersSizer;
    std::vector<wxCheckBox*> m_Filters;
    std::vector<wxCheckBox*> m_Expands;

    std::vector<int> m_FileOffsets;
    std::vector<int> m_ExpandOffsets;
    std::vector<bool> m_ExpandChecks;
    std::vector<wxString> m_ExtraExpands;

    wxTextFile m_fp;
public:
    CIwLayoutDataFilter(wxWindow* parent,std::vector<CIwASDData*>& dataList,std::vector<CIwLayoutData::ExpandType>& expandTypes,
                        const wxString& show,bool all) : CIwStyleDialog(parent,L"Filter Added Files",wxSize(500,400)),m_DataList(dataList),m_ExpandTypes(expandTypes)
    {
        int i,j;
        wxSizer* vert=new wxBoxSizer(wxVERTICAL);
        wxSizer* horiz=new wxBoxSizer(wxHORIZONTAL);
        wxSizer* buttons=new wxBoxSizer(wxVERTICAL);

        if (all)
            SetTitle(L"Filter all Files");

        wxArrayString strings;
        for (i=0; i<(int)dataList.size(); i++)
        {
            if (dataList[i]==NULL) continue;

            wxColour col(L"BLACK");
            std::map<wxString,wxString> opts;
            dataList[i]->GetOptions(opts,col);

            strings.Add(ReplaceOptions(show,opts));
            m_FileOffsets.push_back(i);
        }

        m_Files=new wxCheckListBox(this,CTRLID_FILES,wxPoint(-1,-1),wxSize(-1,-1),strings,wxLB_EXTENDED);
        horiz->Add(m_Files,1,wxEXPAND);

        buttons->Add(new CIwStyleButton(this,CTRLID_SELALL,L"Select All"));
        buttons->Add(new CIwStyleButton(this,CTRLID_SELNONE,L"Select None"));
        buttons->Add(new CIwStyleButton(this,CTRLID_SELINV,L"Invert Sel"));
        horiz->Add(buttons,0,wxEXPAND|wxLEFT,4);

        vert->Add(horiz,1,wxEXPAND|wxALL,4);

        horiz=new wxFlexGridSizer(3);
        horiz->Add(new wxStaticText(this,wxID_ANY,L"Filters:"),0,wxEXPAND);
        horiz->AddStretchSpacer();
        horiz->Add(new wxStaticText(this,wxID_ANY,L"Extra Options:"),0,wxEXPAND);

        m_FiltersPanel=new wxScrolledWindow(this,wxID_ANY,wxPoint(-1,-1),wxSize(100,200),wxHSCROLL|wxVSCROLL|wxSUNKEN_BORDER);
        m_FiltersPanel->SetBackgroundColour(wxColour(L"WHITE"));
        m_FiltersPanel->SetScrollRate(10,10);
        m_FiltersSizer=new wxBoxSizer(wxVERTICAL);
        m_FiltersPanel->SetSizer(m_FiltersSizer);

        horiz->Add(m_FiltersPanel,0,wxEXPAND);

        buttons=new wxBoxSizer(wxVERTICAL);

        buttons->Add(new CIwStyleButton(this,CTRLID_ADDFILTER,L"Add..."));
        buttons->Add(new CIwStyleButton(this,CTRLID_DELFILTER,L"Remove"));
        horiz->Add(buttons,0,wxEXPAND|wxRIGHT,32);

        wxSizer* checks=new wxBoxSizer(wxVERTICAL);

        for (i=0; i<(int)expandTypes.size(); i++)
        {
            if (!expandTypes[i].m_PreExpand)
            {
                bool found=false;
                for (j=0; j<(int)m_Expands.size(); j++)
                {
                    if (expandTypes[i].m_Name.IsSameAs(m_Expands[j]->GetLabel(),false))
                    {
                        found=true;
                        break;
                    }
                }
                if (found) continue;

                wxCheckBox* check=new wxCheckBox(this,wxID_ANY,expandTypes[i].m_Name);
                m_Expands.push_back(check);
                checks->Add(check);
                m_ExpandOffsets.push_back(i);
            }
        }

        horiz->Add(checks,1,wxEXPAND,8);

        vert->Add(horiz,1,wxEXPAND|wxALL,4);

        CIwStyleButtonBar* bar=new CIwStyleButtonBar(this);
        bar->Add(new CIwStyleButton(this,wxID_OK,L"OK"),CIwStyleButtonBar::SPACE_PROP);
        bar->Add(new CIwStyleButton(this,wxID_CANCEL,L"Cancel"));
        vert->Add(bar,0,wxEXPAND);

        SetSizer(vert);

        Setup(all);
    }
    void Setup(bool all)
    {
        int i,j;
        m_fp.Create(CIwTheApp->MakeAbsoluteFilename(L"{viewer}FilterTemp.ast"));

        for (i=0; i<(int)m_Expands.size(); i++)
        {
            m_Expands[i]->SetValue(true);
        }

        if (m_fp.Exists() && m_fp.Open() && m_fp.GetLineCount()>0)
        {
            for (i=0; i<(int)m_fp.GetLineCount(); i++)
            {
                std::vector<wxString> args;

                if (SuperSplit(m_fp[i],args,L" \t\n")<3) continue;

                if (args[0].IsSameAs(L"filter",false))
                {
                    wxCheckBox* f=new wxCheckBox(m_FiltersPanel,CTRLID_FILTER+m_Filters.size(),args[1],wxPoint(-1,-1),wxSize(-1,-1),wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER);
                    m_Filters.push_back(f);
                    m_FiltersSizer->Add(f);

                    if (args[2].IsSameAs(L"true",false))
                        f->Set3StateValue(wxCHK_CHECKED);
                    else if (args[2].IsSameAs(L"false",false))
                        f->Set3StateValue(wxCHK_UNCHECKED);
                    else
                        f->Set3StateValue(wxCHK_UNDETERMINED);
                }
                else if (args[0].IsSameAs(L"expand",false))
                {
                    for (j=0; j<(int)m_Expands.size(); j++)
                    {
                        if (m_Expands[j]->GetLabel().IsSameAs(args[1],false))
                        {
                            m_Expands[j]->SetValue(args[2].IsSameAs(L"true",false));
                            m_ExpandTypes[m_ExpandOffsets[j]].m_Checked=m_Expands[j]->GetValue();
                            break;
                        }
                    }
                    if (j==(int)m_Expands.size())
                    {
                        m_ExtraExpands.push_back(args[1]);
                        m_ExpandChecks.push_back(args[2].IsSameAs(L"true",false));
                    }
                }
            }
            m_fp.Clear();
        }

        for (i=0; i<(int)m_Files->GetCount(); i++)
        {
            m_Files->Check(i,true);
            if (all) continue;

            for (j=0; j<(int)m_Filters.size(); j++)
            {
                if (m_Files->GetString(i).Lower().Matches(m_Filters[j]->GetLabel().Lower()))
                {
                    switch (m_Filters[j]->Get3StateValue())
                    {
                    case wxCHK_CHECKED:
                        m_Files->Check(i,true);     break;
                    case wxCHK_UNCHECKED:
                        m_Files->Check(i,false);    break;
                    case wxCHK_UNDETERMINED:
                        break;
                    }
                }
            }
        }

        ResetFilters();
        m_FiltersSizer->FitInside(m_FiltersPanel);
    }
    void OnOK(wxCommandEvent&)
    {
        EndModal(wxID_OK);
    }
    void OnSelAll(wxCommandEvent&)
    {
        for (int i=0; i<(int)m_Files->GetCount(); i++)
        {
            m_Files->SetSelection(i,true);
        }
    }
    void OnSelNone(wxCommandEvent&)
    {
        for (int i=0; i<(int)m_Files->GetCount(); i++)
        {
            m_Files->SetSelection(i,false);
        }
    }
    void OnSelInv(wxCommandEvent&)
    {
        for (int i=0; i<(int)m_Files->GetCount(); i++)
        {
            m_Files->SetSelection(i,!m_Files->IsSelected(i));
        }
    }
    void OnAddFilter(wxCommandEvent&)
    {
        wxTextEntryDialog dlg(this,L"Please enter a wildcard filter:",L"Filter Added Files",L"*.*");
        if (dlg.ShowModal()!=wxID_OK)
            return;

        wxCheckBox* f=new wxCheckBox(m_FiltersPanel,CTRLID_FILTER+m_Filters.size(),dlg.GetValue(),wxPoint(-1,-1),wxSize(-1,-1),wxCHK_3STATE|wxCHK_ALLOW_3RD_STATE_FOR_USER);
        m_Filters.push_back(f);
        m_FiltersSizer->Add(f);
        f->Set3StateValue(wxCHK_UNDETERMINED);
        m_FiltersSizer->FitInside(m_FiltersPanel);

        ResetFilters();
    }
    void OnDelFilter(wxCommandEvent&)
    {
        int i;
        wxArrayString strings;
        for (i=0; i<(int)m_Filters.size(); i++)
        {
            strings.push_back(m_Filters[i]->GetLabel());
        }

        wxMultiChoiceDialog dlg(this,L"Please select filters to delete",L"Delete Filters",strings);
        if (dlg.ShowModal()!=wxID_OK)
            return;

        m_FiltersSizer->Clear();

        for (i=0; i<(int)dlg.GetSelections().size(); i++)
        {
            delete m_Filters[dlg.GetSelections()[i]];
            m_Filters[dlg.GetSelections()[i]]=NULL;
        }

        for (i=(int)m_Filters.size()-1; i>=0; i--)
        {
            if (m_Filters[i]==NULL)
                m_Filters.erase(m_Filters.begin()+i);
        }

        for (i=0; i<(int)m_Filters.size(); i++)
        {
            m_Filters[i]->SetId(CTRLID_FILTER+i);
            m_FiltersSizer->Add(m_Filters[i]);
        }
        m_FiltersSizer->FitInside(m_FiltersPanel);
    }
    void OnFilter(wxCommandEvent& e)
    {
        int i,j=e.GetId()-CTRLID_FILTER;
        if (j<0 || j>(int)m_Filters.size())
            return;

        for (i=0; i<(int)m_Files->GetCount(); i++)
        {
            if (m_Files->GetString(i).Lower().Matches(m_Filters[j]->GetLabel().Lower()))
            {
                switch (m_Filters[j]->Get3StateValue())
                {
                case wxCHK_CHECKED:
                    m_Files->Check(i,true);     break;
                case wxCHK_UNCHECKED:
                    m_Files->Check(i,false);    break;
                case wxCHK_UNDETERMINED:
                    break;
                }
            }
        }
    }
    void OnFiles(wxCommandEvent& e)
    {
        wxArrayInt Sel;
        int i,j,len=m_Files->GetSelections(Sel);

        for (i=0; i<len; i++)
        {
            if (Sel[i]==e.GetInt())
                break;
        }
        if (i<len)
        {
            for (j=0; j<len; j++)
            {
                if (j!=i)
                    m_Files->Check(Sel[j],m_Files->IsChecked(Sel[i]));
            }
        }

        ResetFilters();
    }
    void ResetFilters()
    {
        for (int j=0; j<(int)m_Filters.size(); j++)
        {
            int t=0,f=0;

            for (int i=0; i<(int)m_Files->GetCount(); i++)
            {
                if (m_Files->GetString(i).Lower().Matches(m_Filters[j]->GetLabel().Lower()))
                {
                    if (m_Files->IsChecked(i))
                        t++;
                    else
                        f++;
                }
            }
            if (t==0 && f>0)
                m_Filters[j]->Set3StateValue(wxCHK_UNCHECKED);
            else if (t>0 && f==0)
                m_Filters[j]->Set3StateValue(wxCHK_CHECKED);
            else if (t>0 && f>0)
                m_Filters[j]->Set3StateValue(wxCHK_UNDETERMINED);
        }
    }
    void Done()
    {
        int i;
        for (i=0; i<(int)m_Filters.size(); i++)
        {
            switch (m_Filters[i]->Get3StateValue())
            {
            case wxCHK_CHECKED:
                m_fp.AddLine(wxString::Format(L"filter \"%s\" %s",m_Filters[i]->GetLabel().c_str(),L"true"));
                break;
            case wxCHK_UNCHECKED:
                m_fp.AddLine(wxString::Format(L"filter \"%s\" %s",m_Filters[i]->GetLabel().c_str(),L"false"));
                break;
            case wxCHK_UNDETERMINED:
                m_fp.AddLine(wxString::Format(L"filter \"%s\" %s",m_Filters[i]->GetLabel().c_str(),L"none"));
                break;
            }
        }

        for (i=0; i<(int)m_Expands.size(); i++)
        {
            m_fp.AddLine(wxString::Format(L"expand \"%s\" %s",m_Expands[i]->GetLabel().c_str(),m_Expands[i]->GetValue() ? L"true" : L"false"));

            for (int j=0; j<(int)m_ExpandTypes.size(); j++)
            {
                if (m_ExpandTypes[j].m_Name.IsSameAs(m_Expands[i]->GetLabel(),false))
                    m_ExpandTypes[j].m_Checked=m_Expands[i]->GetValue();
            }
        }
        for (i=0; i<(int)m_ExtraExpands.size(); i++)
        {
            m_fp.AddLine(wxString::Format(L"expand \"%s\" %s",m_ExtraExpands[i].c_str(),m_ExpandChecks[i] ? L"true" : L"false"));
        }

        for (i=0; i<(int)m_Files->GetCount(); i++)
        {
            if (!m_Files->IsChecked(i))
                m_DataList[m_FileOffsets[i]]=NULL;
        }

        m_fp.Write();
    }

    DECLARE_EVENT_TABLE()
};

#define EVT_CHECKBOX_RANGE(id1, id2, func) wx__DECLARE_EVT2(wxEVT_COMMAND_CHECKBOX_CLICKED, id1, id2, wxCommandEventHandler(func))

BEGIN_EVENT_TABLE(CIwLayoutDataFilter, CIwStyleDialog)
    EVT_BUTTON(wxID_OK,CIwLayoutDataFilter::OnOK)
    EVT_BUTTON(CTRLID_SELALL,CIwLayoutDataFilter::OnSelAll)
    EVT_BUTTON(CTRLID_SELNONE,CIwLayoutDataFilter::OnSelNone)
    EVT_BUTTON(CTRLID_SELINV,CIwLayoutDataFilter::OnSelInv)
    EVT_BUTTON(CTRLID_ADDFILTER,CIwLayoutDataFilter::OnAddFilter)
    EVT_BUTTON(CTRLID_DELFILTER,CIwLayoutDataFilter::OnDelFilter)
    EVT_CHECKBOX_RANGE(CTRLID_FILTER,CTRLID_FILTEREND,CIwLayoutDataFilter::OnFilter)
    EVT_CHECKLISTBOX(CTRLID_FILES,CIwLayoutDataFilter::OnFiles)
END_EVENT_TABLE()

//------------------------------------------------------------------------------
bool CIwLayoutData::FilterAndExpand(std::vector<CIwASDData*>& dataList,std::vector<ExpandType>& expandTypes,const wxString& show,bool all)
{
    int i=0,j,num=0;

    while (true)
    {
        int len=(int)dataList.size();
        for (; i<len; i++)
        {
            num++;
            for (j=0; j<(int)expandTypes.size(); j++)
            {
                if (dataList[i]==NULL)
                    break;

                if (dataList[i]->HasType(expandTypes[j].m_Type))
                {
                    if (!expandTypes[j].m_PreExpand)
                    {
                        num++;  //needs options
                        continue;
                    }

                    if (dataList[i]->ExpandList(dataList,expandTypes[j].m_ToType))
                    {
                        dataList[i]=NULL;
                        num--;
                    }
                }
            }
        }

        if (len==(int)dataList.size())
            break;
    }
    if (num<(all ? 1 : 2)) //1 or 0 added and no options needed
        return true;

    CIwLayoutDataFilter dlg(CIwTheFrame,dataList,expandTypes,show,all);
    if (dlg.ShowModal()!=wxID_OK)
    {
        dlg.Done();
        for (i=0; i<(int)dataList.size(); i++)
        {
            dataList[i]=NULL;
        }
        return false;
    }

    dlg.Done();

    i=0;

    while (true)
    {
        int len=(int)dataList.size();
        for (; i<len; i++)
        {
            for (j=0; j<(int)expandTypes.size(); j++)
            {
                if (dataList[i]==NULL)
                    break;

                if (expandTypes[j].m_PreExpand)
                    continue;

                if (!expandTypes[j].m_Checked)
                    continue;

                if (dataList[i]->HasType(expandTypes[j].m_Type))
                    if (dataList[i]->ExpandList(dataList,expandTypes[j].m_ToType))
                        dataList[i]=NULL;

            }
        }

        if (len==(int)dataList.size())
            break;
    }
    for (i=0; i<(int)dataList.size(); i++)
    {
        for (j=i+1; j<(int)dataList.size(); j++)
        {
            if (dataList[i]==dataList[j])
                dataList[j]=NULL;
        }
    }

    return true;
}

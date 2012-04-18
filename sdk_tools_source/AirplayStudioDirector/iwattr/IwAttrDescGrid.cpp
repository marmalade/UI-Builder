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
#include "IwAttrDescGrid.h"

BEGIN_EVENT_TABLE(CIwAttrGrid, wxGrid)
    EVT_GRID_CELL_LEFT_CLICK(CIwAttrGrid::OnCellLeftClick)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CIwAttrGridCombo, wxComboBox)
    EVT_COMBOBOX(wxID_ANY,CIwAttrGridCombo::OnChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CIwAttrGridNumber, wxSpinCtrl)
    EVT_COMMAND_SCROLL(wxID_ANY,CIwAttrGridNumber::OnChange)
    EVT_TEXT(wxID_ANY,CIwAttrGridNumber::OnChangeText)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(CIwAttrGridText, wxTextCtrl)
    EVT_TEXT(wxID_ANY,CIwAttrGridText::OnChange)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
class CIwAttrGridPtr : public wxObject
{
public:
    CIwAttrGrid* ptr;
    CIwAttrGridPtr(CIwAttrGrid* _ptr) : ptr(_ptr) {}
};

//-----------------------------------------------------------------------------
void CIwAttrGrid::OnMouseMotion(wxMouseEvent& e)
{
    CIwAttrGrid* grid=((CIwAttrGridPtr*)e.m_callbackUserData)->ptr;

    wxPoint pt=grid->CalcUnscrolledPosition(e.GetPosition());
    int row=grid->YToRow(pt.y);
    int col=grid->XToCol(pt.x);

    if (m_OldCurrRow!=row || m_OldCurrCol!=col)
    {
        m_OldCurrRow=row;
        m_OldCurrCol=col;

        wxString hint=grid->GetToolTip(row,col);
        grid->GetGridWindow()->SetToolTip(hint);
    }

    e.Skip();
}

//-----------------------------------------------------------------------------
wxString CIwAttrGrid::GetToolTip(int row,int col)
{
    wxGridCellRenderer* rend=GetCellRenderer(row,col);
    wxGridCellButtonRenderer* button=dynamic_cast<wxGridCellButtonRenderer*>(rend);
    wxGridCellButtonIconRenderer* icon=dynamic_cast<wxGridCellButtonIconRenderer*>(rend);
    if (button!=NULL || icon!=NULL)
        return GetCellValue(row,col);

    return L"";
}

//-----------------------------------------------------------------------------
void CIwAttrGridCombo::OnChange(wxCommandEvent& event)
{
    m_Editor->Changed();
    event.Skip();
}

//-----------------------------------------------------------------------------
CIwAttrGridNumberEditor::CIwAttrGridNumberEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,int min,int max) :
    wxGridCellNumberEditor(min,max),IIwAttrDataEditor(data),m_Min2(min),m_Max2(max),m_Adjust(false),m_Num(num),m_Alts(alts)
{
}

//-----------------------------------------------------------------------------
void CIwAttrGridNumberEditor::Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler)
{
    if (HasRange())
        m_control=new CIwAttrGridNumber(parent,id,this,m_Min2,m_Max2);
    else
    {
        m_control=new CIwAttrGridText(parent,id,this);

        Text()->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
    }

    wxGridCellEditor::Create(parent,id,evtHandler);
}

//-----------------------------------------------------------------------------
void CIwAttrGridNumberEditor::Changed(int pos)
{
    if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        m_Data->m_Items[m_Num].m_UInt=pos;
    else
        m_Data->m_Items[m_Num].m_Int=pos;

    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
        m_Alts[i]->m_Items[m_Num].m_UInt=m_Data->m_Items[m_Num].m_UInt;
    }

    /*m_Adjust=true;
       if(m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        Spin()->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_UInt));
       else
        Spin()->SetValue(wxString::Format(wxT("%d"),m_Data->m_Items[m_Num].m_Int));
       m_Adjust=false;*/
    //m_Data->SetChanged();
}
//-----------------------------------------------------------------------------
void CIwAttrGridNumberEditor::Changed()
{
    if (m_Adjust) return;

    if (HasRange())
    {
        if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            m_Data->m_Items[m_Num].m_UInt=Spin()->GetValue();
        else
            m_Data->m_Items[m_Num].m_Int=Spin()->GetValue();
    }
    else
    {
        if (m_Data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
            m_Data->m_Items[m_Num].m_UInt=atoi(Text()->GetValue().mb_str());
        else
            m_Data->m_Items[m_Num].m_Int=atoi(Text()->GetValue().mb_str());
    }

    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_Int=m_Data->m_Items[m_Num].m_Int;
        m_Alts[i]->m_Items[m_Num].m_UInt=m_Data->m_Items[m_Num].m_UInt;
    }

    m_Data->SetChanged();
}

//-----------------------------------------------------------------------------
CIwAttrGridTextEditor::CIwAttrGridTextEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts) :
    IIwAttrDataEditor(data),m_Num(num),m_Alts(alts)
{
}

//-----------------------------------------------------------------------------
void CIwAttrGridTextEditor::Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler)
{
    m_control=new CIwAttrGridText(parent,id,this);
    wxGridCellEditor::Create(parent,id,evtHandler);
}
//-----------------------------------------------------------------------------
void CIwAttrGridTextEditor::Changed()
{
    m_Data->m_Items[m_Num].m_String=Text()->GetValue();
    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_String=Text()->GetValue();
    }
    m_Data->SetChanged();

    // We need to reset whole grid panel or else we end up with
    // widgets referencing deleted CIwAttrData objects.
    CIwAttrClass* pClass = m_Data->m_Member->m_Items.size() ?
                           m_Data->m_Member->m_Items[0].m_Class : NULL;
    if (pClass)
    {
        if (pClass->m_Name==L"CIwPropertySet" || pClass->m_Name==L"CIwUIPropertySet")
        {
            CIwAttrDialog* pDialog = m_Data->m_Instance ? m_Data->m_Instance->m_Dialog : NULL;
            if (pDialog)
                pDialog->ScheduleReset();
        }
    }
}
//-----------------------------------------------------------------------------
CIwAttrGridFloatEditor::CIwAttrGridFloatEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts) :
    IIwAttrDataEditor(data),m_Num(num),m_Alts(alts)
{
}

//-----------------------------------------------------------------------------
void CIwAttrGridFloatEditor::Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler)
{
    m_control=new CIwAttrGridText(parent,id,this);
    wxGridCellEditor::Create(parent,id,evtHandler);
}
//-----------------------------------------------------------------------------
void CIwAttrGridFloatEditor::Changed()
{
    m_Data->m_Items[m_Num].m_Float=atof(Text()->GetValue().mb_str());
    for (int i=0; i<(int)m_Alts.size(); i++)
    {
        m_Alts[i]->m_Items[m_Num].m_Float=atof(Text()->GetValue().mb_str());
    }
    m_Data->SetChanged();
}

//-----------------------------------------------------------------------------
CIwAttrGridComboEditor::CIwAttrGridComboEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,const wxArrayString& strings,Mode mode,bool allowOthers) :
    wxGridCellChoiceEditor(strings,allowOthers),IIwAttrDataEditor(data),m_Mode(mode),m_Num(num),m_Alts(alts)
{
    SetClientData((void*)&m_pointActivate);
}
//-----------------------------------------------------------------------------
CIwAttrGridComboEditor::CIwAttrGridComboEditor(CIwAttrData* data,int num,std::vector<CIwAttrData*> alts,Mode mode) : wxGridCellChoiceEditor(),IIwAttrDataEditor(data),m_Mode(mode),m_Num(num),m_Alts(alts)
{
    switch (mode)
    {
    case MODE_BOOL:
        m_choices.Add(L"false");
        m_choices.Add(L"true");
        break;
    case MODE_3STATE:
        m_choices.Add(L"<mixed>");
        m_choices.Add(L"false");
        m_choices.Add(L"true");
        break;
    default:
        break;
    }
    SetClientData((void*)&m_pointActivate);
}

//-----------------------------------------------------------------------------
CIwAttrGridComboEditor::CIwAttrGridComboEditor(const wxArrayString& strings) :
    wxGridCellChoiceEditor(strings,false),IIwAttrDataEditor(NULL),m_Mode(MODE_STRINGS),m_Num(0)
{
    SetClientData((void*)&m_pointActivate);
}

//-----------------------------------------------------------------------------
wxGridCellEditor *CIwAttrGridComboEditor::Clone() const
{
    if (m_Mode==MODE_STRINGS)
        return new CIwAttrGridComboEditor(m_choices);
    else if (m_Mode==MODE_ENUM)
        return new CIwAttrGridComboEditor(m_Data,m_Num,m_Alts,m_choices,m_Mode,m_allowOthers);
    else
        return new CIwAttrGridComboEditor(m_Data,m_Num,m_Alts,m_Mode);
}

//-----------------------------------------------------------------------------
void CIwAttrGridComboEditor::Create(wxWindow* parent,wxWindowID id,wxEvtHandler* evtHandler)
{
    m_control = new CIwAttrGridCombo(parent,id,m_choices,this,m_allowOthers ? 0 : wxCB_READONLY);
    wxGridCellEditor::Create(parent,id,evtHandler);
}

//-----------------------------------------------------------------------------
void CIwAttrGridComboEditor::BeginEdit(int row, int col, wxGrid* grid)
{
    CIwAttrGrid* pEzGrid=(CIwAttrGrid*)grid;
    pEzGrid->RevertSel();

    wxGridCellChoiceEditor::BeginEdit(row,col,grid);

    if (m_pointActivate.x > -1 && m_pointActivate.y > -1)
    {
        m_pointActivate = Combo()->ScreenToClient(m_pointActivate);
#ifdef I3D_OS_WINDOWS
        SendMessage((HWND)Combo()->GetHandle(), WM_LBUTTONDOWN, 0,MAKELPARAM(m_pointActivate.x, m_pointActivate.y));
#endif
    }
}

//-----------------------------------------------------------------------------
void CIwAttrGridComboEditor::Changed()
{
    int i;
    switch (m_Mode)
    {
    case MODE_3STATE:
        if (!Combo()->GetValue().IsSameAs(L"<mixed>"))
        {
            m_Mode=MODE_BOOL;
            m_choices.erase(m_choices.begin());
            Combo()->Delete(0);
        }

    case MODE_BOOL:
        if (Combo()->GetValue().IsSameAs(L"false"))
        {
            m_Data->m_Items[m_Num].m_Int=0;
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Int=0;
            }
        }
        else if (Combo()->GetValue().IsSameAs(L"true"))
        {
            m_Data->m_Items[m_Num].m_Int=1;
            for (i=0; i<(int)m_Alts.size(); i++)
            {
                m_Alts[i]->m_Items[m_Num].m_Int=1;
            }
        }

        break;
    case MODE_ENUM:
        for (i=1; i<(int)m_Data->m_Member->m_Text.size(); i++)
        {
            if (m_Data->m_Member->m_Text[i].IsSameAs(Combo()->GetValue(),false))
            {
                m_Data->m_Items[m_Num].m_Int=i-1;
                for (int j=0; j<(int)m_Alts.size(); j++)
                {
                    m_Alts[j]->m_Items[m_Num].m_Int=i-1;
                }

                CIwAttrInstance* inst=m_Data->m_Instance;
                if (m_Data->m_Member->m_Items[0].m_Class!=NULL)
                {
                    inst->m_Dialog->ScheduleReset();
                    inst->Reset(inst->m_Class);
                }

                inst->SetChanged(true);
                m_Data->m_FromDefault=false;
                return;
            }
        }
        break;
    case MODE_STRINGS:
        return;
    case MODE_CLASSES:
        m_Data->m_Items[m_Num].m_Class=m_Data->m_Mgr->GetClass(Combo()->GetValue());
        if (m_Data->m_Items[m_Num].m_Class==NULL)
            m_Data->m_Items[m_Num].m_Class=m_Data->m_Member->m_Parent;

        for (int j=0; j<(int)m_Alts.size(); j++)
        {
            m_Alts[j]->m_Items[m_Num].m_Class=m_Data->m_Items[m_Num].m_Class;
        }

        CIwAttrInstance* inst=m_Data->m_Instance;
        inst->Reset(inst->m_Class);
        inst->SetChanged(true);
        inst->ResetDlg();
        return;
    }
    m_Data->SetChanged();
}

static wxDash dashes[]={1,2,1,2};

//-----------------------------------------------------------------------------
void wxGridCellTextureRenderer::Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);

    if (m_Bmp==NULL) return;

    dc.DrawBitmap(*m_Bmp,rect.x+(rect.width-m_Bmp->GetWidth())/2,rect.y); //+(rect.height-m_Bmp->GetHeight())/2);
}

//-----------------------------------------------------------------------------
void wxGridCellExpandRenderer::Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rect,int row,int col,bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rect, row, col, isSelected);
    int w=18;
    int h=9;
    wxRect rectCell(rect);
    wxRect rectButton(rectCell.x+(w-h)/2,rectCell.y+(rectCell.height-h)/2,h,h);
    wxPen dots(attr.GetTextColour(),1,wxUSER_DASH);

    dots.SetDashes(4,dashes);

    dc.SetBrush(*wxWHITE_BRUSH);
    for (int i=0; i<(int)m_Types.size(); i++)
    {
        if (m_Types[i]==4)
            break;

        switch (m_Types[i]&15)
        {
        case 1:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x+w/2,rectCell.y,rectCell.x+w/2,rectCell.y+rectCell.height);
            break;
        case 2:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x+w/2,rectCell.y,rectCell.x+w/2,rectCell.y+rectCell.height/2);
            dc.DrawLine(rectCell.x+w/2,rectCell.y+rectCell.height/2,rectCell.x+w,rectCell.y+rectCell.height/2);
            break;
        case 3:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x+w/2,rectCell.y,rectCell.x+w/2,rectCell.y+rectCell.height);
            dc.DrawLine(rectCell.x+w/2,rectCell.y+rectCell.height/2,rectCell.x+w,rectCell.y+rectCell.height/2);
            break;
        case 4:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x,rectCell.y+rectCell.height/2,rectCell.x+w,rectCell.y+rectCell.height/2);
            break;
        case 5:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x,rectCell.y+rectCell.height/2,rectCell.x+w,rectCell.y+rectCell.height/2);
            dc.DrawLine(rectCell.x+w/2,rectCell.y+rectCell.height/2,rectCell.x+w/2,rectCell.y+rectCell.height);
            break;
        case 6:
            dc.SetPen(dots);
            dc.DrawLine(rectCell.x+w/2,rectCell.y+rectCell.height/2,rectCell.x+w,rectCell.y+rectCell.height/2);
            dc.DrawLine(rectCell.x+w/2,rectCell.y+rectCell.height/2,rectCell.x+w/2,rectCell.y+rectCell.height);
            break;
        }
        if (m_Types[i]&32)
        {
            dc.SetPen(wxPen(attr.GetTextColour(), 1, wxSOLID));
            dc.DrawRectangle(rectButton);
            dc.DrawLine(rectButton.x+2,rectButton.y+rectButton.height/2,rectButton.x+rectButton.width-2,rectButton.y+rectButton.height/2);
            dc.DrawLine(rectButton.x+rectButton.width/2,rectButton.y+2,rectButton.x+rectButton.width/2,rectButton.y+rectButton.height-2);
        }
        else if (m_Types[i]&16)
        {
            dc.SetPen(wxPen(attr.GetTextColour(), 1, wxSOLID));
            dc.DrawRectangle(rectButton);
            dc.DrawLine(rectButton.x+2,rectButton.y+rectButton.height/2,rectButton.x+rectButton.width-2,rectButton.y+rectButton.height/2);
        }

        rectButton.x+=w;
        rectCell.x+=w;
    }

    SetTextColoursAndFont(grid, attr, dc, isSelected);
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    // leave room for button
    wxRect rect2 = rect;
    w=rectCell.x-rect.x;
    rect2.x+=w;
    rect2.width-=w;
    rect2.Inflate(-1);
    grid.DrawTextRectangle(dc, grid.GetCellValue(row, col), rect2, hAlign, vAlign);
}

//-----------------------------------------------------------------------------
void wxGridCellButtonRenderer::Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rectCell,int row,int col,bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
    // first calculate button size
    // don't draw outside the cell
    //int nButtonWidth = 17;
    if (rectCell.height < 2) return;

    wxRect rectButton;
    rectButton.x = rectCell.x + 1;
    rectButton.y = rectCell.y + 1;
    int cell_rows, cell_cols;
    attr.GetSize(&cell_rows, &cell_cols);
    rectButton.width = rectCell.width-2;
    rectButton.height = rectCell.height-2;

    // don't bother drawing if the cell is too small
    if (rectButton.height < 4 || rectButton.width < 4) return;  // draw 3-d button

    wxColour colourBackGround = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    if (m_Data!=NULL)
        colourBackGround=wxColour(m_Data->m_Items[0].m_Int,m_Data->m_Items[1].m_Int,m_Data->m_Items[2].m_Int);

    dc.SetBrush(wxBrush(colourBackGround, wxSOLID));
    dc.SetPen(wxPen(colourBackGround, 1, wxSOLID));
    dc.DrawRectangle(rectButton);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT), 1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetBottom());
    dc.DrawLine(rectButton.GetRight(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetTop()-1);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetBottom()-1);
    dc.DrawLine(rectButton.GetRight()-1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetTop());
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetRight()-2, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetTop()+1);
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetBottom()-1);

    if (m_Data!=NULL)
        return;

    SetTextColoursAndFont(grid, attr, dc, isSelected);
    if (m_Data!=NULL && m_Data->m_Items[0].m_Int+m_Data->m_Items[1].m_Int+m_Data->m_Items[2].m_Int<=128*3)
        dc.SetTextForeground(wxColour(wxT("WHITE")));

    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    // leave room for button
    wxRect rect = rectButton;
    grid.DrawTextRectangle(dc, grid.GetCellValue(row, col), rect, wxALIGN_CENTRE,wxALIGN_CENTRE);
}

//-----------------------------------------------------------------------------
void wxGridCellButtonIconRenderer::Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rectCell,int row,int col,bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
    // first calculate button size
    // don't draw outside the cell
    //int nButtonWidth = 17;
    if (rectCell.height < 2) return;

    wxRect rectButton;
    rectButton.x = rectCell.x + 1;
    rectButton.y = rectCell.y + 1;
    int cell_rows, cell_cols;
    attr.GetSize(&cell_rows, &cell_cols);
    rectButton.width = rectCell.width-2;
    rectButton.height = rectCell.height-2;

    // don't bother drawing if the cell is too small
    if (rectButton.height < 4 || rectButton.width < 4) return;  // draw 3-d button

    wxColour colourBackGround = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    if (m_Data!=NULL)
        colourBackGround=wxColour(m_Data->m_Items[0].m_Int,m_Data->m_Items[1].m_Int,m_Data->m_Items[2].m_Int);

    dc.SetBrush(wxBrush(colourBackGround, wxSOLID));
    dc.SetPen(wxPen(colourBackGround, 1, wxSOLID));
    dc.DrawRectangle(rectButton);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT), 1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetBottom());
    dc.DrawLine(rectButton.GetRight(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetTop()-1);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetBottom()-1);
    dc.DrawLine(rectButton.GetRight()-1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetTop());
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetRight()-2, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetTop()+1);
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetBottom()-1);

    dc.DrawBitmap(*m_Bmp,wxPoint(rectButton.x,rectButton.y));
}

//-----------------------------------------------------------------------------
void wxGridCellChoiceRenderer::Draw(wxGrid& grid,wxGridCellAttr& attr,wxDC& dc,const wxRect& rectCell,int row,int col,bool isSelected)
{
    wxGridCellRenderer::Draw(grid, attr, dc, rectCell, row, col, isSelected);
    // first calculate button size
    // don't draw outside the cell
    int nButtonWidth = 17;
    if (rectCell.height < 2) return;

    wxRect rectButton;
    rectButton.x = rectCell.x + rectCell.width - nButtonWidth;
    rectButton.y = rectCell.y + 1;
    int cell_rows, cell_cols;
    attr.GetSize(&cell_rows, &cell_cols);
    rectButton.width = nButtonWidth;
    if (cell_rows == 1)
        rectButton.height = rectCell.height-2;
    else
        rectButton.height = nButtonWidth;

    SetTextColoursAndFont(grid, attr, dc, isSelected);
    int hAlign, vAlign;
    attr.GetAlignment(&hAlign, &vAlign);
    // leave room for button
    wxRect rect = rectCell;
    rect.SetWidth(rectCell.GetWidth() - rectButton.GetWidth()-2);
    rect.Inflate(-1);
    grid.DrawTextRectangle(dc, grid.GetCellValue(row, col), rect, hAlign, vAlign);

    // don't bother drawing if the cell is too small
    if (rectButton.height < 4 || rectButton.width < 4) return;  // draw 3-d button

    wxColour colourBackGround = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    dc.SetBrush(wxBrush(colourBackGround, wxSOLID));
    dc.SetPen(wxPen(colourBackGround, 1, wxSOLID));
    dc.DrawRectangle(rectButton);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT), 1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetBottom());
    dc.DrawLine(rectButton.GetRight(), rectButton.GetBottom(),
                rectButton.GetRight(), rectButton.GetTop()-1);
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetBottom()-1);
    dc.DrawLine(rectButton.GetRight()-1, rectButton.GetBottom()-1,
                rectButton.GetRight()-1, rectButton.GetTop());
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT),
                    1, wxSOLID));
    dc.DrawLine(rectButton.GetRight()-2, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetTop()+1);
    dc.DrawLine(rectButton.GetLeft()+1, rectButton.GetTop()+1,
                rectButton.GetLeft()+1, rectButton.GetBottom()-1);
    // Draw little triangle
    int nTriWidth = 7;
    int nTriHeight = 4;
    wxPoint point[3];
    point[0] = wxPoint(rectButton.GetLeft() + (rectButton.GetWidth()-nTriWidth)/2,
                       rectButton.GetTop()+(rectButton.GetHeight()-nTriHeight)/2);
    point[1] = wxPoint(point[0].x+nTriWidth-1, point[0].y);
    point[2] = wxPoint(point[0].x+3, point[0].y+nTriHeight-1);
    dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT), wxSOLID));
    dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT), 1, wxSOLID));
    dc.DrawPolygon(3, point);
    if (m_border == wxLAYOUT_TOP)
    {
        dc.SetPen(wxPen(*wxBLACK, 1, wxDOT));
        dc.DrawLine(rectCell.GetRight(), rectCell.GetTop(),
                    rectCell.GetLeft(), rectCell.GetTop());
    }
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::OnCellLeftClick(wxGridEvent& ev)
{
    // This forces the cell to go into edit mode directly
    SetGridCursor(ev.GetRow(),ev.GetCol());
    // Store the click co-ordinates in the editor if possible
    // if an editor has created a ClientData area, we presume it's
    // a wxPoint and we store the click co-ordinates
    wxGridCellEditor* pEditor =GetCellEditor(ev.GetRow(),ev.GetCol());
    wxPoint* pClickPoint=(wxPoint*)pEditor->GetClientData();
    if (pClickPoint)
        *pClickPoint = ClientToScreen(ev.GetPosition());

    // to prevent selection from being lost when click combobox:
    if (ev.GetCol() == 1 && IsInSelection(ev.GetRow(),ev.GetCol()))
    {
        m_selTemp = m_selection;
        m_selection = NULL;
    }

    ev.Skip();
    pEditor->DecRef();

    if (ev.GetRow()<(int)m_DataList.size() && m_DataList[ev.GetRow()]!=NULL)
        Activate(m_DataList[ev.GetRow()],ev.GetRow(),ev.GetCol());
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::RemoveData(CIwAttrData* data)
{
    CIwAttrInstance* inst=data->m_Instance;
    for (std::vector<CIwAttrData*>::iterator it=inst->m_Data.begin(); it!=inst->m_Data.end(); )
    {
        CIwAttrData* data2;
        for (data2=(*it); data2!=NULL; data2=data2->m_Group)
        {
            if (data2==data)
            {
                it=inst->m_Data.erase(it);
                break;
            }
        }
        if (data2==NULL)
            ++it;
    }
    delete data;
    inst->SetChanged(true);
    inst->ResetDlg();
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::Activate(DataItem* data,int row,int col)
{
    int i;
    wxArrayString strings;

    if (!data->m_Bases.empty())
    {
        if (col==4 && GetCellValue(row,col)==L"Add")
        {
            wxString value=GetCellValue(row,1);
            if (value.empty())
                return;

            CIwAttrNote Note;
            Note.m_Info=-1;
            Note.m_Data=L"{";

            std::vector<wxString> argv;
            argv.push_back(wxT("{"));

            for (int i=0; i<(int)data->m_Bases.size(); i++)
            {
                CIwAttrInstance* Inst2=data->m_Bases[i];

                Inst2->AddFromNote(value,Note,argv,Inst2);

                Inst2->SetChanged(true);
            }
            data->m_Bases[0]->ResetDlg();
        }

        return;
    }

    if (data->m_Data->m_Instance->m_Dialog!=NULL)
        if (data->m_Data->m_Instance->m_Dialog->GridOverrideActivate(this,data->m_Data,data->m_Alts,row,col))
            return;

    if (data->m_Data->m_Member->m_Type&ATTRMEMBER_LIST && (data->m_Data->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_INT124)
    {
        if (col==4 && data->m_Offset!=-1)
        {
            data->m_Data->m_Items.erase(data->m_Data->m_Items.begin()+data->m_Offset);
            data->m_Data->SetChanged();
            data->m_Data->m_Instance->ResetDlg();
            return;
        }

        if (col==1 && data->m_Offset==-1)
        {
            data->m_Data->m_Items.resize(data->m_Data->m_Items.size()+1);
            data->m_Data->SetChanged();
            data->m_Data->m_Instance->ResetDlg();
            return;
        }

        if (col==0 && data->m_Offset==-2)
        {
            if (data->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)
                data->m_Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
            else
                data->m_Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;

            data->m_Data->m_Instance->ResetDlg();
        }
    }

    if (data->m_Data->m_Member->m_Type&ATTRMEMBER_0ORMORE && col==4)
    {
        RemoveData(data->m_Data);
        for (i=0; i<(int)data->m_Alts.size(); i++)
        {
            RemoveData(data->m_Alts[i]);
        }
        return;
    }

    switch (data->m_Data->m_Member->m_Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_COLOUR3:
    case ATTRMEMBER_COLOUR:
        if (col==1)
        {
            wxColourData Data;
            wxColourDialog Dlg(this,&Data);
            Data.SetColour(wxColour(data->m_Data->m_Items[0].m_Int,data->m_Data->m_Items[1].m_Int,data->m_Data->m_Items[2].m_Int));

            if (Dlg.ShowModal()==wxID_OK)
            {
                data->m_Data->m_Items[0].m_Int=Dlg.GetColourData().GetColour().Red();
                data->m_Data->m_Items[1].m_Int=Dlg.GetColourData().GetColour().Green();
                data->m_Data->m_Items[2].m_Int=Dlg.GetColourData().GetColour().Blue();

                for (int i=0; i<(int)data->m_Alts.size(); i++)
                {
                    data->m_Alts[i]->m_Items[0].m_Int=data->m_Data->m_Items[0].m_Int;
                    data->m_Alts[i]->m_Items[1].m_Int=data->m_Data->m_Items[1].m_Int;
                    data->m_Alts[i]->m_Items[2].m_Int=data->m_Data->m_Items[2].m_Int;
                    data->m_Alts[i]->m_Items[3].m_Int=data->m_Data->m_Items[3].m_Int;
                }

                data->m_Data->SetChanged();
            }
        }

        break;
    case ATTRMEMBER_INT124:
        if (col!=3) return;

        switch (data->m_Data->m_Items.size())
        {
        case 1:
            data->m_Data->m_Items.resize(2,data->m_Data->m_Items[0]);
            break;
        case 2:
            data->m_Data->m_Items.resize(4);
            data->m_Data->m_Items[3]=data->m_Data->m_Items[1];
            data->m_Data->m_Items[2]=data->m_Data->m_Items[1];
            data->m_Data->m_Items[1]=data->m_Data->m_Items[0];
            break;
        default:
            data->m_Data->m_Items.resize(1);
            break;
        }
        data->m_Data->m_Instance->ResetDlg();
        break;
    case ATTRMEMBER_GROUP:
    case ATTRMEMBER_CLASS:
        if (col!=0) return;

        if (data->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)
            data->m_Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
        else
            data->m_Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;

        for (i=0; i<(int)data->m_Alts.size(); i++)
        {
            data->m_Alts[i]->m_Items[0].m_Flags=data->m_Data->m_Items[0].m_Flags;
        }
        data->m_Data->m_Instance->ResetDlg();
        break;
    case ATTRMEMBER_FILEFONT:
        if (col==3)
        {
            strings.Add(L"");
            if (m_Mgr.m_Extra!=NULL)
                m_Mgr.m_Extra->GetStrings(ATTRSTRING_FILEFONT,strings,data->m_Data);

            strings.Add(L"-= new =-");

            wxSingleChoiceDialog Dlg(this,wxT("Please choose an item"),wxT("Fonts"),strings);
            for (i=0; i<(int)strings.size(); i++)
            {
                if (data->m_Data->m_Items[0].m_String.IsSameAs(strings[i],false))
                    Dlg.SetSelection(i);
            }

            if (Dlg.ShowModal()==wxID_OK)
            {
                if (Dlg.GetStringSelection()==L"-= new =-")
                    data->m_Data->m_Items[0].m_String=data->m_Data->m_Instance->m_File->MakeNew(ATTRSTRING_FILEFONT);
                else
                    data->m_Data->m_Items[0].m_String=Dlg.GetStringSelection();

                for (i=0; i<(int)data->m_Alts.size(); i++)
                {
                    data->m_Alts[i]->m_Items[0].m_String=data->m_Data->m_Items[0].m_String;
                }
                SetCellValue(data->m_Data->m_Items[0].m_String,row,1);

                wxBitmap* bmp=NULL;
                if (data->m_Data->m_Instance->m_File!=NULL)
                    bmp=data->m_Data->m_Instance->m_File->GetBitmap(data->m_Data,ATTRSTRING_FILEFONT);

                ((wxGridCellTextureRenderer*)GetCellRenderer(row,2))->m_Bmp=bmp;
                data->m_Data->SetChanged();
            }
        }

        break;
    case ATTRMEMBER_FILETEXTURE:
        if (col==3)
        {
            strings.Add(L"");
            if (m_Mgr.m_Extra!=NULL)
                m_Mgr.m_Extra->GetStrings(ATTRSTRING_FILETEXTURE,strings,data->m_Data);

            strings.Add(L"-= new =-");

            wxSingleChoiceDialog Dlg(this,wxT("Please choose an item"),wxT("Textures"),strings);
            for (i=0; i<(int)strings.size(); i++)
            {
                if (data->m_Data->m_Items[0].m_String.IsSameAs(strings[i],false))
                    Dlg.SetSelection(i);
            }

            if (Dlg.ShowModal()==wxID_OK)
            {
                if (Dlg.GetStringSelection()==L"-= new =-")
                    data->m_Data->m_Items[0].m_String=data->m_Data->m_Instance->m_File->MakeNew(ATTRSTRING_FILETEXTURE);
                else
                    data->m_Data->m_Items[0].m_String=Dlg.GetStringSelection();

                for (i=0; i<(int)data->m_Alts.size(); i++)
                {
                    data->m_Alts[i]->m_Items[0].m_String=data->m_Data->m_Items[0].m_String;
                }
                SetCellValue(data->m_Data->m_Items[0].m_String,row,1);

                wxBitmap* bmp=NULL;
                if (data->m_Data->m_Instance->m_File!=NULL)
                    bmp=data->m_Data->m_Instance->m_File->GetBitmap(data->m_Data,ATTRSTRING_FILETEXTURE);

                ((wxGridCellTextureRenderer*)GetCellRenderer(row,2))->m_Bmp=bmp;
                data->m_Data->SetChanged();
            }
        }

        break;
    case ATTRMEMBER_PTR:
        if (col==0 && data->m_Data->m_Items[data->m_Offset].m_Flags&ATTRITEM_ALLOCED_F && data->m_Offset==-2)
        {
            if (data->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)
                data->m_Data->m_Items[0].m_Flags&=~ATTRITEM_HIDDEN_F;
            else
                data->m_Data->m_Items[0].m_Flags|=ATTRITEM_HIDDEN_F;

            for (i=0; i<(int)data->m_Alts.size(); i++)
            {
                data->m_Alts[i]->m_Items[0].m_Flags=data->m_Data->m_Items[0].m_Flags;
            }
            data->m_Data->m_Instance->ResetDlg();
        }
        else if (col==3)
        {
            bool gettingNew=false;
            strings.Add(L"");
            if (m_Mgr.m_Extra!=NULL)
                m_Mgr.m_Extra->GetStrings(ATTRSTRING_PTR,strings,data->m_Data);

            strings.Add(L"-= inline =-");
            strings.Add(L"-= new =-");

            wxSingleChoiceDialog Dlg(this,wxT("Please choose an item"),wxT("Items"),strings);
            for (i=0; i<(int)strings.size(); i++)
            {
                if (GetCellValue(row,1).IsSameAs(strings[i],false))
                    Dlg.SetSelection(i);
            }

            if (Dlg.ShowModal()==wxID_OK)
            {
                bool ChangedInline=false;

                wxString val=Dlg.GetStringSelection();
                if (val==L"-= inline =-")
                {
                    if ((data->m_Data->m_Items[data->m_Offset].m_Flags&ATTRITEM_ALLOCED_F)==0)
                    {
                        ChangedInline=true;

                        CIwAttrInstance* Inst=new CIwAttrInstance;
                        Inst->m_Mgr=data->m_Data->m_Instance->m_Mgr;
                        Inst->m_ClassInfo=data->m_Data->m_Instance->m_ClassInfo;
                        Inst->m_Class=data->m_Data->m_Member->m_Items[0].m_Class;
                        Inst->AddDefaults(Inst->m_Class);
                        Inst->m_Parent=data->m_Data;
                        Inst->m_File=data->m_Data->m_Instance->m_File;
                        Inst->m_Dialog=data->m_Data->m_Instance->m_Dialog;

                        if (data->m_Data->m_Instance->m_File!=NULL)
                            data->m_Data->m_Instance->m_File->SetupInlinePtr(Inst,data->m_Data->m_Member);

                        data->m_Data->m_Items[data->m_Offset].m_Flags=ATTRITEM_ALLOCED_F;
                        data->m_Data->m_Items[data->m_Offset].m_Inst=Inst;
                        data->m_Data->m_Items[data->m_Offset].m_String=L"";
                    }
                }
                else if (val==L"-= new =-")
                {
                    if ((data->m_Data->m_Items[data->m_Offset].m_Flags&ATTRITEM_ALLOCED_F)==ATTRITEM_ALLOCED_F)
                        ChangedInline=true;

                    data->m_Data->m_Items[data->m_Offset].m_Flags=0;
                    val=data->m_Data->m_Items[data->m_Offset].m_String=data->m_Data->m_Instance->m_File->MakeNew(ATTRSTRING_PTR,data->m_Data,data->m_Offset);
                    data->m_Data->m_Items[data->m_Offset].m_Inst=data->m_Data->m_Instance->m_File->TryGetPtrItem(data->m_Data->m_Items[data->m_Offset].m_String,data->m_Data);
                    gettingNew=true;
                }
                else
                {
                    if ((data->m_Data->m_Items[data->m_Offset].m_Flags&ATTRITEM_ALLOCED_F)==ATTRITEM_ALLOCED_F)
                        ChangedInline=true;

                    data->m_Data->m_Items[data->m_Offset].m_Flags=0;
                    data->m_Data->m_Items[data->m_Offset].m_Inst=data->m_Data->m_Instance->m_File->TryGetPtrItem(Dlg.GetStringSelection(),data->m_Data);
                    data->m_Data->m_Items[data->m_Offset].m_String=Dlg.GetStringSelection();
                }

                for (i=0; i<(int)data->m_Alts.size(); i++)
                {
                    data->m_Alts[i]->m_Items[data->m_Offset].m_Flags=data->m_Data->m_Items[data->m_Offset].m_Flags;
                    data->m_Alts[i]->m_Items[data->m_Offset].m_Inst=data->m_Data->m_Items[data->m_Offset].m_Inst;
                    data->m_Alts[i]->m_Items[data->m_Offset].m_String=data->m_Data->m_Items[data->m_Offset].m_String;
                }
                if (!gettingNew)
                {
                    wxBitmap* bmp=NULL;
                    if (data->m_Data->m_Instance->m_File!=NULL)
                        bmp=data->m_Data->m_Instance->m_File->GetBitmap(data->m_Data,ATTRSTRING_PTR);

                    ((wxGridCellTextureRenderer*)GetCellRenderer(row,2))->m_Bmp=bmp;
                    SetCellValue(val,row,1);
                }

                data->m_Data->SetChanged();
                data->m_Data->m_Instance->ResetDlg();
            }
        }

        break;
    }
}

//-----------------------------------------------------------------------------
CIwAttrGrid::CIwAttrGrid(wxWindow* parent,CIwAttrDescMgr& mgr) : wxGrid(parent,wxID_ANY),m_Mgr(mgr),m_selTemp(NULL),m_OldCurrRow(-1),m_OldCurrCol(-1),m_HideInline(false)
{
    m_Even=new wxGridCellAttr();
    m_Even->SetBackgroundColour(wxColour(240,240,240));
    m_Even->SetTextColour(wxColour(0,0,0));
    m_Odd=new wxGridCellAttr();
    m_Odd->SetTextColour(wxColour(0,0,0));
    m_Sect=new wxGridCellAttr();
    m_Sect->SetTextColour(wxColour(0,0,0));
    m_Sect->SetBackgroundColour(GetGridLineColour());
    m_Sect->SetReadOnly();

    EnableGridLines(true);
    SetColLabelSize(0);
    SetRowLabelSize(0);
    CreateGrid(0,5);
    SetColumnWidth(0,120);
    SetColumnWidth(1,60);
    SetColumnWidth(2,60);
    SetColumnWidth(3,20);
    SetColumnWidth(4,20);
    SetColLabelValue(0,L"member");
    SetColLabelValue(1,L"");
    SetColLabelValue(2,L"");
    SetColLabelValue(3,L"");
    SetColLabelValue(4,L"");

    GetGridWindow()->Connect(wxEVT_MOTION,wxMouseEventHandler(CIwAttrGrid::OnMouseMotion),new CIwAttrGridPtr(this),this);
}

//-----------------------------------------------------------------------------
CIwAttrGrid::~CIwAttrGrid()
{
    GetGridWindow()->Disconnect(wxEVT_MOTION,wxMouseEventHandler(CIwAttrGrid::OnMouseMotion));
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::Clean()
{
    if (GetNumberRows()>0)
        DeleteRows(0,GetNumberRows());
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::Fill(CIwAttrInstance* inst,wxArrayString* classStrings,bool basic)
{
    std::vector<CIwAttrInstance*> bases;
    bases.push_back(inst);

    m_DataList.clear();

    BeginBatch();
    Fill2(bases,classStrings,0,basic);

    SetupTree();
    EndBatch();
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::SetupTree()
{
    int i,max=0;
    std::vector<int> last;
    for (i=0; i<(int)m_DataList.size(); i++)
    {
        if (m_DataList[i]==NULL) continue;

        if (m_DataList[i]->m_Indent+1>(int)last.size())
        {
            if (i!=0)
                m_DataList[i-1]->m_Leaf=false;

            last.resize(m_DataList[i]->m_Indent+1,i);
            if (max<m_DataList[i]->m_Indent)
                max=m_DataList[i]->m_Indent;
        }
        else if (m_DataList[i]->m_Indent+1==(int)last.size())
        {
            m_DataList[last[m_DataList[i]->m_Indent]]->m_Last=false;
            last[m_DataList[i]->m_Indent]=i;
        }
        else
        {
            last.resize(m_DataList[i]->m_Indent+1,i);

            m_DataList[last[m_DataList[i]->m_Indent]]->m_Last=false;
            last[m_DataList[i]->m_Indent]=i;
        }
    }
    int indent=0;
    last.clear();
    last.resize(max+2,4);
    //SetColumnWidth(0,18*max+18);
    for (i=0; i<(int)m_DataList.size(); i++)
    {
        if (m_DataList[i]==NULL) continue;

        if (m_DataList[i]->m_Indent>indent)
        {
            if (i!=0)
            {
                if (m_DataList[i-1]->m_Last)
                    last[m_DataList[i-1]->m_Indent]=0;
                else
                    last[m_DataList[i-1]->m_Indent]=1;
            }

            indent=m_DataList[i]->m_Indent;
        }
        else if (m_DataList[i]->m_Indent<indent)
        {
            indent=m_DataList[i]->m_Indent;
            for (int j=indent+1; j<max+1; j++)
            {
                last[j]=4;
            }
        }

        if (i==0)
            last[m_DataList[i]->m_Indent]=6;
        else if (m_DataList[i]->m_Last)
            last[m_DataList[i]->m_Indent]=2;
        else
            last[m_DataList[i]->m_Indent]=3;

        if (m_DataList[i]->m_Data!=NULL)
        {
            if (m_DataList[i]->m_Offset>=0)
            {
                if ((m_DataList[i]->m_Data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_GROUP)
                {
                    if ((m_DataList[i]->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)!=ATTRITEM_HIDDEN_F)
                        last[m_DataList[i]->m_Indent]|=16;
                    else
                        last[m_DataList[i]->m_Indent]|=32;
                }

                if ((m_DataList[i]->m_Data->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_PTR && m_DataList[i]->m_Offset==-2 &&
                    (m_DataList[i]->m_Data->m_Items[m_DataList[i]->m_Offset].m_Flags&ATTRITEM_ALLOCED_F)==ATTRITEM_ALLOCED_F)
                {
                    if ((m_DataList[i]->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)!=ATTRITEM_HIDDEN_F)
                        last[m_DataList[i]->m_Indent]|=16;
                    else
                        last[m_DataList[i]->m_Indent]|=32;
                }
            }

            if ((m_DataList[i]->m_Data->m_Member->m_Type&ATTRMEMBER_LIST) && m_DataList[i]->m_Offset==-2)
            {
                if (!m_DataList[i]->m_Data->m_Items.empty() && (m_DataList[i]->m_Data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)==ATTRITEM_HIDDEN_F)
                    last[m_DataList[i]->m_Indent]|=32;
                else
                    last[m_DataList[i]->m_Indent]|=16;
            }
        }

        SetCellRenderer(i,0,new wxGridCellExpandRenderer(last));
    }
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::Fill2(std::vector<CIwAttrInstance*>& bases,wxArrayString* classStrings,int indent,bool basic)
{
    CIwAttrInstance* base=bases[0];
    std::vector<CIwAttrData*> Alts;

    int i;
    if (bases.size()<1) return;

    bool AllSameClass=true;
    for (i=1; i<(int)bases.size(); i++)
    {
        if (bases[0]->m_Class!=bases[i]->m_Class)
        {
            AllSameClass=false;
            break;
        }
    }
    //class chooser control
    if (classStrings!=NULL && classStrings->size()>0)
    {
        AddRow(L"Class:",indent);
        m_DataList.push_back(NULL);
        if (!AllSameClass)
            classStrings->Insert(wxT(""),0);

        SetCellRenderer(currRow,1,new wxGridCellChoiceRenderer);
        //SetCellEditor(currRow,1,new CIwAttrGridComboEditor(data,num,alts,*classStrings));
        if (!AllSameClass)
            SetCellValue(L"",currRow,1);
        else
            SetCellValue(base->m_Class->m_Name,currRow,1);
    }
    else
    {
        AddRow(L"class",indent);
        m_DataList.push_back(new DataItem(currRow,1,bases));

        SetAttr(currRow,0,m_Sect->Clone());
        SetAttr(currRow,1,m_Sect->Clone());
        SetAttr(currRow,2,m_Sect->Clone());
        SetAttr(currRow,3,m_Sect->Clone());
        SetAttr(currRow,4,m_Sect->Clone());

        SetCellValue(base->m_Class->m_Name,currRow,1);
        SetCellSize(currRow,1,1,3);
    }

    if (!AllSameClass) return;

    for (i=0; i<(int)base->m_Data.size(); i++)
    {
        if (base->m_Data[i]->m_Member->m_Priority==0 && basic) continue;

        if (base->m_Data[i]->m_Instance->m_Dialog!=NULL && base->m_Data[i]->m_Instance->m_Dialog->GridOverrideCheck(base->m_Data[i])) continue;

        if (!m_Mgr.m_IsExporter && (base->m_Data[i]->m_Member->m_Type&ATTRMEMBER_MASK)==ATTRMEMBER_CHILD) continue;

        if (m_Mgr.m_IsWizard && base->m_Data[i]->m_Member->m_Wizard.empty()) continue;

        if (bases.size()>1)
            if (!m_Mgr.FindAlts(bases,Alts,base->m_Data[i]))
                continue;

        int start=currRow+1;
        AddData(base->m_Data[i],-1,1,Alts,indent,basic);

        if ((base->m_Data[i]->m_Member->m_Type&ATTRMEMBER_0ORMORE)!=0)
            MakeButton(base,L"Remove",start,4);
    }
    if (basic) return;

    wxArrayString Strings;
    std::vector<CIwAttrClass*> Classes;
    base->GetChildClasses(Classes,base->m_Class);
    m_Mgr.GetDerivedClasses(Classes);
    m_Mgr.GetClassStrings(Classes,Strings);
    base->GetAddStrings(Strings,base->m_Class,(base->m_Class->m_Flags&ATTRCLASS_ORDERED_F)!=0);
    if (Strings.size()>0)
    {
        AddRow(L"Add:",indent);
        m_DataList.push_back(new DataItem(currRow,indent,bases));
        SetCellRenderer(currRow,1,new wxGridCellChoiceRenderer);
        SetCellEditor(currRow,1,new CIwAttrGridComboEditor(Strings));
        SetCellValue(Strings[0],currRow,1);
        SetCellSize(currRow,1,1,3);

        MakeButton(base,L"Add",currRow,4);
    }
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::AddRow(wxString name,int indent)
{
    currRow=GetNumberRows();
    AppendRows();

    wxGridCellAttr* attr;
    if ((currRow%2)==0)
        attr=m_Even;
    else
        attr=m_Odd;

    EnableDragRowSize(false);
    SetCellValue(name,currRow,0);
    SetAttr(currRow,0,attr->Clone());
    SetAttr(currRow,1,attr->Clone());
    SetAttr(currRow,2,attr->Clone());
    SetAttr(currRow,3,attr->Clone());
    SetAttr(currRow,4,attr->Clone());
    SetReadOnly(currRow,0);
    SetReadOnly(currRow,3);
    SetReadOnly(currRow,4);
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::AddData(CIwAttrData* origData,int num,int offset,std::vector<CIwAttrData*>& alts,int indent,bool basic)
{
    int origNum=num,firstRow=0;
    int i,j,k;
    wxArrayString strings;
    std::vector<CIwAttrClass*> Classes;
    CIwAttrData* data=origData;
    bool readOnly=false;
    wxBitmap* bmp=NULL;
    bool shouldHave;

    if (origData->m_Instance->m_Dialog!=NULL)
    {
        data=origData->m_Instance->m_Dialog->OverrideDataForGrid(origData,true);
        if (data==NULL)
            data=origData;
        else if (data!=origData)
            readOnly=true;
    }

    if (!basic)
    {
        for (CIwAttrData* group=origData->m_Group; group!=NULL; group=group->m_Group)
        {
            if (group->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)
                return;

            indent++;
        }
    }

    if (num==-1 && data->m_Member->m_Type&ATTRMEMBER_LIST && (data->m_Member->m_Type&ATTRMEMBER_MASK)!=ATTRMEMBER_INT124)
    {
        if (data->m_Items.empty())
        {
            AddRow(data->m_Member->m_Name,indent);
            m_DataList.push_back(new DataItem(-1,indent,origData,alts));

            SetCellRenderer(currRow,offset,new wxGridCellButtonRenderer);
            SetCellValue(L"Add new "+data->m_Member->m_Name,currRow,offset);
            SetCellSize(currRow,offset,1,3);
            SetReadOnly(currRow,offset);

            if (origData->m_Instance->m_Dialog!=NULL)
                origData->m_Instance->m_Dialog->GridOverride(this,currRow,currRow,origData);

            return;
        }

        AddRow(data->m_Member->m_Name,indent);
        firstRow=currRow;
        m_DataList.push_back(new DataItem(-2,indent,origData,alts));
        SetAttr(currRow,0,m_Sect->Clone());
        SetAttr(currRow,1,m_Sect->Clone());
        SetAttr(currRow,2,m_Sect->Clone());
        SetAttr(currRow,3,m_Sect->Clone());
        SetAttr(currRow,4,m_Sect->Clone());

        if ((data->m_Items[0].m_Flags&ATTRITEM_HIDDEN_F)==0)
        {
            for (i=0; i<(int)data->m_Items.size(); i++)
            {
                AddRow(wxString::Format(L"<item %d>",i),indent+1);
                m_DataList.push_back(new DataItem(i,indent+1,origData,alts));

                int start=currRow;
                AddData(origData,i,offset,alts,indent+1,basic);

                MakeButton(data->m_Instance,L"Remove",start,offset+3);
            }
            AddRow(L"",indent+1);
            m_DataList.push_back(new DataItem(-1,indent+1,origData,alts));

            SetCellRenderer(currRow,offset,new wxGridCellButtonRenderer);
            SetCellValue(L"Add new "+data->m_Member->m_Name,currRow,offset);
            SetCellSize(currRow,offset,1,3);
            SetReadOnly(currRow,offset);
        }

        if (origData->m_Instance->m_Dialog!=NULL)
            origData->m_Instance->m_Dialog->GridOverride(this,firstRow,currRow,origData);

        return;
    }

    if (num==-1)
    {
        AddRow(data->m_Member->m_Name,indent);
        firstRow=currRow;
        m_DataList.push_back(new DataItem(0,indent,origData,alts));
    }

    if (num==-1 && data->m_Items.empty())
    {
        SetCellSize(currRow,offset,1,3);
        SetReadOnly(currRow,offset);
        if (origData->m_Instance->m_Dialog!=NULL)
            origData->m_Instance->m_Dialog->GridOverride(this,currRow,currRow,origData);

        return;
    }

    if (num==-1 && data->m_Member->m_Type&ATTRMEMBER_ARRAY)  //make a set of controls for a list item
    {
        for (i=0,k=1; i<data->m_Member->m_ArraySize; i++,k++)
        {
            if (k==3)
            {
                AddRow(data->m_Member->m_Name,indent);
                m_DataList.push_back(new DataItem(i,indent,origData,alts));
                k=1;
            }

            AddData(origData,i,k,alts,indent,basic);
        }
        if (k==2)
            SetReadOnly(currRow,3);

        if (origData->m_Instance->m_Dialog!=NULL)
            origData->m_Instance->m_Dialog->GridOverride(this,firstRow,currRow,origData);

        return;
    }

    if (num==-1)
        num=0;

    switch (data->m_Member->m_Type&ATTRMEMBER_MASK)
    {
    case ATTRMEMBER_BOOL:
        SetCellRenderer(currRow,offset,new wxGridCellChoiceRenderer);

        for (j=0; j<(int)alts.size(); j++)
        {
            if (data->m_Items[num].m_Int!=alts[j]->m_Items[num].m_Int)
                break;
        }
        if (j==(int)alts.size())
        {
            SetCellEditor(currRow,offset,new CIwAttrGridComboEditor(data,num,alts,CIwAttrGridComboEditor::MODE_BOOL));
            SetCellValue((data->m_Items[num].m_Int==1) ? L"true" : L"false",currRow,offset);
        }
        else
        {
            SetCellEditor(currRow,offset,new CIwAttrGridComboEditor(data,num,alts,CIwAttrGridComboEditor::MODE_3STATE));
            SetCellValue(L"<mixed>",currRow,offset);
        }

        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetReadOnly(currRow,offset+1);

        break;
    case ATTRMEMBER_BYTE:
        SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        if (data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,0,255));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_UInt),currRow,offset);
        }
        else
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,-128,127));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_Int),currRow,offset);
        }

        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetReadOnly(currRow,offset+1);

        break;
    case ATTRMEMBER_SHORT:
        SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        if (data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,0,65535));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_UInt),currRow,offset);
        }
        else
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,-32768,32767));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_Int),currRow,offset);
        }

        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetCellSize(currRow,offset,1,2);

        break;
    case ATTRMEMBER_INT:
        SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        if (data->m_Member->m_Type&ATTRMEMBER_UNSIGNED)
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,0,0xffffffff));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_UInt),currRow,offset);
        }
        else
        {
            if (data->m_Member->m_Limits[2]!=0)
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,
                                                                         data->m_Member->m_Limits[0]/data->m_Member->m_Limits[2],data->m_Member->m_Limits[1]/data->m_Member->m_Limits[2]));
            else
                SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,num,alts,0x80000000,0x7fffffff));

            SetCellValue(wxString::Format(L"%d",data->m_Items[num].m_Int),currRow,offset);
        }

        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetCellSize(currRow,offset,1,2);

        break;
    case ATTRMEMBER_INT124:
        SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,0,alts,0x80000000,0x7fffffff));
        SetCellValue(wxString::Format(L"%d",data->m_Items[0].m_Int),currRow,offset);
        if (data->m_Items.size()==1)
            SetCellSize(currRow,offset,1,2);
        else if (data->m_Items.size()==2)
        {
            SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
            SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,1,alts,0x80000000,0x7fffffff));
            SetCellValue(wxString::Format(L"%d",data->m_Items[1].m_Int),currRow,offset+1);
        }
        else
        {
            SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
            SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,1,alts,0x80000000,0x7fffffff));
            SetCellValue(wxString::Format(L"%d",data->m_Items[1].m_Int),currRow,offset+1);

            AddRow(data->m_Member->m_Name,indent);
            m_DataList.push_back(new DataItem(2,indent,origData,alts));

            SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
            SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,2,alts,0x80000000,0x7fffffff));
            SetCellValue(wxString::Format(L"%d",data->m_Items[2].m_Int),currRow,offset);

            if (data->m_Items.size()>3)
            {
                SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
                SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,3,alts,0x80000000,0x7fffffff));
                SetCellValue(wxString::Format(L"%d",data->m_Items[3].m_Int),currRow,offset+1);
            }
            else
                SetReadOnly(currRow,offset+1);
        }

        MakeButton(data->m_Instance,L"1->2->4",currRow,offset+2);
        break;
    case ATTRMEMBER_FLOAT:
        SetCellRenderer(currRow,offset,new wxGridCellFloatRenderer);
        SetCellEditor(currRow,offset,new CIwAttrGridFloatEditor(data,num,alts));
        SetCellValue(wxString::Format(L"%f",data->m_Items[num].m_Float),currRow,offset);
        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetCellSize(currRow,offset,1,2);

        break;
    case ATTRMEMBER_FILE:
    case ATTRMEMBER_DATA:
    case ATTRMEMBER_STRING:
        SetCellEditor(currRow,offset,new CIwAttrGridTextEditor(data,num,alts));
        SetCellValue(data->m_Items[num].m_String,currRow,offset);
        if ((data->m_Member->m_Type&ATTRMEMBER_ARRAY)!=ATTRMEMBER_ARRAY)
            SetCellSize(currRow,offset,1,3);

        break;
    case ATTRMEMBER_USETEMPLATE:
        SetCellEditor(currRow,offset,new CIwAttrGridTextEditor(data,0,alts));
        SetCellEditor(currRow,offset+1,new CIwAttrGridTextEditor(data,1,alts));
        SetCellValue(data->m_Items[0].m_String,currRow,offset);
        SetCellValue(data->m_Items[1].m_String,currRow,offset+1);
        break;
    case ATTRMEMBER_COLOUR:
        //SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,0,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[0].m_Int),currRow,offset);
        //SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,1,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[1].m_Int),currRow,offset+1);
        //SetCellRenderer(currRow,offset+2,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset+2,new CIwAttrGridNumberEditor(data,2,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[2].m_Int),currRow,offset+2);
        SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
        SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,3,alts,0,255));
        SetCellValue(wxString::Format(L"%d",data->m_Items[3].m_Int),currRow,offset+1);

        SetCellRenderer(currRow,offset,new wxGridCellButtonRenderer(data));
        SetCellValue(L"Colour...",currRow,offset);
        break;
    case ATTRMEMBER_COLOUR3:
        //SetCellRenderer(currRow,offset,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset,new CIwAttrGridNumberEditor(data,0,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[0].m_Int),currRow,offset);
        //SetCellRenderer(currRow,offset+1,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset+1,new CIwAttrGridNumberEditor(data,1,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[1].m_Int),currRow,offset+1);
        //SetCellRenderer(currRow,offset+2,new wxGridCellNumberRenderer);
        //SetCellEditor(currRow,offset+2,new CIwAttrGridNumberEditor(data,2,alts,0,255));
        //SetCellValue(wxString::Format(L"%d",data->m_Items[2].m_Int),currRow,offset+2);

        SetCellRenderer(currRow,offset,new wxGridCellButtonRenderer(data));
        SetCellValue(L"Colour...",currRow,offset);
        SetReadOnly(currRow,offset+1);
        break;
    case ATTRMEMBER_ENUM:
        for (i=1; i<(int)data->m_Member->m_Text.size(); i++)
        {
            strings.Add(data->m_Member->m_Text[i]);
        }
        for (i=0; i<(int)alts.size(); i++)
        {
            if (alts[i]->m_Items[num].m_Int!=data->m_Items[num].m_Int)
            {
                strings.Insert(wxT(""),0);
                break;
            }
        }
        SetCellRenderer(currRow,offset,new wxGridCellChoiceRenderer);
        SetCellEditor(currRow,offset,new CIwAttrGridComboEditor(data,num,alts,strings,CIwAttrGridComboEditor::MODE_ENUM));
        SetCellValue(data->m_Member->m_Text[data->m_Items[num].m_Int+1],currRow,offset);
        SetCellSize(currRow,offset,1,2);
        break;
    case ATTRMEMBER_GROUP:
        SetAttr(currRow,0,m_Sect->Clone());
        SetAttr(currRow,1,m_Sect->Clone());
        SetAttr(currRow,2,m_Sect->Clone());
        SetAttr(currRow,3,m_Sect->Clone());
        SetAttr(currRow,4,m_Sect->Clone());
        break;
    case ATTRMEMBER_FILETEXTURE:
        SetCellEditor(currRow,offset,new CIwAttrGridTextEditor(data,num,alts));
        SetCellValue(data->m_Items[num].m_String,currRow,offset);

        if (data->m_Instance->m_File!=NULL)
            bmp=data->m_Instance->m_File->GetBitmap(data,ATTRSTRING_FILETEXTURE);

        SetCellRenderer(currRow,offset+1,new wxGridCellTextureRenderer(bmp));
        SetReadOnly(currRow,offset+1);

        MakeButton(data->m_Instance,L"Change",currRow,offset+2);
        SetReadOnly(currRow,offset+2);
        break;
    case ATTRMEMBER_FILEFONT:
        SetCellEditor(currRow,offset,new CIwAttrGridTextEditor(data,num,alts));
        SetCellValue(data->m_Items[num].m_String,currRow,offset);

        if (data->m_Instance->m_File!=NULL)
            bmp=data->m_Instance->m_File->GetBitmap(data,ATTRSTRING_FILEFONT);

        SetCellRenderer(currRow,offset+1,new wxGridCellTextureRenderer(bmp));
        SetReadOnly(currRow,offset+1);

        MakeButton(data->m_Instance,L"Change",currRow,offset+2);
        SetReadOnly(currRow,offset+2);
        break;
    case ATTRMEMBER_PTR:
        SetCellEditor(currRow,offset,new CIwAttrGridTextEditor(data,num,alts));
        SetCellValue(data->m_Items[num].m_String,currRow,offset);

        if (data->m_Instance->m_File!=NULL)
            bmp=data->m_Instance->m_File->GetBitmap(data,ATTRSTRING_PTR,&shouldHave);

        if (shouldHave)
        {
            SetCellRenderer(currRow,offset+1,new wxGridCellTextureRenderer(bmp));
            SetReadOnly(currRow,offset+1);
        }
        else
            SetCellSize(currRow,offset,1,2);

        MakeButton(data->m_Instance,L"Change",currRow,offset+2);
        SetReadOnly(currRow,offset+2);

        if (data->m_Items[num].m_Flags&ATTRITEM_ALLOCED_F)
        {
            SetCellValue(L"-= inline =-",currRow,offset);

            if ((data->m_Items[num].m_Flags&ATTRITEM_HIDDEN_F)==0 && data->m_Items[num].m_Inst!=NULL && !m_HideInline)
            {
                std::vector<CIwAttrInstance*> Bases;
                Bases.push_back(data->m_Items[num].m_Inst);
                for (i=0; i<(int)alts.size(); i++)
                {
                    Bases.push_back(alts[i]->m_Items[num].m_Inst);
                }

                Fill2(Bases,NULL,indent+1,basic);
            }
        }

        break;
    case ATTRMEMBER_EXTRACLASS:
        if (data->m_Instance->m_File!=NULL && data->m_Instance->m_File->m_CurrType!=EXPORTTYPE_NUM)
            m_Mgr.GetClasses(Classes,data->m_Instance->m_File->m_CurrType);
        else
            m_Mgr.GetClasses(Classes,EXPORTTYPE_GUI);

        m_Mgr.GetClassStrings(Classes,strings);

        SetCellRenderer(currRow,offset,new wxGridCellChoiceRenderer);
        SetCellEditor(currRow,offset,new CIwAttrGridComboEditor(data,num,alts,strings,CIwAttrGridComboEditor::MODE_CLASSES));
        SetCellValue(data->m_Items[num].m_Class->m_Name,currRow,offset);
        SetCellSize(currRow,offset,1,2);
        break;
    case ATTRMEMBER_CLASS:
        SetAttr(currRow,0,m_Sect->Clone());
        SetAttr(currRow,1,m_Sect->Clone());
        SetAttr(currRow,2,m_Sect->Clone());
        SetAttr(currRow,3,m_Sect->Clone());
        SetAttr(currRow,4,m_Sect->Clone());

        if ((data->m_Items[num].m_Flags&ATTRITEM_HIDDEN_F)==0 && data->m_Items[num].m_Inst!=NULL)
        {
            std::vector<CIwAttrInstance*> Bases;
            Bases.push_back(data->m_Items[num].m_Inst);
            for (i=0; i<(int)alts.size(); i++)
            {
                Bases.push_back(alts[i]->m_Items[num].m_Inst);
            }

            Fill2(Bases,NULL,indent+1,basic);
        }

        break;
    }

    if (origNum==-1 && origData->m_Instance->m_Dialog!=NULL)
        origData->m_Instance->m_Dialog->GridOverride(this,firstRow,currRow,origData);
}

//-----------------------------------------------------------------------------
void CIwAttrGrid::MakeButton(CIwAttrInstance* inst,const wxString& textName,int currRow,int currCol)
{
    wxBitmap* bmp=NULL;
    if (inst->m_File!=NULL)
        bmp=inst->m_File->GetIcon(textName);

    if (bmp!=NULL)
        SetCellRenderer(currRow,currCol,new wxGridCellButtonIconRenderer(bmp));
    else
        SetCellRenderer(currRow,currCol,new wxGridCellButtonRenderer);

    SetCellValue(textName,currRow,currCol);
}

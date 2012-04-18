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
    \file IwWinTreeView.cpp
    \brief Tree Control
 */
//--------------------------------------------------------------------------------

#include "IwUIEdHeader.h"

//--------------------------------------------------------------------------------
BEGIN_EVENT_TABLE(CIwUIEdLayoutFrame, wxPanel)
    EVT_PAINT(OnPaint)
    EVT_LEFT_DOWN(OnLeftDown)
    EVT_LEFT_UP(OnLeftUp)
    EVT_MOTION(OnMotion)
    EVT_LEAVE_WINDOW(OnLeftUp)
    EVT_KEY_DOWN(OnKeyDown)
END_EVENT_TABLE()

#define GET_COL(x,y) wxColour col(m_Image.GetRed(x,y),m_Image.GetGreen(x,y),m_Image.GetBlue(x,y))

//--------------------------------------------------------------------------------
void CIwUIEdFont::Load(const wxString& fileName)
{
    int i;
    wxTextFile fp(fileName);
    if (!fp.Exists() ||!fp.Open())
        return;

    wxFileName name(fileName);
    m_Name=name.GetName();

    for (i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> args;
        if (SuperSplit(fp[i],args,L" \t\n\r")<2)
            continue;

        if (args[0].IsSameAs(L"image",false))
        {
            wxFileName name(fileName);
            name.SetFullName(args[1]);
            m_Image.LoadFile(name.GetFullPath());
        }
        else if (args[0].IsSameAs(L"charmap",false))
            m_CharString=args[1];
        else if (args[0].IsSameAs(L"kernall",false))
            m_KernAll=atoi(args[1].mb_str());
        else if (args[0].IsSameAs(L"kerndef",false))
            m_KernDef=atoi(args[1].mb_str());
        else if (args[0].IsSameAs(L"leading",false))
            m_Leading=atoi(args[1].mb_str());
        else if (args[0].IsSameAs(L"kern",false))
        {
            if (args.size()<3) continue;

            m_Kernings.push_back(Kern(args[1][0],args[1][1],atoi(args[2].mb_str())));
        }
    }
    m_CharString.Replace(L"\\\\",L"\\");

    int x=0,y=0,maxh=0;
    wxColour baseCol(m_Image.GetRed(0,0),m_Image.GetGreen(0,0),m_Image.GetBlue(0,0));

    while (true)
    {
        GET_COL(x,y);
        if (col!=baseCol)
            break;

        x++;
        if (x>=m_Image.GetWidth())
        {
            x=0;
            y++;
        }
    }

    x=0;
    for (i=0; i<(int)m_CharString.size(); i++)
    {
        Char c(m_CharString[i]);
        while (true)
        {
            GET_COL(x,y);
            if (col!=baseCol)
                break;

            x++;
            if (x>=m_Image.GetWidth())
            {
                x=1;
                y+=maxh;
                while (true)
                {
                    GET_COL(x,y);
                    if (col!=baseCol)
                        break;

                    y++;
                    if (y>=m_Image.GetHeight())
                    {
                        wxLogWarning(L"Problem with font file %s",fileName);
                        return;
                    }
                }
                break;
            }
        }
        for (c.w=0;; c.w++)
        {
            GET_COL(x+c.w,y);
            if (col==baseCol)
                break;

            c.w++;
            if (x+c.w>=m_Image.GetWidth()-1)
                break;
        }
        for (c.h=0;; c.h++)
        {
            GET_COL(x,y+c.h);
            if (col==baseCol)
                break;

            c.h++;
            if (y+c.h>=m_Image.GetHeight()-1)
                break;
        }
        if (c.h>maxh) maxh=c.h;

        c.x=x;
        c.y=y;
        x+=c.w;


        m_CharMap.push_back(c);
    }
}

//--------------------------------------------------------------------------------
int CIwUIEdFont::Find(wxChar c,wxChar prev,int& x,int& y,int& depth)
{
    if (c=='\n')
    {
        y+=depth+m_Leading;
        x=0;
        return -1;
    }

    for (int i=0; i<(int)m_CharMap.size(); i++)
    {
        if (m_CharMap[i].c==c)
        {
            int kern=m_KernDef;
            for (int j=0; j<(int)m_Kernings.size(); j++)
            {
                if (m_Kernings[j].from==prev && m_Kernings[j].to==c)
                    kern=m_Kernings[j].amount;
            }
            kern+=m_KernAll;
            x+=kern;
            if (x<0) x=0;

            if (depth<m_CharMap[i].h)
                depth=m_CharMap[i].h;

            return i;
        }
    }
    return -1;
}

//--------------------------------------------------------------------------------
void CIwUIEdFont::Draw(int num,int x,int y,wxImage& image,wxColour col)
{
    for (int j=0; j<(int)m_CharMap[num].h; j++)
    {
        for (int i=0; i<(int)m_CharMap[num].w; i++)
        {
            int fx=m_CharMap[num].x+i;
            int fy=m_CharMap[num].y+j;
            int icol[4]={ m_Image.GetRed(fx,fy),m_Image.GetGreen(fx,fy),m_Image.GetBlue(fx,fy),m_Image.GetAlpha(fx,fy) };
            int scol[4]={ image.GetRed(x+i,y+j),image.GetGreen(x+i,y+j),image.GetBlue(x+i,y+j),image.GetAlpha(x+i,y+j) };

            icol[0]=icol[0]*col.Red()/255;
            icol[1]=icol[1]*col.Green()/255;
            icol[2]=icol[2]*col.Blue()/255;

            if (icol[3]>=scol[3])
            {
                image.SetRGB(x+i,y+j,icol[0],icol[1],icol[2]);
                image.SetAlpha(x+i,y+j,icol[3]);
            }
        }
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdFont::RenderText(const wxString& text,wxBitmap& bmp,wxColour col)
{
    int i,x=0,y=0,depth=0,width=0;
    wxChar prev=0;

    if (text.empty())
    {
        wxImage image(1,1);
        unsigned char* alpha=(unsigned char*)malloc(1);
        memset(alpha,0,1);
        image.SetAlpha(alpha);

        bmp=wxBitmap(image);
        return;
    }

    for (i=0; i<(int)text.size(); i++)
    {
        int num=Find(text[i],prev,x,y,depth);
        if (num!=-1)
            x+=m_CharMap[num].w;

        if (width<x)
            width=x;

        prev=text[i];
    }
    y+=depth;

    wxImage image(width,y);
    unsigned char* alpha=(unsigned char*)malloc(width*y);
    memset(alpha,0,width*y);
    image.SetAlpha(alpha);

    x=0; y=0; depth=0; prev=0;
    for (i=0; i<(int)text.size(); i++)
    {
        int num=Find(text[i],prev,x,y,depth);
        if (num!=-1)
        {
            Draw(num,x,y,image,col);

            x+=m_CharMap[num].w;
        }

        prev=text[i];
    }
    bmp=wxBitmap(image);
}

//--------------------------------------------------------------------------------
int CIwUIEdLayoutItem::Index()
{
    if (m_Parent==NULL) return -1;

    for (int i=0; i<(int)m_Parent->m_Children.size(); i++)
    {
        if (m_Parent->m_Children[i]==this) return i;
    }
    return -1;
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutFrame::CIwUIEdLayoutFrame(wxWindow* parent,CIwASDUIEdAttrPanel* Frame) :
    wxPanel(parent,wxID_ANY,wxPoint(-1,-1),wxSize(710,340),wxNO_BORDER|wxWANTS_CHARS),m_Curr(NULL),m_ScreenSize(480,320),m_Border(10),m_Frame(Frame),
    m_DragStartX(-1),m_DragStartY(-1),m_DragStart2X(-1),m_DragStart2Y(-1),m_ChooseSize(200,320),m_CurrChoose(NULL),m_ChooseScroll(0)
{
    wxString data;
    if (Project.Get(L"guideffont",data))
    {
        m_Fonts.push_back(CIwUIEdFont(Project.GetPath()+data));
        m_Fonts[0].m_Name=L"<debug>";
    }

    // load the project specific settings from uimetatemplate.txt
    if (Project.GetFile(L"data",data))
    {
        data+=L"uimetatemplate.txt";

        if (wxFileName::FileExists(data))
            LoadTemplate(data);
        else if (Project.GetFile(L"viewerdata",data))
        {
            data+=L"uimetatemplate.txt";

            if (wxFileName::FileExists(data))
                LoadTemplate(data);
        }
    }

    SetupRoot();

    // load the project specific settings from uipalette.ui
    if (Project.GetFile(L"data",data))
    {
        data+=L"uipalette.ui";

        if (wxFileName::FileExists(data))
            LoadPalette(data);
        else if (Project.GetFile(L"viewerdata",data))
        {
            data+=L"uipalette.ui";

            if (wxFileName::FileExists(data))
                LoadPalette(data);
        }
    }

    ResetSize();
    ResetPaletteSize();
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::LoadPalette(const wxString& filename)
{
    CIwASDFileDataAttr::Group* group=new CIwASDFileDataAttr::Group;
    m_Choose.m_Inst=CIwTheFileMetaMgr.GetFromFile(filename,group);

    m_Choose.m_Children.clear();
    Setup(m_Choose.m_Inst,&m_Choose,true);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::SetupRoot()
{
    CIwUIEdLayoutItemDef* catDef=new CIwUIEdLayoutItemDef(L"CIwUIElement",DISPLAY_NONE);
    catDef->m_Content=L"!name";
    catDef->m_Font=L"<debug>";
    catDef->m_Colour=L"!colour";
    m_Defs.push_back(catDef);

    m_List.m_Frame=this;
    m_List.m_Name=L"root";
    m_Choose.m_Frame=this;
    m_List.m_Flags=CIwUIEdLayoutItem::INVISIBLE;
    m_Choose.m_Flags=CIwUIEdLayoutItem::INVISIBLE;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::ResetSize()
{
    m_Choose.x=m_Border+m_ScreenSize.x;
    m_Choose.y=0;
    m_Choose.x1=m_Border+m_ScreenSize.x+m_ChooseSize.x;
    m_Choose.y1=m_ChooseSize.y;

    m_List.x=0;
    m_List.y=0;
    m_List.x1=m_ScreenSize.x;
    m_List.y1=m_ScreenSize.y;
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem::CIwUIEdLayoutItem(CIwUIEdLayoutItem* parent,CIwUIEdLayoutItemDef* def,bool palette,CIwAttrInstance* inst) : m_Colour(0,0,0),
    x(0),y(0),x1(1),y1(1),sel(0),m_Font(NULL),m_Parent(parent),m_Flags(palette ? HOVERING : NONE),m_Inst(inst),m_Def(def),m_Frame(parent->m_Frame)
{
    m_Parent->m_Children.push_back(this);

    if (m_Def==NULL) return;

    m_Name=m_Def->m_Name;
    if (m_Inst!=NULL)
    {
        m_Flags|=ROOT_ELEMENT;
        CIwAttrData* name=m_Inst->FindData(L"name",CIwAttrInstance::FINDMODE_EXPORTERTAG);
        if (name!=NULL && !name->m_Items.empty())
            m_Name=name->m_Items[0].m_String;
    }
    else if (m_Parent!=NULL)
        m_Inst=m_Parent->m_Inst;

    SetLocation(palette);

    switch (m_Def->m_Type)
    {
    case DISPLAY_FILLEDBOX :
    case DISPLAY_LINEBOX:
    case DISPLAY_LINE:
        SetColour();
        break;
    case DISPLAY_NONE:
    case DISPLAY_TEXT:
        SetColour();
        SetText();
        break;
    case DISPLAY_BITMAP:
        SetBitmap();
        break;
    }
    for (int i=0; i<(int)m_Def->m_Children.size(); i++)
    {
        new CIwUIEdLayoutItem(this,m_Def->m_Children[i],palette);
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SetColour()
{
    if (m_Inst!=NULL && !m_Def->m_Colour.empty() && m_Def->m_Colour[0]=='!')
    {
        CIwAttrData* data=m_Inst->FindData(m_Def->m_Colour.Mid(1),0);
        if (data!=NULL)
        {
            switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
            {
            case ATTRMEMBER_BYTE:
            case ATTRMEMBER_SHORT:
            case ATTRMEMBER_INT:
            case ATTRMEMBER_COLOUR:
                if (data->m_Items.size()>=3)
                    m_Colour=wxColour(data->m_Items[0].m_Int,data->m_Items[1].m_Int,data->m_Items[2].m_Int);

                break;
            case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
                if (data->m_Items.size()>=3)
                    m_Colour=wxColour(data->m_Items[0].m_UInt,data->m_Items[1].m_UInt,data->m_Items[2].m_UInt);

                break;
            case ATTRMEMBER_FLOAT:
                if (data->m_Items.size()>=3)
                    m_Colour=wxColour(data->m_Items[0].m_Float*255,data->m_Items[1].m_Float*255,data->m_Items[2].m_Float*255);

                break;
            }
        }
    }
    else if (!m_Def->m_Colour.empty())
        m_Colour=wxColour(m_Def->m_Colour);
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutItem::Find(const wxString& name)
{
    if (m_Name.IsSameAs(name,false))
        return this;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        CIwUIEdLayoutItem* item=m_Children[i]->Find(name);
        if (item!=NULL)
            return item;
    }
    return NULL;
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutItem::Find2(const wxString& name)
{
    CIwUIEdLayoutItem* item=Find(name);
    if (item!=NULL)
        return item;

    if (m_Parent==NULL)
        return NULL;

    return m_Parent->Find2(name);
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutItem::Find(CIwUIEdLayoutItemDef* def)
{
    if (m_Def==def)
        return this;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        CIwUIEdLayoutItem* item=m_Children[i]->Find(def);
        if (item!=NULL)
            return item;
    }
    return NULL;
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutItem::Find2(CIwUIEdLayoutItemDef* def)
{
    CIwUIEdLayoutItem* item=Find(def);
    if (item!=NULL)
        return item;

    if (m_Parent==NULL)
        return NULL;

    return m_Parent->Find2(def);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SetLocation(bool palette)
{
    CIwAttrData* data;

    if (m_Def==NULL) return;

    x1=y1=0;
    if (m_Inst!=NULL)
    {
        data=m_Inst->FindData(L"size");
        if (data!=NULL)
        {
            switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
            {
            case ATTRMEMBER_BYTE:
            case ATTRMEMBER_SHORT:
            case ATTRMEMBER_INT:
                if (data->m_Items.size()>=2)
                {
                    x1=data->m_Items[0].m_Int;
                    y1=data->m_Items[1].m_Int;
                }

                break;
            case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
                if (data->m_Items.size()>=2)
                {
                    x1=data->m_Items[0].m_UInt;
                    y1=data->m_Items[1].m_UInt;
                }

                break;
            case ATTRMEMBER_FLOAT:
                if (data->m_Items.size()>=2)
                {
                    x1=data->m_Items[0].m_Float;
                    y1=data->m_Items[1].m_Float;
                }

                break;
            }
        }
    }
    else
    {
        x1=m_Def->m_Size[0];
        y1=m_Def->m_Size[1];
    }

    if (palette)
        return;

    x=y=0;
    if (m_Inst!=NULL)
    {
        data=m_Inst->FindData(L"pos");
        if (data!=NULL)
        {
            switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
            {
            case ATTRMEMBER_BYTE:
            case ATTRMEMBER_SHORT:
            case ATTRMEMBER_INT:
                if (data->m_Items.size()>=2)
                {
                    x=data->m_Items[0].m_Int;
                    y=data->m_Items[1].m_Int;
                }

                break;
            case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
                if (data->m_Items.size()>=2)
                {
                    x=data->m_Items[0].m_UInt;
                    y=data->m_Items[1].m_UInt;
                }

                break;
            case ATTRMEMBER_FLOAT:
                if (data->m_Items.size()>=2)
                {
                    x=data->m_Items[0].m_Float;
                    y=data->m_Items[1].m_Float;
                }

                break;
            }
        }
    }
    else
    {
        x=m_Def->m_Pos[0];
        y=m_Def->m_Pos[1];
    }

    x1+=x;
    y1+=y;

    rot=0;
    if (m_Inst!=NULL)
    {
        data=m_Inst->FindData(L"rotation");
        if (data!=NULL)
        {
            switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
            {
            case ATTRMEMBER_BYTE:
            case ATTRMEMBER_SHORT:
            case ATTRMEMBER_INT:
                rot=(data->m_Items[0].m_Int/4096.0f)*M_PI;
                break;
            case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
            case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
                rot=(data->m_Items[0].m_UInt/4096.0f)*M_PI;
                break;
            case ATTRMEMBER_FLOAT:
                rot=data->m_Items[0].m_Float;
                break;
            }
        }
    }
    else
        rot=m_Def->m_Rotation;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::GetLocation()
{
    CIwAttrData* data;

    if (m_Def==NULL || m_Inst==NULL || (m_Flags&ROOT_ELEMENT)==0) return;

    data=m_Inst->FindData(L"size");
    if (data!=NULL)
    {
        switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
        {
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_Int=x1-x;
                data->m_Items[1].m_Int=y1-y;
            }

            break;
        case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
        case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
        case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_UInt=x1-x;
                data->m_Items[1].m_UInt=y1-y;
            }

            break;
        case ATTRMEMBER_FLOAT:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_Float=x1-x;
                data->m_Items[1].m_Float=y1-y;
            }

            break;
        }
    }

    data=m_Inst->FindData(L"pos");
    if (data!=NULL)
    {
        switch (data->m_Member->m_Type&(ATTRMEMBER_MASK|ATTRMEMBER_UNSIGNED))
        {
        case ATTRMEMBER_BYTE:
        case ATTRMEMBER_SHORT:
        case ATTRMEMBER_INT:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_Int=x;
                data->m_Items[1].m_Int=y;
            }

            break;
        case ATTRMEMBER_BYTE|ATTRMEMBER_UNSIGNED:
        case ATTRMEMBER_SHORT|ATTRMEMBER_UNSIGNED:
        case ATTRMEMBER_INT|ATTRMEMBER_UNSIGNED:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_UInt=x;
                data->m_Items[1].m_UInt=y;
            }

            break;
        case ATTRMEMBER_FLOAT:
            if (data->m_Items.size()>=2)
            {
                data->m_Items[0].m_Float=x;
                data->m_Items[1].m_Float=y;
            }

            break;
        }
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SetDefaultSize()
{
    if (m_Bitmap.IsOk())
    {
        x1=x+m_Bitmap.GetWidth();
        y1=y+m_Bitmap.GetHeight();
        GetLocation();
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::RenderText(const wxString& text)
{
    if (m_Font==NULL) return;

    m_Font->RenderText(text,m_Bitmap,m_Colour);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SetText()
{
    wxString text;
    wxString font;

    if (m_Inst!=NULL && !m_Def->m_Font.empty() && m_Def->m_Font[0]=='!')
    {
        CIwAttrData* data=m_Inst->FindData(m_Def->m_Font.Mid(1),0);
        if (data!=NULL && !data->m_Items.empty())
            font=data->m_Items[0].m_String;
    }
    else
        font=m_Def->m_Font;

    if (m_Frame!=NULL && !m_Frame->m_Fonts.empty())
    {
        m_Font=&m_Frame->m_Fonts[0];
        for (int i=0; i<(int)m_Frame->m_Fonts.size(); i++)
        {
            if (m_Frame->m_Fonts[i].m_Name.IsSameAs(font,false))
            {
                m_Font=&m_Frame->m_Fonts[i];
                break;
            }
        }
    }

    if (m_Inst!=NULL && !m_Def->m_Content.empty() && m_Def->m_Content[0]=='!')
    {
        CIwAttrData* data=m_Inst->FindData(m_Def->m_Content.Mid(1),0);
        if (data!=NULL && !data->m_Items.empty())
            text=data->m_Items[0].m_String;
    }
    else
        text=m_Def->m_Content;

    RenderText(text);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SetBitmap()
{
    wxString mat;
    if (m_Inst!=NULL && !m_Def->m_Content.empty() && m_Def->m_Content[0]=='!')
    {
        CIwAttrData* data=m_Inst->FindData(m_Def->m_Content.Mid(1),0);
        if (data!=NULL && !data->m_Items.empty())
            mat=data->m_Items[0].m_String;
    }
    else
        mat=m_Def->m_Content;

    if (m_Frame!=NULL)
    {
        for (int i=0; i<(int)m_Frame->m_Materials.size(); i++)
        {
            if (m_Frame->m_Materials[i].m_Name.IsSameAs(mat,false))
            {
                if (!m_Frame->m_Materials[i].m_Bitmap.Ok())
                {
                    wxImage image(m_Frame->m_Materials[i].m_Filename);
                    m_Frame->m_Materials[i].m_Bitmap=wxBitmap(image);
                }

                m_Bitmap=m_Frame->m_Materials[i].m_Bitmap;
                break;
            }
        }
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::ResetSize()
{
    if (m_Def!=NULL && m_Def->m_Type==DISPLAY_BITMAP)
        SetBitmap();

    if ((m_Flags&PRESET_SIZE)==0)
        SetLocation(false);

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->ResetSize();
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::ResetPaletteSize()
{
    int y=m_Border;
    for (int i=0; i<(int)m_Choose.m_Children.size(); i++)
    {
        int w=m_Choose.m_Children[i]->x1-m_Choose.m_Children[i]->x;
        int h=m_Choose.m_Children[i]->y1-m_Choose.m_Children[i]->y;

        m_Choose.m_Children[i]->x=m_ScreenSize.x+m_Border*2;
        m_Choose.m_Children[i]->x1=m_ScreenSize.x+m_Border*2+w;
        m_Choose.m_Children[i]->y=y;
        m_Choose.m_Children[i]->y1=y+h;
        m_Choose.m_Children[i]->m_Flags&=~CIwUIEdLayoutItem::ROOT_ELEMENT;
        y+=h+m_Border;

        for (int j=0; j<(int)m_Choose.m_Children[i]->m_Children.size(); j++)
        {
            int w=m_Choose.m_Children[i]->m_Children[j]->x1-m_Choose.m_Children[i]->m_Children[j]->x;
            int h=m_Choose.m_Children[i]->m_Children[j]->y1-m_Choose.m_Children[i]->m_Children[j]->y;

            m_Choose.m_Children[i]->m_Children[j]->x=m_ScreenSize.x+m_Border*3;
            m_Choose.m_Children[i]->m_Children[j]->x1=m_ScreenSize.x+m_Border*3+w;
            m_Choose.m_Children[i]->m_Children[j]->y=y;
            m_Choose.m_Children[i]->m_Children[j]->y1=y+h;
            y+=h+m_Border;
        }
    }
    m_Choose.ResetSize();
}

static wxChar* displayText[]={  //matches CIwUIEdLayoutDisplay
    L"none",
    L"filledbox",
    L"linebox",
    L"line",
    L"text",
    L"bitmap",
    NULL
};
static wxChar* alignFontH[]={   //matches CIwUIEdLayoutAlignFont
    L"left",
    L"mid",
    L"right",
    NULL
};
static wxChar* alignFontV[]={   //matches CIwUIEdLayoutAlignFont
    L"top",
    L"mid",
    L"bottom",
    NULL
};

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::LoadTemplate(const wxString& filename)
{
    wxString Data=L"c:\\temp\\data\\";
    Project.GetFile(L"data",Data);

    wxTextFile fp(filename);
    if (!fp.Exists() || !fp.Open())
        return;

    m_Fonts.clear();
    m_Defs.clear();
    m_Materials.clear();

    CIwUIEdLayoutItemDef* curr=NULL;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> args;
        if (SuperSplit(fp[i],args,L" \t\n\r")<1) continue;

        if (args[0].IsSameAs(L"font",false))
        {
            if (args.size()<2) continue;

            m_Fonts.push_back(CIwUIEdFont(Data+args[1]));
        }
        else if (args[0].IsSameAs(L"element",false))
        {
            if (args.size()<3) continue;

            CIwUIEdLayoutDisplay display=DISPLAY_NONE;
            for (int j=0; displayText[j]!=NULL; j++)
            {
                if (args[2].IsSameAs(displayText[j],false))
                {
                    display=(CIwUIEdLayoutDisplay)j;
                    break;
                }
            }
            CIwUIEdLayoutItemDef* def=new CIwUIEdLayoutItemDef(args[1],display);
            if (curr!=NULL)
            {
                curr->m_Children.push_back(def);
                def->m_Parent=curr;
            }
            else
            {
                m_Defs.push_back(def);
                def->m_Parent=NULL;
            }

            curr=def;
        }
        else if (args[0].IsSameAs(L"xywh",false))
        {
            if (args.size()<5 || curr==NULL) continue;

            curr->m_Pos[0]=atoi(args[1].mb_str());
            curr->m_Pos[1]=atoi(args[2].mb_str());
            curr->m_Size[0]=atoi(args[3].mb_str());
            curr->m_Size[1]=atoi(args[4].mb_str());
        }
        else if (args[0].IsSameAs(L"rotation",false))
        {
            if (args.size()<2 || curr==NULL) continue;

            curr->m_Rotation=atof(args[1].mb_str());
        }
        else if (args[0].IsSameAs(L"category",false))
        {
            if (args.size()<2 || curr==NULL) continue;

            curr->m_Category=args[1];
        }
        else if (args[0].IsSameAs(L"colour",false))
        {
            if (args.size()<2 || curr==NULL) continue;

            curr->m_Colour=args[1];
        }
        else if (args[0].IsSameAs(L"text",false))
        {
            if (args.size()<3 || curr==NULL) continue;

            curr->m_Content=args[1];
            curr->m_Font=args[2];
        }
        else if (args[0].IsSameAs(L"material",false))
        {
            if (args.size()<2) continue;

            if (curr==NULL)
                LoadMaterial(args[1]);
            else
                curr->m_Content=args[1];
        }
        else if (args[0].IsSameAs(L"}",false))
        {
            if (curr==NULL) continue;

            curr=curr->m_Parent;
        }
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::LoadMaterial(const wxString& filename)
{
    wxString Data=L"c:\\temp\\data\\";
    Project.GetFile(L"data",Data);

    wxFileName name(filename);
    wxTextFile fp(Data+filename);
    if (!fp.Exists() || !fp.Open())
        return;

    bool got=false;

    for (int i=0; i<(int)fp.GetLineCount(); i++)
    {
        std::vector<wxString> args;
        if (SuperSplit(fp[i],args,L" \t\n\r")<1) continue;

        if (args[0].IsSameAs(L"name",false))
        {
            if (args.size()<2) continue;

            m_Materials.push_back(CIwUIEdBmp(args[1]));
            got=true;
        }
        else if (args[0].IsSameAs(L"texture0",false))
        {
            if (args.size()<2 && got) continue;

            if (args[1][0]=='//')
                m_Materials.back().m_Filename=Data+args[1];
            else if (args[1][0]=='.')
                m_Materials.back().m_Filename=Data+name.GetPath()+args[1];
            else
                m_Materials.back().m_Filename=args[1];
        }
        else if (args[0].IsSameAs(L"}",false))
            got=false;
    }
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItemDef* CIwUIEdLayoutItemDef::Find(const wxString& name)
{
    if (m_Name.IsSameAs(name,false))
        return this;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        CIwUIEdLayoutItemDef* def=m_Children[i]->Find(name);
        if (def!=NULL) return def;
    }
    return NULL;
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItemDef* CIwUIEdLayoutFrame::FindDef(const wxString& name)
{
    for (int i=0; i<(int)m_Defs.size(); i++)
    {
        CIwUIEdLayoutItemDef* def=m_Defs[i]->Find(name);
        if (def!=NULL) return def;
    }
    return NULL;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::Setup()
{
    Setup(m_List.m_Inst);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::Setup(CIwAttrInstance* inst,CIwUIEdLayoutItem* item,bool palette)
{
    CIwUIEdLayoutItem* item2=NULL;

    for (CIwAttrClass* klass=inst->m_Class; klass!=NULL; klass=klass->m_Parent)
    {
        for (int i=0; i<(int)m_Defs.size(); i++)
        {
            if (klass->m_Name.IsSameAs(m_Defs[i]->m_Name,false))
            {
                item2=new CIwUIEdLayoutItem(item,m_Defs[i],false,inst);
                if (palette)
                    item2->m_Flags|=CIwUIEdLayoutItem::HOVERING|CIwUIEdLayoutItem::PRESET_SIZE;

                break;
            }
        }
        if (item2!=NULL) break;
    }
    if (item2==NULL) item2=item;

    for (int j=0; j<(int)inst->m_Data.size(); j++)
    {
        switch (inst->m_Data[j]->m_Member->m_Type&ATTRMEMBER_MASK)
        {
        case ATTRMEMBER_CHILD:
        case ATTRMEMBER_CLASS:
            Setup(inst->m_Data[j]->m_Items[0].m_Inst,item2,palette);
            break;
        }
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::Setup(CIwAttrInstance* inst)
{
    m_List.m_Children.clear();
    m_List.m_Inst=inst;

    if (inst!=NULL)
        Setup(inst,&m_List);

    ResetPaletteSize();
    Refresh();
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::Changed()
{
    if (m_Curr!=NULL)
    {
        m_Curr->GetLocation();
        m_Frame->ResetProp();
    }

    Refresh(false);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::OnKeyDown(wxKeyEvent& e)
{
    if (m_Curr==NULL) return;

    int val[4]={0,0,0,0};
    bool stick[4]={false,false,false,false};
    if (e.GetKeyCode()==WXK_LEFT)
    {
        switch (m_Curr->sel&3)
        {
        case 1:
            val[0]--;
            val[2]--;
            break;
        case 2:
            val[0]--;
            break;
        case 3:
            val[2]--;
            break;
        }
        Refresh(false);
    }

    if (e.GetKeyCode()==WXK_RIGHT)
    {
        switch (m_Curr->sel&3)
        {
        case 1:
            val[0]++;
            val[2]++;
            break;
        case 2:
            val[0]++;
            break;
        case 3:
            val[2]++;
            break;
        }
        Refresh(false);
    }

    if (e.GetKeyCode()==WXK_UP)
    {
        switch (m_Curr->sel&12)
        {
        case 4:
            val[1]--;
            val[3]--;
            break;
        case 8:
            val[1]--;
            break;
        case 12:
            val[3]--;
            break;
        }
        Refresh(false);
    }

    if (e.GetKeyCode()==WXK_DOWN)
    {
        switch (m_Curr->sel&12)
        {
        case 4:
            val[1]++;
            val[3]++;
            break;
        case 8:
            val[1]++;
            break;
        case 12:
            val[3]++;
            break;
        }
        Refresh(false);
    }

    if (e.GetKeyCode()=='S')
    {
        m_Curr->SetDefaultSize();
        Refresh(false);
    }

    m_Curr->Move(stick,val);
    if (e.GetKeyCode()==WXK_NUMPAD_RIGHT)
    {
        CIwAttrData* data=m_Curr->m_Inst->m_Parent;
        if (data!=NULL)
        {
            int index=data->Index();
            if (index!=-1 && index<(int)data->m_Instance->m_Data.size()-1)
            {
                data->m_Instance->m_Data.erase(data->m_Instance->m_Data.begin()+index);
                data->m_Instance->m_Data.insert(data->m_Instance->m_Data.begin()+index+1,data);
                m_Frame->Reset();
            }
        }
    }

    if (e.GetKeyCode()==WXK_NUMPAD_LEFT)
    {
        CIwAttrData* data=m_Curr->m_Inst->m_Parent;
        if (data!=NULL)
        {
            int index=data->Index();
            if (index!=-1 && index>0)
            {
                data->m_Instance->m_Data.erase(data->m_Instance->m_Data.begin()+index);
                data->m_Instance->m_Data.insert(data->m_Instance->m_Data.begin()+index-1,data);
                m_Frame->Reset();
            }
        }
    }

    if (e.GetKeyCode()==WXK_NUMPAD_UP)
    {
        CIwAttrData* data=m_Curr->m_Inst->m_Parent;
        if (data!=NULL)
        {
            int index=data->Index();
            if (index!=-1 && data->m_Instance->m_Parent!=NULL)
            {
                data->m_Instance->m_Data.erase(data->m_Instance->m_Data.begin()+index);
                data->m_Instance->m_Parent->m_Instance->m_Data.push_back(data);
                data->m_Instance=data->m_Instance->m_Parent->m_Instance;
                m_Frame->Reset();
            }
        }
    }

    if (e.GetKeyCode()==WXK_NUMPAD_DOWN)
    {
        CIwAttrData* data=m_Curr->m_Inst->m_Parent;
        if (data!=NULL)
        {
            int index=data->Index();
            if (index!=-1)
            {
                int i;
                CIwAttrData* data2=NULL;

                for (i=index+1; i<(int)data->m_Instance->m_Data.size(); i++)
                {
                    if (data->m_Instance->m_Data[i]->m_Member->m_Name.IsSameAs(L"CIwUIElement",false))
                    {
                        data2=data->m_Instance->m_Data[i];
                        break;
                    }
                }

                if (data2==NULL)
                {
                    for (i=0; i<index; i++)
                    {
                        if (data->m_Instance->m_Data[i]->m_Member->m_Name.IsSameAs(L"CIwUIElement",false))
                        {
                            data2=data->m_Instance->m_Data[i];
                            break;
                        }
                    }
                }

                if (data2!=NULL)
                {
                    data->m_Instance->m_Data.erase(data->m_Instance->m_Data.begin()+index);
                    data2->m_Items[0].m_Inst->m_Data.push_back(data);
                    data->m_Instance=data2->m_Items[0].m_Inst;
                    m_Frame->Reset();
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem::CIwUIEdLayoutItem(CIwUIEdLayoutItem& from) : x(from.x),y(from.y),x1(from.x1),y1(from.y1),sel(from.sel),m_Font(from.m_Font),
    m_Parent(NULL),m_Flags(from.m_Flags),m_Inst(from.m_Inst),m_Def(from.m_Def),m_Frame(from.m_Frame),m_Colour(from.m_Colour),m_Bitmap(from.m_Bitmap)
{
    for (int i=0; i<(int)from.m_Children.size(); i++)
    {
        m_Children.push_back(new CIwUIEdLayoutItem(*from.m_Children[i]));
    }
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutFrame::Create(CIwUIEdLayoutItem* from)
{
    CIwUIEdLayoutItem* item=new CIwUIEdLayoutItem(*from);
    if (m_List.m_Children.empty())
        return NULL;

    CIwUIEdLayoutItem* parent=m_List.m_Children[0];

    parent->m_Children.push_back(item);
    item->m_Parent=parent;
    item->sel=5;

    if (from->m_Inst!=NULL)
    {
        std::vector<CIwAttrNote> notes;
        from->m_Inst->FillNotes(notes);
        item->m_Inst=CIwTheFileMetaMgr.GetFromNotes(notes,EXPORTTYPE_GUI,true);

        CIwAttrData* data=new CIwAttrData;
        data->m_Mgr=parent->m_Inst->m_Mgr;
        data->m_Instance=parent->m_Inst;
        data->m_Member=parent->m_Inst->m_Class->GetClassMember(item->m_Inst->m_Class);
        data->m_Info=-1;
        data->m_Group=NULL;
        data->SetDefault();
        parent->m_Inst->m_Data.push_back(data);
        item->m_Inst->m_Parent=data;

        data->m_Items.resize(1);
        data->m_Items[0].m_Inst=item->m_Inst;
        data->m_Items[0].m_Flags=ATTRITEM_ALLOCED_F;
    }

    m_Selection.clear();
    m_Selection.push_back(item);

    item->GetLocation();
    m_Frame->Reset();

    return item;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::ResetSel()
{
    for (int i=0; i<(int)m_Selection.size(); i++)
    {
        m_Selection[i]->sel=32;
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::OnLeftDown(wxMouseEvent& e)
{
    int x=e.m_x-m_Border;
    int y=e.m_y-m_Border;
    m_DragStartX=x;
    m_DragStartY=y;
    m_DragStart2X=x;
    m_DragStart2Y=y;
    m_Dragging=false;
    m_Moving=false;
    SetFocus();

    if (m_CurrChoose!=NULL)
    {
        m_Curr=Create(m_CurrChoose);
        m_CurrChoose=NULL;
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::OnLeftUp(wxMouseEvent& e)
{
    int i;
    if (m_Curr!=NULL && !m_Dragging)
    {
        if (e.ShiftDown())
        {
            bool found=false;
            for (i=0; i<(int)m_Selection.size(); i++)
            {
                if (m_Selection[i]==m_Curr)
                {
                    m_Selection[i]->sel=0;
                    m_Selection.erase(m_Selection.begin()+i);
                    found=true;
                    break;
                }
            }
            if (!found)
                m_Selection.push_back(m_Curr);
        }
        else
        {
            m_Selection.clear();
            m_Selection.push_back(m_Curr);
        }

        ResetSel();
    }

    if (m_Dragging && !m_Moving)
    {
        m_Selection.clear();
        m_List.GetSelection(m_Selection);
    }

    m_DragStartX=-1;
    m_DragStartY=-1;
    m_DragStart2X=-1;
    m_DragStart2Y=-1;
    m_Dragging=false;
    m_Moving=false;
    if (m_Curr!=NULL && (m_Curr->m_Flags&CIwUIEdLayoutItem::HOVERING)==CIwUIEdLayoutItem::HOVERING)
    {
        if (m_Curr->x>m_ScreenSize.x-2 || m_Curr->y>m_ScreenSize.y-2)
        {
            m_Curr->m_Parent->m_Children.erase(m_Curr->m_Parent->m_Children.begin()+m_Curr->Index());
            m_Curr=NULL;
        }
        else
            m_Curr->m_Flags&=~CIwUIEdLayoutItem::HOVERING;
    }

    m_List.Clear();
    Changed();
}
#define SNAP(v,by) (((v)+(by)/2)/(by)*(by))
#define SNAP2(v,by,bypart) (((v)+(by)/(2*(bypart)))/(by)*(by)-(by)/(2*(bypart)))

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::OnMotion(wxMouseEvent& e)
{
    int x=e.m_x-m_Border;
    int y=e.m_y-m_Border;
    int bw=0,bh=0,i;

    if (e.AltDown() && m_Curr!=NULL && m_Curr->m_Bitmap.IsOk())
    {
        int bwpart=1,bhpart=1;

        bw=m_Curr->m_Bitmap.GetWidth();
        while (bw/bwpart>16 && bw/(bwpart*2)*(bwpart*2)==bw) bwpart*=2;
        bw/=bwpart;
        if (bw<1) bw=1;

        bh=m_Curr->m_Bitmap.GetHeight();
        while (bh/bhpart>16 && bh/(bhpart*2)*(bhpart*2)==bh) bhpart*=2;
        bh/=bhpart;
        if (bh<1) bh=1;
    }

    bool refresh=false;
    bool OnSel=false;
    for (i=0; i<(int)m_Selection.size(); i++)
    {
        if (m_Selection[i]==m_Curr)
        {
            OnSel=true;
            break;
        }
    }

    if (m_DragStartX!=-1 && !m_Dragging)
    {
        if (fabs(x-m_DragStartX)>0)
            m_Dragging=true;

        if (fabs(y-m_DragStartY)>0)
            m_Dragging=true;
    }

    if (!m_Dragging)
    {
        CIwUIEdLayoutItem* last=m_Curr;
        CIwUIEdLayoutItem* lastChoose=m_CurrChoose;
        int lastSel=0;
        if (m_Curr!=NULL)
        {
            lastSel=m_Curr->sel;
            m_Curr->sel=0;
        }

        m_Curr=m_List.Select(x,y);
        ResetSel();
        //m_Curr->Select2(x,y);
        m_CurrChoose=m_Choose.Select(x,y);
        if (m_CurrChoose!=NULL)
            m_CurrChoose->sel=5;

        if (m_Curr!=last || lastChoose!=m_CurrChoose)
            refresh=true;
        else if (m_Curr!=NULL && m_Curr->sel!=lastSel)
            refresh=true;

        if (OnSel && m_Curr!=NULL)
        {
            ResetSel();
            m_Curr->Select2(x,y);
            refresh=true;
        }
    }
    else if (!OnSel && !m_Moving)
    {
        m_DragStart2X=x;
        m_DragStart2Y=y;
        refresh=true;
        int xx=m_DragStartX<m_DragStart2X ? m_DragStartX : m_DragStart2X;
        int yy=m_DragStartY<m_DragStart2Y ? m_DragStartY : m_DragStart2Y;
        int x2=m_DragStartX>m_DragStart2X ? m_DragStartX : m_DragStart2X;
        int y2=m_DragStartY>m_DragStart2Y ? m_DragStartY : m_DragStart2Y;

        m_List.Select(xx,yy,x2,y2);
    }
    else
    {
        bool stick[4]={ false,false,false,false };

        if (m_Curr!=NULL)
        {
            bool found=false;
            for (CIwUIEdLayoutItem* item=m_Curr->m_Parent; item!=NULL && !found; item=item->m_Parent)
            {
                for (int j=0; j<(int)m_Selection.size(); j++)
                {
                    if (m_Selection[j]==item)
                    {
                        found=true;
                        break;
                    }
                }
            }

            if (!found)
                Move(x,y,bh,bw,stick,e,m_Curr);
        }

        for (i=0; i<(int)m_Selection.size(); i++)
        {
            bool found=m_Selection[i]==m_Curr;
            for (CIwUIEdLayoutItem* item=m_Selection[i]->m_Parent; item!=NULL && !found; item=item->m_Parent)
            {
                for (int j=0; j<(int)m_Selection.size(); j++)
                {
                    if (m_Selection[j]==item)
                    {
                        found=true;
                        break;
                    }
                }
            }

            if (!found)
                Move(x,y,bh,bw,stick,e,m_Selection[i]);
        }

        if (!stick[0])
            m_DragStartX=x;

        if (!stick[2])
            m_DragStart2X=x;

        if (!stick[1])
            m_DragStartY=y;

        if (!stick[3])
            m_DragStart2Y=y;

        refresh=true;
        m_Moving=true;
    }

    if (refresh)
        Refresh(false);
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::Move(int x,int y,int bh,int bw,bool stick[4],wxMouseEvent& e,CIwUIEdLayoutItem* curr)
{
    int val[4]={ 0,0,0,0 };

    switch (m_Curr->sel)
    {
    case 5:
        val[0]=(x-m_DragStartX);
        val[1]=(y-m_DragStartY);
        val[2]=(x-m_DragStart2X);
        val[3]=(y-m_DragStart2Y);
        if (e.ControlDown())
        {
            int newX=SNAP(curr->x,16);
            int newY=SNAP(curr->y,16);
            curr->x1+=newX-curr->x;
            curr->y1+=newY-curr->y;
            curr->x=newX;
            curr->y=newY;
        }
        else if (bh!=0)
        {
            int newX=SNAP(curr->x,bw);
            int newY=SNAP(curr->y,bh);
            curr->x1+=newX-curr->x;
            curr->y1+=newY-curr->y;
            curr->x=newX;
            curr->y=newY;
        }

        break;
    case 6:
        val[0]=(x-m_DragStartX);
        if (e.ControlDown())
            curr->x=SNAP(curr->x,16);
        else if (bh!=0)
            curr->x=SNAP(curr->x,bw);

        break;
    case 7:
        val[2]=(x-m_DragStart2X);
        if (e.ControlDown())
            curr->x1=SNAP(curr->x1,16);
        else if (bh!=0)
            curr->x1=curr->x+SNAP(curr->x1-curr->x,bw);

        break;

    case 9:
        val[1]=(y-m_DragStartY);
        if (e.ControlDown())
            curr->y=SNAP(curr->y,16);
        else if (bh!=0)
            curr->y=SNAP(curr->y,bh);

        break;
    case 10:
        val[0]=(x-m_DragStartX);
        val[1]=(y-m_DragStartY);
        if (e.ControlDown())
        {
            curr->x=SNAP(curr->x,16);
            curr->y=SNAP(curr->y,16);
        }
        else if (bh!=0)
        {
            curr->x=SNAP(curr->x,bw);
            curr->y=SNAP(curr->y,bh);
        }

        break;
    case 11:
        val[1]=(y-m_DragStartY);
        val[2]=(x-m_DragStart2X);
        if (e.ControlDown())
        {
            curr->y=SNAP(curr->y,16);
            curr->x1=SNAP(curr->x1,16);
        }
        else if (bh!=0)
        {
            curr->y=SNAP(curr->y,bh);
            curr->x1=curr->x+SNAP(curr->x1-curr->x,bw);
        }

        break;

    case 13:
        val[3]=(y-m_DragStart2Y);
        if (e.ControlDown())
            curr->y1=SNAP(curr->y1,16);
        else if (bh!=0)
            curr->y1=curr->y+SNAP(curr->y1-curr->y,bh);

        break;
    case 14:
        val[0]=(x-m_DragStartX);
        val[3]=(y-m_DragStart2Y);
        if (e.ControlDown())
        {
            curr->x=SNAP(curr->x,16);
            curr->y1=SNAP(curr->y1,16);
        }
        else if (bh!=0)
        {
            curr->x=SNAP(curr->x,bw);
            curr->y1=curr->y+SNAP(curr->y1-curr->y,bh);
        }

        break;
    case 15:
        val[2]=(x-m_DragStart2X);
        val[3]=(y-m_DragStart2Y);
        if (e.ControlDown())
        {
            curr->x1=SNAP(curr->x1,16);
            curr->y1=SNAP(curr->y1,16);
        }
        else if (bh!=0)
        {
            curr->x1=curr->x+SNAP(curr->x1-curr->x,bw);
            curr->y1=curr->y+SNAP(curr->y1-curr->y,bh);
        }

        break;
    }

    if (e.ControlDown())
    {
        if (fabs(val[0])<16)
            stick[0]=true;

        if (fabs(val[1])<16)
            stick[1]=true;

        if (fabs(val[2])<16)
            stick[2]=true;

        if (fabs(val[3])<16)
            stick[3]=true;
    }
    else if (bh!=0)
    {
        if (fabs(val[0])<bw)
            stick[0]=true;

        if (fabs(val[1])<bh)
            stick[1]=true;

        if (fabs(val[2])<bw)
            stick[2]=true;

        if (fabs(val[3])<bh)
            stick[3]=true;
    }
    else if (!e.ShiftDown() && !e.ControlDown() && curr==m_Curr)
    {
        m_List.StickyOuterEdge(curr,stick,val);
        m_List.StickyInnerEdge(curr,stick,val);
    }

    if ((curr->x1+val[2]<2 || (curr->x+val[0]>m_ScreenSize.x-2) && (curr->m_Flags&CIwUIEdLayoutItem::HOVERING)==0))
    {
        val[0]=0;
        val[2]=0;
    }

    if ((curr->y1+val[3]<2 || (curr->y+val[1]>m_ScreenSize.y-2) && (curr->m_Flags&CIwUIEdLayoutItem::HOVERING)==0))
    {
        val[1]=0;
        val[3]=0;
    }

    curr->Move(stick,val);

    if (curr->x1-curr->x<1) curr->x1=curr->x+1;

    if (curr->y1-curr->y<1) curr->y1=curr->y+1;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::Move(bool stick[4],int val[4])
{
    if (!stick[0])
        x+=val[0];

    if (!stick[2])
        x1+=val[2];

    if (!stick[1])
        y+=val[1];

    if (!stick[3])
        y1+=val[3];

    bool stick2[4]={stick[0],stick[1],stick[0],stick[1]};
    int val2[4]={val[0],val[1],val[0],val[1]};

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->Move(stick2,val2);
    }

    //if(m_Parent!=NULL)
    //	m_Parent->SizeChanged();
}

//--------------------------------------------------------------------------------
CIwUIEdLayoutItem* CIwUIEdLayoutItem::Select(int xx,int yy)
{
    for (int i=m_Children.size()-1; i>=0; i--)
    {
        CIwUIEdLayoutItem* item=m_Children[i]->Select(xx,yy);
        if (item!=NULL)
            return item;
    }

    if (xx<x-6 || xx>x1+5)
        return NULL;

    if (yy<y-6 || yy>y1+5)
        return NULL;

    if ((m_Flags&ROOT_ELEMENT)==ROOT_ELEMENT)
        return this;

    return NULL;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::Select(int xx,int yy,int x2,int y2)
{
    sel=0;

    if (xx<x && x2<x) return;

    if (xx>x1 && x2>x1) return;

    if (yy<y && y2<y) return;

    if (yy>y1 && y2>y1) return;

    for (int i=m_Children.size()-1; i>=0; i--)
    {
        m_Children[i]->Select(xx,yy,x2,y2);
    }

    if ((m_Flags&ROOT_ELEMENT)==ROOT_ELEMENT)
        if (!(xx>x && x2<x1 && yy>y && y2<y1))
            sel=5;

}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::Select2(int xx,int yy)
{
    if ((m_Flags&ROOT_ELEMENT)==0)
        sel=0;
    else if ((m_Flags&FIXED_SIZE)==FIXED_SIZE)
        sel=5;
    else
    {
        int a=1,b=4;

        int w=(x1-x)/4;
        if (w>7) w=7;

        int h=(y1-y)/4;
        if (h>7) h=7;

        if (xx<x+w) a=2;

        if (xx>x1-w) a=3;

        if (yy<y+h) b=8;

        if (yy>y1-h) b=12;

        sel=a+b;
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::GetSelection(std::vector<CIwUIEdLayoutItem*>& selList)
{
    if (sel!=0)
        selList.push_back(this);

    for (int i=m_Children.size()-1; i>=0; i--)
    {
        m_Children[i]->GetSelection(selList);
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::SizeChanged()
{
    if (m_Children.empty() || (m_Flags&FIXED_SIZE)==0)
    {
        x=m_Children[0]->x;
        y=m_Children[0]->y;
        x1=m_Children[0]->x1;
        y1=m_Children[0]->y1;

        for (int i=1; i<(int)m_Children.size(); i++)
        {
            if (m_Children[i]->x<x) x=m_Children[i]->x;

            if (m_Children[i]->y<y) y=m_Children[i]->y;

            if (m_Children[i]->x1>x1) x1=m_Children[i]->x1;

            if (m_Children[i]->y1>y1) y1=m_Children[i]->y1;
        }
    }

    if (m_Parent!=NULL)
        m_Parent->SizeChanged();
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::Clear()
{
    sel=0;

    for (int i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->Clear();
    }
}

#define MARGIN 4

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::StickyOuterEdge(CIwUIEdLayoutItem* item,bool stick[4],int val[4])
{
    int i;
    bool found=false;
    if (this==item) return;

    for (i=0; i<(int)m_Frame->m_Selection.size(); i++)
    {
        if (this==m_Frame->m_Selection[i])
        {
            found=true;
            break;
        }
    }

    if (!found)
    {
        sel=0;

        int val2[4]={0,0,0,0};
        bool stick2[4]={false,false,false,false};
        if ((m_Flags&ROOT_ELEMENT)==ROOT_ELEMENT || m_Parent==NULL)
        {
            if (y-MARGIN<item->y1+val[3] && y1+MARGIN>item->y+val[1])
            {
                if (x1-MARGIN<item->x+val[0] && x1+MARGIN>item->x+val[0])
                {
                    if ((item->sel&3)==1)
                    {
                        stick[2]=true;
                        val2[2]=x1-item->x;
                    }

                    stick[0]=true;
                    val[0]=val2[0]=x1-item->x;
                    sel=64;
                }

                if (x-MARGIN<item->x1+val[2] && x+MARGIN>item->x1+val[2])
                {
                    if ((item->sel&3)==1)
                    {
                        stick[0]=true;
                        val2[0]=x-item->x1;
                    }

                    stick[2]=true;
                    val[2]=val2[2]=x-item->x1;
                    sel=64;
                }
            }

            if (x-MARGIN<item->x1+val[2] && x1+MARGIN>item->x+val[0])
            {
                if (y1-MARGIN<item->y+val[1] && y1+MARGIN>item->y+val[1])
                {
                    if ((item->sel&12)==4)
                    {
                        stick[3]=true;
                        val2[3]=y1-item->y;
                    }

                    stick[1]=true;
                    val[1]=val2[1]=y1-item->y;
                    sel=64;
                }

                if (y-MARGIN<item->y1+val[3] && y+MARGIN>item->y1+val[3])
                {
                    if ((item->sel&12)==4)
                    {
                        stick[1]=true;
                        val2[1]=y-item->y1;
                    }

                    stick[3]=true;
                    val[3]=val2[3]=y-item->y1;
                    sel=64;
                }
            }
        }

        item->Move(stick2,val2);
    }

    for (i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->StickyOuterEdge(item,stick,val);
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutItem::StickyInnerEdge(CIwUIEdLayoutItem* item,bool stick[4],int val[4])
{
    int i;
    bool found=false;
    if (this==item) return;

    for (i=0; i<(int)m_Frame->m_Selection.size(); i++)
    {
        if (this==m_Frame->m_Selection[i])
        {
            found=true;
            break;
        }
    }

    if (!found)
    {
        if (sel!=64)
            sel=0;

        int val2[4]={0,0,0,0};
        bool stick2[4]={false,false,false,false};
        if ((m_Flags&ROOT_ELEMENT)==ROOT_ELEMENT || m_Parent==NULL)
        {
            if (y-MARGIN<item->y1+val[3] && y1+MARGIN>item->y+val[1])
            {
                if (x-MARGIN<item->x+val[0] && x+MARGIN>item->x+val[0])
                {
                    if ((item->sel&3)==1)
                    {
                        stick[2]=true;
                        val2[2]=x-item->x;
                    }

                    stick[0]=true;
                    val[0]=val2[0]=x-item->x;
                    sel=64;
                }

                if (x1-MARGIN<item->x1+val[2] && x1+MARGIN>item->x1+val[2])
                {
                    if ((item->sel&3)==1)
                    {
                        stick[0]=true;
                        val2[0]=x1-item->x1;
                    }

                    stick[2]=true;
                    val[2]=val2[2]=x1-item->x1;
                    sel=64;
                }
            }

            if (x-MARGIN<item->x1+val[2] && x1+MARGIN>item->x+val[0])
            {
                if (y-MARGIN<item->y+val[1] && y+MARGIN>item->y+val[1])
                {
                    if ((item->sel&12)==4)
                    {
                        stick[3]=true;
                        val2[3]=y-item->y;
                    }

                    stick[1]=true;
                    val[1]=val2[1]=y-item->y;
                    sel=64;
                }

                if (y1-MARGIN<item->y1+val[3] && y1+MARGIN>item->y1+val[3])
                {
                    if ((item->sel&12)==4)
                    {
                        stick[1]=true;
                        val2[1]=y1-item->y1;
                    }

                    stick[3]=true;
                    val[3]=val2[3]=y1-item->y1;
                    sel=64;
                }
            }
        }

        item->Move(stick2,val2);
    }

    for (i=0; i<(int)m_Children.size(); i++)
    {
        m_Children[i]->StickyInnerEdge(item,stick,val);
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::DrawCorner(wxDC& dc,CIwUIEdLayoutItem& item,int corner)
{
    int w=(item.x1-item.x)/2;
    if (w>7) w=7;

    int h=(item.y1-item.y)/2;
    if (h>7) h=7;

    switch (corner)
    {
    case 0:
        dc.DrawLine(m_Border+item.x-6,m_Border+item.y-6,m_Border+item.x+w,m_Border+item.y-6);
        dc.DrawLine(m_Border+item.x-6,m_Border+item.y-6,m_Border+item.x-6,m_Border+item.y+h);
        break;
    case 1:
        dc.DrawLine(m_Border+item.x1+5,m_Border+item.y-6,m_Border+item.x1-w,m_Border+item.y-6);
        dc.DrawLine(m_Border+item.x1+5,m_Border+item.y-6,m_Border+item.x1+5,m_Border+item.y+h);
        break;
    case 2:
        dc.DrawLine(m_Border+item.x-6,m_Border+item.y1+5,m_Border+item.x+w,m_Border+item.y1+5);
        dc.DrawLine(m_Border+item.x-6,m_Border+item.y1+5,m_Border+item.x-6,m_Border+item.y1-h);
        break;
    case 3:
        dc.DrawLine(m_Border+item.x1+5,m_Border+item.y1+5,m_Border+item.x1-w,m_Border+item.y1+5);
        dc.DrawLine(m_Border+item.x1+5,m_Border+item.y1+5,m_Border+item.x1+5,m_Border+item.y1-h);
        break;
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::DrawLine(wxDC& dc,CIwUIEdLayoutItem& item,int line)
{
    int w=(item.x1-item.x)/2;
    if (w>7) w=7;

    int h=(item.y1-item.y)/2;
    if (h>7) h=7;

    switch (line)
    {
    case 0:
        dc.DrawLine(m_Border+item.x-6,m_Border+item.y+h,m_Border+item.x-6,m_Border+item.y1-h+1);
        break;
    case 1:
        dc.DrawLine(m_Border+item.x+w,m_Border+item.y-6,m_Border+item.x1-w+1,m_Border+item.y-6);
        break;
    case 2:
        dc.DrawLine(m_Border+item.x1+5,m_Border+item.y+h,m_Border+item.x1+5,m_Border+item.y1-h+1);
        break;
    case 3:
        dc.DrawLine(m_Border+item.x+w,m_Border+item.y1+5,m_Border+item.x1-w+1,m_Border+item.y1+5);
        break;
    }
}

//--------------------------------------------------------------------------------
wxBitmap CIwUIEdLayoutFrame::Draw(CIwUIEdLayoutItem& item)
{
    wxBitmap bmp(item.x1-item.x,item.y1-item.y);
    wxMemoryDC dc(bmp);

    dc.SetPen(wxPen(wxColour(255,255,255),0,wxTRANSPARENT));
    dc.SetBrush(wxBrush(wxColour(255,0,255)));
    dc.DrawRectangle(0,0,item.x1-item.x,item.y1-item.y);

    if (item.m_Def!=NULL && (item.m_Flags&CIwUIEdLayoutItem::INVISIBLE)==0)
    {
        int x=item.x,y=item.y,x1=item.x1,y1=item.y1;

        if (item.m_Def->m_Type==DISPLAY_TEXT)
        {
            /*switch(item.m_Def->m_FontAlign&FONTALIGN_HMASK)
               {
                case FONTALIGN_HMID:
                    x=item.x+(item.x1-item.x-item.m_Bitmap.GetWidth())/2;
                    x1=item.x+(item.x1-item.x+item.m_Bitmap.GetWidth())/2;
                    break;
                case FONTALIGN_RIGHT:
                    x=item.x1-item.m_Bitmap.GetWidth();
                    x1=item.x1;
                    break;
               }
               switch(item.m_Def->m_FontAlign&FONTALIGN_VMASK)
               {
                case FONTALIGN_VMID:
                    y=item.y+(item.y1-item.y-item.m_Bitmap.GetHeight())/2;
                    y1=item.y+(item.y1-item.y+item.m_Bitmap.GetHeight())/2;
                    break;
                case FONTALIGN_BOTTOM:
                    y=item.y1-item.m_Bitmap.GetHeight();
                    y1=item.y1;
                    break;
               }*/
        }

        if (x<0) x=0;

        if (y<0) y=0;

        if ((item.m_Flags&CIwUIEdLayoutItem::HOVERING)==0)
        {
            if (x1>m_ScreenSize.x) x1=m_ScreenSize.x;

            if (y1>m_ScreenSize.y) y1=m_ScreenSize.y;
        }

        int w=x1-x,h=y1-y;

        switch (item.m_Def->m_Type)
        {
        case DISPLAY_NONE:
            dc.SetPen(wxPen(item.m_Colour,0,wxSOLID));
            dc.SetBrush(wxBrush(item.m_Colour,wxTRANSPARENT));
            dc.DrawRectangle(m_Border+x,m_Border+y,x1-x,y1-y);

            if (!item.m_Bitmap.IsOk()) break;

            dc.SetClippingRegion(m_Border+x,m_Border+y,x1-x,y1-y);
            dc.DrawBitmap(item.m_Bitmap,m_Border+x,m_Border+y);
            dc.DestroyClippingRegion();
            break;
        case DISPLAY_FILLEDBOX:
            dc.SetPen(wxPen(wxColour(255,255,255),0,wxTRANSPARENT));
            dc.SetBrush(wxBrush(item.m_Colour));
            dc.DrawRectangle(m_Border+x,m_Border+y,x1-x,y1-y);
            break;
        case DISPLAY_LINEBOX:
            dc.SetPen(wxPen(item.m_Colour,0,wxSOLID));
            dc.SetBrush(wxBrush(item.m_Colour,wxTRANSPARENT));
            dc.DrawRectangle(m_Border+x,m_Border+y,x1-x,y1-y);
            break;
        case DISPLAY_LINE:
            dc.SetPen(wxPen(item.m_Colour,0,wxSOLID));
            dc.SetBrush(wxBrush(item.m_Colour,wxTRANSPARENT));
            dc.DrawLine(m_Border+x,m_Border+y,m_Border+x1,m_Border+y1);
            break;
        case DISPLAY_TEXT:
            if (!item.m_Bitmap.IsOk()) break;

            dc.SetClippingRegion(m_Border+x,m_Border+y,x1-x,y1-y);
            dc.DrawBitmap(item.m_Bitmap,m_Border+x,m_Border+y);
            dc.DestroyClippingRegion();
            break;
        case DISPLAY_BITMAP:
            if (!item.m_Bitmap.IsOk()) break;

            dc.SetClippingRegion(m_Border+x,m_Border+y,x1-x,y1-y);
            for (int i=0; i*item.m_Bitmap.GetWidth()<x1-item.x; i++)
            {
                for (int j=0; j*item.m_Bitmap.GetHeight()<y1-item.y; j++)
                {
                    dc.DrawBitmap(item.m_Bitmap,m_Border+item.x+i*item.m_Bitmap.GetWidth(),m_Border+item.y+j*item.m_Bitmap.GetHeight());
                }
            }
            dc.DestroyClippingRegion();
            break;
        }
    }

    for (int i=0; i<(int)item.m_Children.size(); i++)
    {
        DrawElem(dc,Draw(*item.m_Children[i]),item);
    }

    if (item.m_Flags&CIwUIEdLayoutItem::ROOT_ELEMENT)
    {
        dc.SetPen(wxPen(wxColour(128,0,0),0,wxDOT));
        dc.SetBrush(wxBrush(wxColour(0,0,0),wxTRANSPARENT));
        dc.DrawRectangle(m_Border+item.x,m_Border+item.y,item.x1-item.x,item.y1-item.y);
    }

    return bmp;
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::DrawElem(wxDC& dc,wxBitmap& bmp,CIwUIEdLayoutItem& item)
{
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::DrawLines(wxDC& dc,CIwUIEdLayoutItem& item)
{
    dc.SetPen(wxPen(wxColour(255,0,0),0));
    switch (item.sel)
    {
    case 5:
        DrawCorner(dc,item,0);
        DrawCorner(dc,item,1);
        DrawCorner(dc,item,2);
        DrawCorner(dc,item,3);
        DrawLine(dc,item,0);
        DrawLine(dc,item,1);
        DrawLine(dc,item,2);
        DrawLine(dc,item,3);
        break;
    case 6:
        DrawLine(dc,item,0); break;
    case 7:
        DrawLine(dc,item,2); break;

    case 9:
        DrawLine(dc,item,1); break;
    case 10:
        DrawCorner(dc,item,0); break;
    case 11:
        DrawCorner(dc,item,1); break;

    case 13:
        DrawLine(dc,item,3); break;
    case 14:
        DrawCorner(dc,item,2); break;
    case 15:
        DrawCorner(dc,item,3); break;
    case 32:
        dc.SetPen(wxPen(wxColour(255,0,0),0,wxDOT));
        DrawCorner(dc,item,0);
        DrawCorner(dc,item,1);
        DrawCorner(dc,item,2);
        DrawCorner(dc,item,3);
        DrawLine(dc,item,0);
        DrawLine(dc,item,1);
        DrawLine(dc,item,2);
        DrawLine(dc,item,3);
        break;
    case 64:
        dc.SetPen(wxPen(wxColour(0,0,255),0));
        DrawCorner(dc,item,0);
        DrawCorner(dc,item,1);
        DrawCorner(dc,item,2);
        DrawCorner(dc,item,3);
        DrawLine(dc,item,0);
        DrawLine(dc,item,1);
        DrawLine(dc,item,2);
        DrawLine(dc,item,3);
        break;
    }

    for (int i=0; i<(int)item.m_Children.size(); i++)
    {
        DrawLines(dc,*item.m_Children[i]);
    }
}

//--------------------------------------------------------------------------------
void CIwUIEdLayoutFrame::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    if (m_ChooseSize.y<m_ScreenSize.y)
        m_ChooseSize.y=m_ScreenSize.y;

    wxBitmap bitmap(m_ScreenSize.x+m_Border*3+m_ChooseSize.x,m_ChooseSize.y+m_Border*2);
    wxMemoryDC dc(bitmap);
    wxPaintDC pdc(this);
    int i;

    dc.SetPen(wxPen(wxColour(255,255,255),0,wxTRANSPARENT));
    dc.SetBrush(wxColour(200,200,200));
    dc.DrawRectangle(0,0,m_ScreenSize.x+m_Border*3+m_ChooseSize.x,m_ChooseSize.y+m_Border*2);

    dc.SetPen(wxPen(wxColour(64,64,64),0));
    for (i=0; i<m_ScreenSize.x; i+=4)
    {
        dc.DrawLine(m_Border+i,3,m_Border+i,m_Border-3);
    }
    for (i=0; i<m_ScreenSize.y; i+=4)
    {
        dc.DrawLine(3,m_Border+i,m_Border-3,m_Border+i);
    }
    for (i=0; i<m_ScreenSize.x; i+=16)
    {
        dc.DrawLine(m_Border+i,1,m_Border+i,m_Border-1);
    }
    for (i=0; i<m_ScreenSize.y; i+=16)
    {
        dc.DrawLine(1,m_Border+i,m_Border-1,m_Border+i);
    }

    dc.SetPen(wxPen(wxColour(255,255,255),0,wxTRANSPARENT));
    dc.SetBrush(*wxLIGHT_GREY_BRUSH);
    dc.DrawRectangle(m_Border,m_Border,m_ScreenSize.x,m_ScreenSize.y);

    dc.SetPen(wxPen(wxColour(255,255,255),0,wxTRANSPARENT));
    dc.SetBrush(*wxLIGHT_GREY_BRUSH);
    dc.DrawRectangle(m_ScreenSize.x+m_Border*2,m_Border,m_ChooseSize.x,m_ChooseSize.y);

    wxBitmap bmp=Draw(m_Choose);
    dc.DrawBitmap(bmp,m_Border+m_Choose.x,m_Border+m_Choose.y);
    bmp=Draw(m_List);
    dc.DrawBitmap(bmp,m_Border+m_List.x,m_Border+m_List.y);

    if (m_CurrChoose!=NULL)
    {
        dc.SetPen(wxPen(wxColour(255,0,0),0));
        dc.SetBrush(wxBrush(m_CurrChoose->m_Colour,wxTRANSPARENT));

        int w=m_CurrChoose->x1-m_CurrChoose->x,h=m_CurrChoose->y1-m_CurrChoose->y;
        dc.DrawRectangle(m_Border+m_CurrChoose->x-6,m_Border+m_CurrChoose->y-6,w+11,h+11);
    }

    DrawLines(dc,m_List);

    if (m_DragStartX!=-1 && m_Dragging && !m_Moving)
    {
        dc.SetPen(wxPen(wxColour(255,0,0),0,wxSHORT_DASH));
        dc.SetBrush(wxBrush(wxColour(0,0,0),wxTRANSPARENT));
        dc.DrawRectangle(m_Border+m_DragStartX,m_Border+m_DragStartY,m_DragStart2X-m_DragStartX,m_DragStart2Y-m_DragStartY);
    }

    pdc.Blit(0,0,m_ScreenSize.x+m_Border*3+m_ChooseSize.x,m_ChooseSize.y+m_Border*2,&dc,0,0);
}

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

#include "wx/html/forcelnk.h"
#include "wx/html/m_templ.h"
#include "MarmaladeVersion.h"

FORCE_LINK_ME(IwHtmlInfo)

static std::vector<wxString> LicenseTag;
static std::vector<wxString> License;

//adds tag <i3d /> to html in about box which has the current version number
TAG_HANDLER_BEGIN(I3D, "I3D")
TAG_HANDLER_CONSTR(I3D) {
}

TAG_HANDLER_PROC(tag)
{
    wxHtmlContainerCell *c;

    m_WParser->CloseContainer();
    c = m_WParser->OpenContainer();

    wxString value=tag.GetParam(L"type");

    if (value.IsSameAs(L"license",false))
    {
        if (License.empty())
        {
            wxString env;
            wxGetEnv(L"S3E_DIR",&env);
            wxTextFile fp(wxString::Format(L"%s/licenses/iwlicense.dat",env.c_str()));
            if (fp.Exists() && fp.Open())
            {
                bool Add=false;

                for (int i=0; i<(int)fp.GetLineCount(); i++)
                {
                    if (!fp[i].empty() && fp[i][fp[i].size()-1]=='\n')
                        fp[i].RemoveLast();

                    if (fp[i].IsSameAs(L"--- BEGIN IW3D LICENSE TEXT ---",false))
                        Add=true;
                    else if (fp[i].IsSameAs(L"--- END IW3D LICENSE TEXT ---",false))
                        Add=false;
                    else if (Add)
                    {
                        if (fp[i].Find(L':')!=wxNOT_FOUND)
                        {
                            LicenseTag.push_back(fp[i].BeforeFirst(':'));
                            License.push_back(fp[i].AfterFirst(':').Trim(false));
                        }
                    }
                }
            }
            else
            {
                LicenseTag.push_back(L"License Key");
                License.push_back(L"<Unlicensed>");
            }
        }

        wxString value2=tag.GetParam(L"field");

        for (int i=0; i<(int)License.size(); i++)
        {
            if (LicenseTag[i].Lower().StartsWith(value2.Lower()))
                c->InsertCell(new wxHtmlWordCell(License[i],*(m_WParser->GetDC())));
        }
    }
    else
        c->InsertCell(new wxHtmlWordCell(wxString(MARMALADE_VERSION_STRING_FULL,wxConvUTF8),*(m_WParser->GetDC())));

    m_WParser->CloseContainer();
    m_WParser->OpenContainer();

    return false;
}

TAG_HANDLER_END(I3D)

TAGS_MODULE_BEGIN(I3DInfo)

TAGS_MODULE_ADD(I3D)

TAGS_MODULE_END(I3DInfo)

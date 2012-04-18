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
#ifndef IW_MODULE_H
#define IW_MODULE_H

class CIwASDApp;
class CIwLayoutDnDObject;
class CIwASDData;

//--------------------------------------------------------------------------------
#define MODULE_TAG(module,tag) (((tag)&0xffffff)|((module)<<24))
#define GET_ACTION_FROM_TAG(tag) ((tag)&0xffffff)
#define GET_MODULE_FROM_TAG(tag) ((tag)>>24)

//--------------------------------------------------------------------------------
// CIwLayoutElement
//	base class for UI containers (notebooks/sizers/sashes etc)

//--------------------------------------------------------------------------------
enum EIwLayoutElementQuery
{
    ELEMENT_QUERY_ISNOTEBOOK,
    ELEMENT_QUERY_ISSASH,
    ELEMENT_QUERY_ISEMPTY,
    ELEMENT_QUERY_CANNOTCLOSE,
    ELEMENT_QUERY_NOICON,
    ELEMENT_QUERY_NODRAGNDROP,
    ELEMENT_QUERY_ISTOOL,
    ELEMENT_QUERY_ISDATACONTAINER,

    ELEMENT_QUERY_ALL=0xffffffff
};

//--------------------------------------------------------------------------------
typedef bool (*TestDefFn)(std::vector<std::string>& conditions);

//--------------------------------------------------------------------------------
class CIwLayoutElement
{
public:
    int m_Indent;               //used in load
    wxString m_ParentData;  //parameter for adding to parent control
    wxString m_Type;            //Typename

    CIwLayoutElement* m_Parent;
    std::vector<CIwLayoutElement*> m_Children;
public:
    virtual ~CIwLayoutElement();

    virtual void Load(std::vector<wxString>& argv)=0;
    void Save(wxTextFile& fp,int indent);
    virtual wxString DoSave() { return L""; }
    virtual wxString GetParentData(CIwLayoutElement* Child) { return L"_"; }
    virtual wxWindow* GetControl()=0;

    virtual wxSizerItem* Create(wxWindow* parent)=0;            //create the window/sizer
    virtual bool AddChild(wxSizerItem* child,CIwLayoutElement* elem) { return false; }  //add a child to this container (if we are one)

    void Layout();
    virtual void DoLayout()=0;

    virtual wxString GetTitle() { return m_ParentData; }

    //override to provide results to questions like am i a notebook?
    virtual bool Query(EIwLayoutElementQuery value) { return false; }

    //override to accept drop (if Data==NULL it is a test drop for cursor mode), second version is replace child
    virtual wxDragResult DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutDnDObject* Data=NULL) { return wxDragNone; }
    virtual wxDragResult DoDrop(wxPoint Pt,wxDragResult def,CIwLayoutElement* Child,CIwLayoutDnDObject* Data=NULL) { return wxDragNone; }

    //remove a child element, if Delete is false returns sizer of element if sucessfull
    virtual wxSizerItem* RemoveChild(CIwLayoutElement* elem,bool Delete) { return NULL; }

    //override to provide extra handling when closing an element, if data needs saving add to list
    virtual void DoCheckSave(std::vector<CIwASDData*>& dataList) { }
    void CheckSave(std::vector<CIwASDData*>& dataList);

    virtual void DoCheckForReset() { }
    void CheckForReset();

    //called when the notebook page containing this is selected
    virtual void Selected() { }

    virtual bool HandleMouseEvent(wxMouseEvent& event)
    {
        for (int i=0; i<(int)m_Children.size(); i++)
        {
            if (m_Children[i]->HandleMouseEvent(event))
                return true;
        }
        return false;
    }
};

//--------------------------------------------------------------------------------
// CIwModule
#define UNKNOWN_TAG 0xffffffff

struct CIwLayoutTagDef;
//--------------------------------------------------------------------------------
class CIwModule
{
public:
    CIwASDApp* m_App;
    std::vector<CIwLayoutTagDef*> m_TagDefs;
    std::vector<unsigned int> m_TagDefTypes;
    wxString m_Prefix;
    unsigned char m_ModuleNum;
public:
    CIwModule(const wxString& prefix) : m_Prefix(prefix) { }
    virtual ~CIwModule() {}
    void Setup(CIwASDApp* App,unsigned char ModuleNum);

    //called once window is made
    virtual void PostWindowInit() { }

    //test tag for matching prefix, if matching return prefix length, else return UNKNOWN_TAG
    int IsPrefix(const wxString& tag,unsigned int type);
    //find the name of a tag from its id
    bool FindTagName(unsigned int tag,unsigned int type,wxString& Text);
    //returns EIwLayoutTagType  or returns type
    virtual unsigned int GetSubActionType(unsigned int tag,unsigned int type) { return type; }

    //override to make a window element (ie a pane or sizer)
    virtual CIwLayoutElement* MakeElement(const wxString& type) { return NULL; }

    //override to make a data object of a particular type (or derived from) that optionally matches info (eg. pass a file extension when creating files)
    virtual CIwASDData* MakeDataObject(unsigned int type,const wxString& info=L"") { return NULL; }
    //override to create a new data object of a particular type that is a copy of the origional possibly of a new type
    virtual CIwASDData* CloneDataObject(unsigned int type,CIwASDData* from,CIwASDData* root,bool copy) { return NULL; }

    //override to fill the list with icons and types with there associated types
    virtual void GetFileTypeInfo(wxImageList* list,std::vector<unsigned int>& types,std::vector<wxString>& names,int size) { }
protected:
    //override to add init code
    virtual void OnInit() {}
    //set the list of tags (name/id pairs) for a particular type
    void SetTagList(unsigned int type,CIwLayoutTagDef* list);
};

//you MUST provide this call in your code to register your modules
extern void RegisterModules(CIwASDApp* App);


#endif // !IW_MODULE_H

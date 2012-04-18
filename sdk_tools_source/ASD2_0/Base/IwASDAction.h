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
#ifndef IW_LAYOUT_ACTION_H
#define IW_LAYOUT_ACTION_H

class CIwAction;
class CIwModule;
class CIwLayoutElement;

//--------------------------------------------------------------------------------
struct CIwLayoutTagDef
{
    const wxChar* m_Name;
    unsigned int  m_ID;
};

//------------------------------------------------------------------------------
// CIwActionControl
//	base class the control side of an action
//------------------------------------------------------------------------------
class CIwActionControl
{
public:
    CIwAction* m_Action;    //can be NULL if gets unlinked
    std::vector<wxString> m_Conditions;
public:
    CIwActionControl() : m_Action(NULL) {}
    virtual ~CIwActionControl();
    virtual void Update()=0;
};

//------------------------------------------------------------------------------
// CIwAction
//	base class for actions
//------------------------------------------------------------------------------
class CIwAction
{
public:
    enum Flags
    {
        FLAG_DELETEONNOCTRLS=1, //normally set, unless you need to keep a reference to it
    };
protected:
    CIwModule* m_Module;
    bool m_Enable;
    int m_Value;
    Flags m_Flags;
    std::vector<CIwActionControl*> m_Controls;
public:
    void* m_Context;
public:
    CIwAction(CIwModule* module) : m_Module(module),m_Enable(true),m_Value(1),m_Flags(FLAG_DELETEONNOCTRLS),m_Context(NULL) {}
    virtual ~CIwAction();

    //From UI
    //	for buttons:value=1, for checks:value=0/1, for combos:value=0-num-1
    virtual void Action(int value=1)=0;
    //	item:-1 is main label
    //	item:0->n are combo box strings (return false to stop fetching strings)
    virtual bool GetLabel(wxString& text,int item=-1) { return false; }
    //	NULL means has no sub actions
    virtual CIwAction* MakeSubAction(const wxString& tag) {  return NULL; }

    //	change menu to be displayed
    virtual void OverrideMenu(wxMenuItem* Item,int& id) { }
    //	return control to put on toolbar
    virtual wxWindow* OverrideTool() { return NULL; }

    virtual bool CheckShouldAddSubAction(const wxString& name) { return true; }

    //data get/set
    virtual int GetValue() { return m_Value; }
    virtual bool GetEnable(int value=-1) { return m_Enable; }
    virtual bool DoCreate() { return true; }    //do we create this control?

    void SetValue(int value) { m_Value=value; }
    void SetEnable(bool enable) { m_Enable=enable; }

    void AddControl(CIwActionControl* ctrl);
    void RemoveControl(CIwActionControl* ctrl);
    virtual void DeleteThis() { delete this; }  //virtual for inter-dll problems

    //update state of Label/Enable/Value on control
    void Update();
};

//------------------------------------------------------------------------------
class CIwActionDelegate : public CIwAction
{
protected:
    FastDelegate0<> m_Delegate;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegate(CIwModule* module,FastDelegate0<> delegate) : CIwAction(module),m_Delegate(delegate) {}
    virtual void Action(int value=1);
};

//------------------------------------------------------------------------------
class CIwActionDelegateDouble : public CIwAction
{
protected:
    double m_Value;
    FastDelegate1<double> m_Delegate;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateDouble(CIwModule* module,FastDelegate1<double> delegate) : CIwAction(module),m_Value(1e100),m_Delegate(delegate) {}

    virtual CIwAction* MakeSubAction(const wxString& tag);
    virtual void Action(int value=1);
};

//------------------------------------------------------------------------------
class CIwActionDelegateInt : public CIwAction
{
protected:
    int m_Value;
    FastDelegate1<int> m_Delegate;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateInt(CIwModule* module,FastDelegate1<int> delegate) : CIwAction(module),m_Value(-5000),m_Delegate(delegate) {}

    virtual CIwAction* MakeSubAction(const wxString& tag);
    virtual void Action(int value=1);
};
//------------------------------------------------------------------------------
class CIwActionDelegateBool : public CIwAction
{
protected:
    FastDelegate1<bool> m_Delegate;
    FastDelegate0<bool> m_GetDelegate;
    FastDelegate0<bool> m_EnableDelegate;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateBool(CIwModule* module,FastDelegate1<bool> delegate,FastDelegate0<bool> getDelegate=NULL,
                          FastDelegate0<bool> enableDelegate=NULL) :
        CIwAction(module),m_Delegate(delegate),m_GetDelegate(getDelegate),m_EnableDelegate(enableDelegate) {}

    virtual void Action(int value=1);
    virtual int GetValue();
    virtual bool GetEnable(int value=-1);
};

//------------------------------------------------------------------------------
class CIwActionDelegateSub : public CIwAction
{
protected:
    unsigned int m_Value;
    CIwLayoutTagDef* m_Values;
    FastDelegate2<unsigned int,int> m_Delegate;
    FastDelegate1<unsigned int,unsigned int> m_GetDelegate;
    FastDelegate1<unsigned int,bool> m_EnableDelegate;
    std::map<unsigned int,CIwAction*> m_Overrides;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateSub(CIwModule* module,FastDelegate2<unsigned int,int> delegate,CIwLayoutTagDef* values,
                         FastDelegate1<unsigned int,unsigned int> getDelegate=NULL,FastDelegate1<unsigned int,bool> enableDelegate=NULL) :
        CIwAction(module),m_Value(0xffffffff),m_Values(values),m_Delegate(delegate),m_GetDelegate(getDelegate),m_EnableDelegate(enableDelegate) {}
    void AddOverride(unsigned int id,CIwAction* action) { m_Overrides[id]=action; }
    virtual ~CIwActionDelegateSub();

    virtual CIwAction* MakeSubAction(const wxString& tag);
    virtual void Action(int value=1);
    virtual int GetValue();
    virtual bool GetEnable(int value=-1);
};

//------------------------------------------------------------------------------
class CIwActionDelegateParam : public CIwAction
{
protected:
    FastDelegate1<const wxString&> m_Delegate;
    wxString m_FileName;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateParam(CIwModule* module,FastDelegate1<const wxString&> delegate,const wxString& file=L"") :
        CIwAction(module),m_Delegate(delegate),m_FileName(file) {}

    virtual CIwAction* MakeSubAction(const wxString& tag);
    virtual void Action(int value=1);
};

//------------------------------------------------------------------------------
class CIwActionDelegateList : public CIwAction
{
protected:
    FastDelegate1<int> m_Delegate;
    FastDelegate0<int> m_UpdateDelegate;
    FastDelegate1<int,bool> m_EnableDelegate;
public:
    //example: new CIwActionDelegate(this,MakeDelegate(this,class::fn));
    CIwActionDelegateList(CIwModule* module,FastDelegate1<int> delegate,FastDelegate0<int> update,FastDelegate1<int,bool> enable=NULL) :
        CIwAction(module),m_Delegate(delegate),m_UpdateDelegate(update),m_EnableDelegate(enable) {}

    virtual void Action(int value=1);
    virtual int GetValue();
    virtual bool GetEnable(int value=-1);
};

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

//------------------------------------------------------------------------------
template<class T> class CIwMemberAction : public CIwAction
{
public:
    typedef  void (T::*MemberPtr)(int value);
    typedef  bool (T::*LabelMemberPtr)(wxString& text,int item);
protected:
    MemberPtr m_MemberPtr;
    LabelMemberPtr m_LabelMemberPtr;
public:
    CIwMemberAction(CIwModule* module,MemberPtr memberPtr=NULL,LabelMemberPtr labelMemberPtr=NULL) :
        CIwAction(module),m_MemberPtr(memberPtr),m_LabelMemberPtr(labelMemberPtr) {}
    virtual void Action(int value=1)
    {
        T* data2=(T*)m_Context;
        if (m_MemberPtr!=NULL)
            CALL_MEMBER_FN(*data2,m_MemberPtr) (value);
    }
    virtual bool GetLabel(wxString& text,int item=-1)
    {
        T* data2=(T*)m_Context;
        if (m_LabelMemberPtr!=NULL)
            return CALL_MEMBER_FN(*data2,m_LabelMemberPtr) (text,item);

        return false;
    }
};

//------------------------------------------------------------------------------
template<class T> class CIwDataAction : public CIwMemberAction<T>
{
protected:
    unsigned int m_Type;
public:
    CIwDataAction(CIwModule* module,unsigned int type,typename CIwMemberAction<T>::MemberPtr memberPtr=NULL) : CIwMemberAction<T>(module,memberPtr),m_Type(type) {}
    virtual void Action(int value=1)
    {
        CIwASDData* data=(CIwASDData*)this->m_Context;
        if (!data->HasType(m_Type)) return;

        T* data2=(T*)this->m_Context;
        if (this->m_MemberPtr!=NULL)
            CALL_MEMBER_FN(*data2,this->m_MemberPtr) (value);
    }
    virtual bool GetLabel(wxString& text,int item=-1)
    {
        if (item!=-1) return false;

        std::map<wxString,wxString> opts;

        CIwASDData* data=(CIwASDData*)this->m_Context;
        if (data!=NULL)
        {
            wxColour col;
            data->GetOptions(opts,col);
        }

        text=ReplaceOptions(text,opts);

        return true;
    }
    virtual bool DoCreate()
    {
        CIwASDData* data=(CIwASDData*)this->m_Context;
        if (data!=NULL)
            if (!data->HasType(m_Type))
                return false;

        return true;
    }
};

//--------------------------------------------------------------------------------
class CIwActionMenu : public wxMenu
{
    CIwAction* m_Action;
public:
    CIwActionMenu(CIwAction* action) : m_Action(action) { }
    void OnItem(wxCommandEvent& e)
    {
        m_Action->Action(e.GetId());
    }

    DECLARE_EVENT_TABLE()
};

#endif

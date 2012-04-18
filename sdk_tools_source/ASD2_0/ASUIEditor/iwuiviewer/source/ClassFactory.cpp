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
// Standard includes
#include "ClassFactory.h"

// Library includes
#include "IwUIElement.h"
#include "IwUIPickerViewItemSource.h"
#include "IwUITableViewItemSource.h"

//-----------------------------------------------------------------------------

template<int n>
class CNameChanger : public CIwManaged
{
public:
    virtual const char* GetClassName() const
    {
        return s_ClassName;
    }

    static void* Create()
    {
        // Create base object and replace its v-table with our customised version
        CIwManaged* pObj = (CIwManaged*) IwClassFactoryCreate(s_BaseHash);
        *(intptr_t**)pObj = s_VTable + 1;
        return pObj;
    }

    int GetIndexOfGetClassName() const
    {
        CIwManaged temp;
        intptr_t* vtable1 = *(intptr_t**)&temp;
        intptr_t* vtable2 = *(intptr_t**)this;

        // Skip destructor
        int index = 1;
        ++vtable1, ++vtable2;

        // Determine second non-match
        while (*vtable1++ == *vtable2++)
        {
            ++index;
        }

        return index;
    }

    static void Register(const char *pClassName, const char* pBaseName)
    {
        strcpy(s_ClassName, pClassName);
        s_BaseHash = IwHashString(pBaseName);

        // Create a base object and find its v-table
        CIwManaged* pObj = (CIwManaged*) IwClassFactoryCreate(s_BaseHash);
        if (!pObj)
        {
            IwAssertMsg(UI, false,
                        ("Custom class %s base %s not recognised by class factory",
                         pClassName, pBaseName));
            return;
        }

        intptr_t* vtable = *(intptr_t**)pObj;
        // RTTI information stored in -1 slot
        --vtable;

        // Copy base objects v-table
        int i = 0;
        int num = sizeof(s_VTable) / sizeof(s_VTable[0]);
        while (*vtable && i < num)
        {
            s_VTable[i++] = *vtable++;
        }

        // Replace GetClassName function
        CNameChanger temp;
        int index = temp.GetIndexOfGetClassName();
        s_VTable[index+1] = (*(intptr_t**)&temp)[index];

        // Register class factory
        uint32 baseClassSize = IwClassFactoryGetSize(s_BaseHash);
        IwClassFactoryAdd(s_ClassName, Create, baseClassSize);

        delete pObj;
    }

    static uint32 s_BaseHash;
    static char s_ClassName[0x100];
    static intptr_t s_VTable[0x100];
};
template<int n>
uint32 CNameChanger<n>::s_BaseHash;
template<int n>
char CNameChanger<n>::s_ClassName[];
template<int n>
intptr_t CNameChanger<n>::s_VTable[];

#define REG_NC(num)     CNameChanger<num>::Register,

typedef void(*RegisterFn)(const char *, const char*);
static RegisterFn s_CustomClassRegister[] =
{
    REG_NC(0)       REG_NC(1)       REG_NC(2)       REG_NC(3)       REG_NC(4)
    REG_NC(5)       REG_NC(6)       REG_NC(7)       REG_NC(8)       REG_NC(9)
    REG_NC(10)      REG_NC(11)      REG_NC(12)      REG_NC(13)      REG_NC(14)
    REG_NC(15)      REG_NC(16)      REG_NC(17)      REG_NC(18)      REG_NC(19)
    REG_NC(20)      REG_NC(21)      REG_NC(22)      REG_NC(23)      REG_NC(24)
    REG_NC(25)      REG_NC(26)      REG_NC(27)      REG_NC(28)      REG_NC(29)
    REG_NC(30)      REG_NC(31)      REG_NC(32)      REG_NC(33)      REG_NC(34)
    REG_NC(35)      REG_NC(36)      REG_NC(37)      REG_NC(38)      REG_NC(39)
    REG_NC(40)      REG_NC(41)      REG_NC(42)      REG_NC(43)      REG_NC(44)
    REG_NC(45)      REG_NC(46)      REG_NC(47)      REG_NC(48)      REG_NC(49)
    REG_NC(50)      REG_NC(51)      REG_NC(52)      REG_NC(53)      REG_NC(54)
    REG_NC(55)      REG_NC(56)      REG_NC(57)      REG_NC(58)      REG_NC(59)
    REG_NC(60)      REG_NC(61)      REG_NC(62)      REG_NC(63)      REG_NC(64)
    REG_NC(65)      REG_NC(66)      REG_NC(67)      REG_NC(68)      REG_NC(69)
    REG_NC(70)      REG_NC(71)      REG_NC(72)      REG_NC(73)      REG_NC(74)
    REG_NC(75)      REG_NC(76)      REG_NC(77)      REG_NC(78)      REG_NC(79)
    REG_NC(80)      REG_NC(81)      REG_NC(82)      REG_NC(83)      REG_NC(84)
    REG_NC(85)      REG_NC(86)      REG_NC(87)      REG_NC(88)      REG_NC(89)
    REG_NC(90)      REG_NC(91)      REG_NC(92)      REG_NC(93)      REG_NC(94)
    REG_NC(95)      REG_NC(96)      REG_NC(97)      REG_NC(98)      REG_NC(99)
    REG_NC(100)     REG_NC(101)     REG_NC(102)     REG_NC(103)     REG_NC(104)
    REG_NC(105)     REG_NC(106)     REG_NC(107)     REG_NC(108)     REG_NC(109)
    REG_NC(110)     REG_NC(111)     REG_NC(112)     REG_NC(113)     REG_NC(114)
    REG_NC(115)     REG_NC(116)     REG_NC(117)     REG_NC(118)     REG_NC(119)
    REG_NC(120)     REG_NC(121)     REG_NC(122)     REG_NC(123)     REG_NC(124)
    REG_NC(125)     REG_NC(126)     REG_NC(127)
};

//-----------------------------------------------------------------------------

class CPickerViewItemSource : public CIwUIPickerViewItemSource
{
public:
    IW_MANAGED_DECLARE(CPickerViewItemSource);

    // IwUIPickerViewItemSource virtuals
    virtual int32 GetNumColumns() const { return 0; }
    virtual int32 GetNumRowsForColumn(int32 column) const { return 0; }
    virtual CIwUIElement* CreateItem(int32 column, int32 row) { return NULL; }
    virtual void ReleaseItem(CIwUIElement* pItem, int32 column, int32 row) { }
    virtual int32 GetColumnWidthHint(int32 column) const { return 0; }
    virtual int32 GetRowHeightForColumn(int32 column, int32 columnWidth) const { return 0; }
};
IW_MANAGED_IMPLEMENT_FACTORY(CPickerViewItemSource);

class CTableViewItemSource : public CIwUITableViewItemSource
{
public:
    IW_MANAGED_DECLARE(CTableViewItemSource);

    // IwUITableViewItemSource virtuals
    virtual bool IsRowAvailable(int32 row) const { return false; }
    virtual CIwUIElement* CreateItem(int32 row) { return NULL; }
    virtual void ReleaseItem(CIwUIElement* pItem, int32 row) { }
    virtual int32 GetRowHeight(int32 row, int32 columnWidth) const { return 0; }
};
IW_MANAGED_IMPLEMENT_FACTORY(CTableViewItemSource);

//-----------------------------------------------------------------------------

namespace ClassFactory
{

void RegisterClasses()
{
    // Provide non-abstract versions of these so can be specified
    // as the base-class of custom classes.
    IW_CLASS_REGISTER_AB(CIwUIPickerViewItemSource, CPickerViewItemSource);
    IW_CLASS_REGISTER_AB(CIwUITableViewItemSource, CTableViewItemSource);
}

void UnRegisterClasses()
{
    IW_CLASS_REMOVE(CIwUIPickerViewItemSource);
    IW_CLASS_REMOVE(CIwUITableViewItemSource);
}

void RegisterCustomClasses()
{
    // Read file list custom class mappings to known base classes
    s3eFile* cm = s3eFileOpen("_viewertemp/classmap.txt", "rb");
    if (cm)
    {
        // Read lines
        char buffer[0x200];
        int index = 0;
        while (s3eFileReadString(buffer, sizeof(buffer), cm))
        {
            IwTrace(UI, ("Registering custom class factory"));

            int num = sizeof(s_CustomClassRegister) / sizeof(s_CustomClassRegister[0]);
            if (index >= num)
            {
                IwAssertMsg(UI, false, ("No more custom class factories available"));
                break;
            }

            // Mapping of form "class base"
            char className[0x200];
            char baseName[0x200];
            if (sscanf(buffer, "%s %s", className, baseName) == 2)
            {
                // Register class factory
                s_CustomClassRegister[index](className, baseName);

                ++index;
            }
        }

        s3eFileClose(cm);
    }
}

}

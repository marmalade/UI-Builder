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
#ifndef BUCKET_ALLOCATOR_H
#define BUCKET_ALLOCATOR_H

// Library includes
#include "IwMemBucket.h"

#include <memory>
#include <limits>

extern CIwMemBucket* g_BucketAllocator;

//-----------------------------------------------------------------------------

template<class T>
class CBucketAllocator
{
public:
    template <class S>
    struct rebind
    {
        typedef CBucketAllocator<S> other;
    };

    typedef uint32      size_type;
    typedef int32       difference_type;
    typedef T*          pointer;
    typedef const T*    const_pointer;
    typedef T&          reference;
    typedef const T&    const_reference;
    typedef T           value_type;

    CBucketAllocator() {}
    ~CBucketAllocator() {}

    template<class Other>
    CBucketAllocator(const CBucketAllocator<Other>&) {}

    pointer address(reference x) const
    {
        return &x;
    }
    const_pointer address(const_reference x) const
    {
        return &x;
    }

    pointer allocate (size_type n, const_pointer hint=0)
    {
        return (pointer) g_BucketAllocator->Realloc(NULL, n * sizeof(T));
    }

    void deallocate (pointer p, size_type n)
    {
        g_BucketAllocator->Free(p);
    }

    size_type max_size() const throw()
    {
        return size_t(-1) / sizeof(value_type);
    }

    void construct (pointer p, const_reference val)
    {
        new ((void*)p)T (val);
    }

    void destroy(pointer p)
    {
        ((T*)p)->~T();
    }
};

namespace std
{

template <class _Tp1, class _Tp2>
CBucketAllocator<_Tp2>& __stl_alloc_rebind(CBucketAllocator<_Tp1>& __a, const _Tp2*)
{
    return (CBucketAllocator<_Tp2>&)(__a);
}

template <class _Tp1, class _Tp2>
CBucketAllocator<_Tp2> __stl_alloc_create(const CBucketAllocator<_Tp1>&, const _Tp2*)
{
    return CBucketAllocator<_Tp2>();
}

}

#endif

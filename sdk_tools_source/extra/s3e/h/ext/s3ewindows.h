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
/**
 * Windows-specific Extensions
 */
#ifndef S3E_EXT_WINDOWS_H
#define S3E_EXT_WINDOWS_H

#include "s3eTypes.h"
#include "s3eExt.h"

S3E_BEGIN_C_DECL

/**
 * This is the type of data returned requesting the "Win32DataDirChange" extension
 * e.g.
 * s3eExtWin32DataDirChangeFn fn;
 * s3eExtGet("Win32DataDirChange", &fn, sizeof(fn));
 * fn("c:\\my data directory");
 *
 * It allows the user to change the s3e data directory dynamically in windows only
 */
typedef void(*s3eExtWin32DataDirChangeFn) (const char* newDir);

S3E_END_C_DECL

#endif /* !S3E_EXT_WINDOWS_H */

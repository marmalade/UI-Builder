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
#ifndef IW_ASD_BASE_H
#define IW_ASD_BASE_H

#include "wxInclude.h"

#ifdef I3D_OS_WINDOWS
#include <winbase.h>
#include <shlobj.h>
#endif
#include <limits>

#include "FastDelegate.h"
using namespace fastdelegate;

//------------------------------------------------------------------------------
#include "IwStyleSheet.h"
#include "IwModule.h"
#include "IwASDData.h"
#include "IwASDAction.h"

#include "toggle.h"
#include "IwLayoutFrameModule.h"
#include "IwLayoutFrame.h"
#include "IwLayoutPanel.h"
#include "IwLayoutToolbar.h"
#include "IwASDApp.h"

//------------------------------------------------------------------------------
#ifndef NO_NEW_REMAP
#ifdef _DEBUG
#include <crtdbg.h>
#define DEBUG_NEW new (_NORMAL_BLOCK,__FILE__, __LINE__)
#define new DEBUG_NEW
#else
#define DEBUG_NEW new
#endif
#endif

#endif // !IW_ASD_BASE_H

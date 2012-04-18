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
#include "IwASDFileHeader.h"

#include "IwProject.h"
#include "IwSetProjectDialog.h"

#include <stdio.h>
#include <signal.h>

#include <sstream>
#include <iostream>

#define UIED_MODULE_ID 2

// Source compatibility with AirplayStudioDirector/iwviewer
#include "IwHostBase.h"
typedef CIwHostBase CIwViewerUI;

#include "IwUIEdProjectUI.h"
#include "IwUIEdData.h"
#include "Host.h"
#include "UIEdLayout.h"
#include "UIEdPane.h"
#include "UIEdAttrPane.h"
#include "UIEdProject.h"
#include "UIEdPalette.h"
#include "UIEdHierarchy.h"
#include "UIEdSource.h"
#include "IwUIEdMod.h"
#include "IwAttrDesc.h"
#include "IwAttrDescGrid.h"

//--------------------------------------------------------------------------------

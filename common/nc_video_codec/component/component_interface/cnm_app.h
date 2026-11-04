//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
// 
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
// 
// The entire notice above must be reproduced on all authorized copies.
// 
// Description  : 
//-----------------------------------------------------------------------------

#ifndef __CNM_APP_H__
#define __CNM_APP_H__

#include "vputypes.h"
#include "component.h"

typedef void*       CNMTask;

typedef enum {
    CNM_TASK_DONE,
    CNM_TASK_RUNNING,
    CNM_TASK_ERROR
} CNMTaskWaitState;

typedef void (*CNMTaskListener)(CNMTask task, void* context);

typedef struct CNMAppConfig {
    char            fwpath[256];
} CNMAppConfig;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void    CNMAppInit(void);
BOOL    CNMAppAdd(CNMTask task);
BOOL    CNMAppRun(void);
void    CNMAppStop(void);

CNMTask CNMTaskCreate(void);
void    CNMTaskRelease(CNMTask task);
BOOL    CNMTaskDestroy(CNMTask task);
BOOL    CNMTaskAdd(CNMTask task, Component component);
BOOL    CNMTaskRun(CNMTask task);
CNMTaskWaitState CNMTaskWait(CNMTask task);
BOOL    CNMTaskStop(CNMTask task);
BOOL    CNMTaskIsTerminated(CNMTask task);

enum {
    CNM_ERROR_NONE,
    CNM_ERROR_FAILURE,
    CNM_ERROR_HANGUP,
};
void CNMErrorSet(Int32 val);
Int32 CNMErrorGet();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CNM_APP_H__ */


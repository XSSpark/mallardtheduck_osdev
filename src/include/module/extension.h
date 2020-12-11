#ifndef _EXTENSION_H
#define _EXTENSION_H

#include "module_api.h"
#include "drivers.h"

typedef void (*uapi_hanlder_fn)(uint16_t,ICPUState&);

struct kernel_extension{
	char *name;
	void *calltable;
	uapi_hanlder_fn uapi_handler;
};

#ifndef __cplusplus
typedef struct kernel_extension kernel_extension;
#endif

#endif
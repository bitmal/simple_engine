#ifndef __CONSOLE_H
#define __CONSOLE_H

#include "types.h"

typedef i32 console_id;

struct basic_dict;

struct console_context
{
    console_id id;
    const char *name;
};

#endif
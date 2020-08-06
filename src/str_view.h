#ifndef __STR_VIEW_H
#define __STR_VIEW_H

#include <string.h>

#include "types.h"

struct str_view
{
    i32 length;
    const char *str;
};

struct str_view
str_view_create(const char *str)
{
    struct str_view view = {strlen(str), str};
    return view;
}

#endif
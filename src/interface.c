#include "interface.h"
#include "memory.h"
#include "basic_dict.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>

struct interface *
interface_init(struct memory *mem)
{
    assert(mem);

    struct interface *context = memory_alloc(mem, sizeof(struct interface));
    memset(context, 0, sizeof(struct interface));

    context->_memoryContext = mem;
    context->_elementMap = DICTIONARY(mem, NULL);

    return context;
}

b32
interface_create_element(struct interface *context, const char *name,
    struct interface_element_pattern *patterns, i32 patternCount,
    real32 x, real32 y, real32 width, real32 height, u32 color)
{
    assert(context);

    if (!basic_dict_get(context->_elementMap, context->_elements, name))
    {
        i32 index = context->_elementCount;
        if (context->_elementCount > 0)
        {
            if (context->_elementCount >= context->_elementCapacity)
            {
                context->_elements = memory_realloc(context->_memoryContext, context->_elements,
                    sizeof(struct interface_element)*(context->_elementCapacity += 10));
            }
        }
        else
        {
            context->_elements = memory_alloc(context->_memoryContext, 
                sizeof(struct interface_element)*(context->_elementCapacity = 10));
        }

        ++context->_elementCount;

        struct interface_element *elementPtr = &context->_elements[index];
        elementPtr->color = color;
        elementPtr->height = height;
        elementPtr->width = width;
        elementPtr->x = x;
        elementPtr->y = y;
        elementPtr->isDirty = B32_TRUE;
        elementPtr->prev = NULL;
        elementPtr->next = NULL;
        const u32 nameSize = strlen(name) + 1;
        (char *)elementPtr->name = memory_alloc(context->_memoryContext, nameSize);
        strcpy((char *)elementPtr->name, name);

        if (patternCount > 0)
        {
            elementPtr->patterns = memory_alloc(context->_memoryContext, sizeof(struct interface_element_pattern)*
                (elementPtr->patternCount = elementPtr->patternCapacity = patternCount));
            memcpy(elementPtr->patterns, patterns, sizeof(struct interface_element_pattern)*patternCount);
        }
        else
        {
            elementPtr->patterns = NULL;
            elementPtr->patternCount = 0;
            elementPtr->patternCapacity = 0;
        }

        basic_dict_set(context->_elementMap, context->_memoryContext, 
            context->_elements, name, strlen(name) + 1, elementPtr);

        return B32_TRUE;
    }

    return B32_FALSE;
}

void
interface_toggle_element(struct interface *context, const char *name, b32 b)
{
    struct interface_element *elementPtr = basic_dict_get(context->_elementMap, context->_elements, name);
    if (!elementPtr)
    {
        return;
    }

    if (b && !elementPtr->prev && !elementPtr->next)
    {
        if (context->_displayStackTop)
        {
            elementPtr->prev = context->_displayStackTop;
            context->_displayStackTop->next = elementPtr;
            context->_displayStackTop = elementPtr;
        }
        else
        {
            context->_displayStack = context->_displayStackTop = elementPtr;
        }
    }
    else if (!b && (elementPtr->prev || elementPtr->next))
    {
        if (elementPtr->next)
        {
            elementPtr->next->prev = elementPtr->prev;
        }
        if (elementPtr->prev)
        {
            elementPtr->prev->next = elementPtr->next;
        }
        if (context->_displayStack == elementPtr)
        {
            context->_displayStack = elementPtr->next;
        }
        if (context->_displayStackTop == elementPtr)
        {
            context->_displayStackTop = elementPtr->prev;
        }

        elementPtr->prev = NULL;
        elementPtr->next = NULL;
    }
}

void
interface_target_element(struct interface *context, const char *name)
{
    struct interface_element *elementPtr = basic_dict_get(context->_elementMap, context->_elements, name);
    if (!elementPtr)
    {
        return;
    }

    if (context->_displayStackTop != elementPtr)
    {
        if (elementPtr->prev)
        {
            elementPtr->prev->next = elementPtr->next;
        }
        if (elementPtr->next)
        {
            elementPtr->next->prev = elementPtr->prev;
        }

        if (!context->_displayStack)
        {
            context->_displayStack = elementPtr;
        }
        else if (context->_displayStack == elementPtr)
        {
            context->_displayStack = elementPtr->next;
        }
        
        if (context->_displayStackTop)
        {
            context->_displayStackTop->next = elementPtr;
        }
        
        elementPtr->prev = context->_displayStackTop;
        elementPtr->next = NULL;
        context->_displayStackTop = elementPtr;
    }
}
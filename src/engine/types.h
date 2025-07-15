#ifndef __TYPES_H
#define __TYPES_H

#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float real32;
typedef double real64;

typedef i32 b32;

#define B32_FALSE (b32)0
#define B32_TRUE (b32)1

typedef uint32_t p32;
typedef uint64_t p64;

#define STATIC_ASSERT(condition, msg) \
    typedef u8 static_assertion_##msg[(condition) ? 1 : 0]

#endif
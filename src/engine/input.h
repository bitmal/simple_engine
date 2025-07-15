#ifndef __INPUT_H
#define __INPUT_H

#include "types.h"

struct basic_dict;
struct memory;

typedef void(*input_key_callback)(const char *key, const char *action, void *dataPtr);

struct input_key
{
    const char *key;
    b32 isDown;
};

struct input_keybind
{
    const char *key;
    input_key_callback callback;
};

struct input
{
    struct input_key *keys;
    i32 keyCount;
    struct basic_dict *keyDict;
    struct input_keybind *keybinds;
    i32 keybindCount;
    struct basic_dict *keybindDict;
    void *dataPtr;
};

struct input *
input_init(struct memory *mem, void *dataPtr);

void
input_add_keys(struct input *inputContext, struct memory *mem, const char *keys[], i32 keyCount);

void
input_bind_key(struct input *inputContext, struct memory *mem, const char *key, input_key_callback cb);

const struct input_key *
input_get_key(struct input *inputContext, const char *key);

const struct input_keybind *
input_get_keybind(struct input *inputContext, const char *key);

b32
input_set_key_down(struct input *inputContext, const char *key, b32 isKeyDown);

b32
input_get_key_down(struct input *inputContext, const char *key);

#endif
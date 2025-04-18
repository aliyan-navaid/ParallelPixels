#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Function pointer typedefs for object lifecycle
typedef void (*destructor)(void *data);
typedef void (*constructor)(void *data);

// Type metadata structure
typedef struct DataType
{
    const char *type_name;
    size_t size;
    destructor destroy;
    constructor create;
} DType;

// Object structure with reference counting
typedef struct
{
    void *data;
    DType type;
    size_t *ref_count;
} Object;

// Function declarations
int is_none(Object obj);
Object let(void *data, DType type);
Object ref(Object obj);
Object copy(Object obj);
void destroy(Object obj);
Object let_none(void);

// Macros
#define None let_none()
#define NoneType let_none().type

#define DEFINE_TYPE(NAME, TYPE, DESTROY_FUNC, CREATE_FUNC)                \
    DType type_##TYPE = {#NAME, sizeof(TYPE), DESTROY_FUNC, CREATE_FUNC}; \
    Object let_##TYPE(TYPE *data) { return let(data, type_##TYPE); }      \
    Object let_v_##TYPE(TYPE data) { return let(&data, type_##TYPE); }    \
    TYPE get_v_##TYPE(Object obj) { return *(TYPE *)obj.data; }           \
    TYPE *get_##TYPE(Object obj) { return (TYPE *)obj.data; }

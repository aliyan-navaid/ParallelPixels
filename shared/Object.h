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

#define DEFINE_TYPE_PROTO(NAME, DTYPE, TYPE) \
    Object let_##NAME(TYPE *data); \
    Object let_##NAME##_v(TYPE data); \
    TYPE* get_##NAME(Object obj); \
    TYPE get_##NAME##_v(Object obj);

#define DEFINE_TYPE(NAME, DTYPE, TYPE) \
    Object let_##NAME(TYPE *data) { return let(data, DTYPE); } \
    Object let_##NAME##_v(TYPE data) { return let(&data, DTYPE); } \
    TYPE* get_##NAME(Object obj) { return (TYPE *)obj.data; } \
    TYPE get_##NAME##_v(Object obj) { return *(TYPE *)obj.data; }

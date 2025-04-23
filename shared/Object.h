#pragma once

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

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

// Support for various essential types
extern DType i8_type;
DEFINE_TYPE_PROTO(i8, i8_type, int8_t)
extern DType i16_type;
DEFINE_TYPE_PROTO(i16, i16_type, int16_t)
extern DType i32_type;
DEFINE_TYPE_PROTO(i32, i32_type, int32_t)
extern DType i64_type;
DEFINE_TYPE_PROTO(i64, i64_type, int64_t)
extern DType f32_type;
DEFINE_TYPE_PROTO(f32, f32_type, float)
extern DType f64_type;
DEFINE_TYPE_PROTO(f64, f64_type, double)
extern DType u8_type;
DEFINE_TYPE_PROTO(u8, u8_type, uint8_t)
extern DType u16_type;
DEFINE_TYPE_PROTO(u16, u16_type, uint16_t)
extern DType u32_type;
DEFINE_TYPE_PROTO(u32, u32_type, uint32_t)
extern DType u64_type;
DEFINE_TYPE_PROTO(u64, u64_type, uint64_t)
extern DType string_type;
DEFINE_TYPE_PROTO(string, string_type, const char *)
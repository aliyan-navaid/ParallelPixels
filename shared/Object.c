#include "Object.h"

int is_none(Object obj) {
    return obj.data == NULL &&
           obj.ref_count == NULL &&
           obj.type.size == 0 &&
           obj.type.destroy == NULL &&
           obj.type.create == NULL;
}

Object let(void *data, DType type) {
    Object obj;
    obj.type = type;

    obj.data = malloc(type.size);
    if (obj.data == NULL) {
        return None;
    }

    if (data != NULL){
        memcpy(obj.data, data, type.size);
    }
    obj.ref_count = (size_t *)malloc(sizeof(size_t));
    if (obj.ref_count == NULL) {
        free(obj.data);
        return None;
    }

    *(obj.ref_count) = 1;

    if (type.create != NULL) {
        type.create(obj.data);
    }

    return obj;
}

Object ref(Object obj) {
    if (is_none(obj)) {
        return obj;
    }
    if (obj.data == NULL) {
        return obj;
    }

    *(obj.ref_count) += 1;
    return obj;
}

Object copy(Object obj) {
    if (is_none(obj)) {
        return obj;
    }
    if (obj.data == NULL) {
        return obj;
    }

    Object new_obj = let(obj.data, obj.type);
    if (new_obj.data == NULL) {
        return new_obj;
    }

    *(new_obj.ref_count) = 1;
    return new_obj;
}

void destroy(Object obj) {
    if (is_none(obj)) {
        return;
    }
    if (obj.data == NULL) {
        return;
    }

    *(obj.ref_count) -= 1;
    if (*(obj.ref_count) == 0) {
        if (obj.type.destroy != NULL) {
            obj.type.destroy(obj.data);
        }
        free(obj.data);
        free(obj.ref_count);
    }
}

Object let_none(void) {
    Object obj;
    obj.data = NULL;
    obj.ref_count = NULL;
    obj.type.size = 0;
    obj.type.destroy = NULL;
    obj.type.create = NULL;
    return obj;
}


// definitions for essential types
DType i8_type = { "i8", sizeof(int8_t), NULL, NULL };
DEFINE_TYPE(i8, i8_type, int8_t)
DType i16_type = { "i16", sizeof(int16_t), NULL, NULL };
DEFINE_TYPE(i16, i16_type, int16_t)
DType i32_type = { "i32", sizeof(int32_t), NULL, NULL };
DEFINE_TYPE(i32, i32_type, int32_t)
DType i64_type = { "i64", sizeof(int64_t), NULL, NULL };
DEFINE_TYPE(i64, i64_type, int64_t)
DType f32_type = { "f32", sizeof(float), NULL, NULL };
DEFINE_TYPE(f32, f32_type, float)
DType f64_type = { "f64", sizeof(double), NULL, NULL };
DEFINE_TYPE(f64, f64_type, double)
DType u8_type = { "u8", sizeof(uint8_t), NULL, NULL };
DEFINE_TYPE(u8, u8_type, uint8_t)
DType u16_type = { "u16", sizeof(uint16_t), NULL, NULL };
DEFINE_TYPE(u16, u16_type, uint16_t)
DType u32_type = { "u32", sizeof(uint32_t), NULL, NULL };
DEFINE_TYPE(u32, u32_type, uint32_t)
DType u64_type = { "u64", sizeof(uint64_t), NULL, NULL };
DEFINE_TYPE(u64, u64_type, uint64_t)

void destroy_string(void* str) {
    free(*(char **)str);
}
void create_string(void* data) {
    const char **str = (const char **)data;
    if (*str != NULL) {
        size_t len = strlen(*str);
        char* new_str = (char *)malloc(len + 1);
        assert(new_str != NULL);
        
        strcpy(new_str, *str);
        *str = new_str;
    }
}

DType string_type = { "string", sizeof(char*), destroy_string, create_string };
DEFINE_TYPE(string, string_type, const char *)
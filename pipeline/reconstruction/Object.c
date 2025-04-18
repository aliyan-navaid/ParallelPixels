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

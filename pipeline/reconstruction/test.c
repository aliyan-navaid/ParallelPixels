#include <stdio.h>
#include <string.h>
#include "Object.h"
#include "dlist.h"
#include "darray.h"
#include "dict.h"

DEFINE_TYPE(int, int, NULL, NULL)
DEFINE_TYPE(char, char, NULL, NULL)

size_t int_hash(Object key) {
    int x = *(int *)key.data;
    return (size_t)(x + (x < 0 ? INT32_MAX : 0));
}

int int_eq(Object key1, Object key2) {
    return *(int *)key1.data == *(int *)key2.data;
}

int main() {
    dict_t dict = dict_init(4, int_hash, int_eq);

    // Insert int -> char mappings
    for (int i = 0; i < 10; i++) {
        char val = 'A' + i;  // A, B, C...
        Object key = let_v_int(i);
        Object value = let_v_char(val);
        dict_insert(&dict, key, value);
    }

    // Retrieve and print them
    for (int i = 0; i < 10; i++) {
        Object key = let_v_int(i);
        Object fallback = let_v_char('?');
        Object value = dict_get(&dict, key, fallback);
        printf("dict[%d] = %c\n", i, *(char *)value.data);
    }

    int del = 3;
    Object deleted = dict_delete(&dict, let_v_int(del));
    printf("Deleted key %d, value was: %c\n", del, *(char *)deleted.data);

    // Try getting deleted key
    Object value = dict_get(&dict, let_v_int(del), let_v_char('?'));
    printf("After deletion, dict[%d] = %c\n", del, *(char *)value.data);

    dict_destroy(&dict);
    return 0;
}
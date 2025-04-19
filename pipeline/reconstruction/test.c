#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // test strdup
    const char *some_string = "\0";
    char *copy = strdup(some_string);

    if (copy == NULL) {
        fprintf(stderr, "strdup failed\n");
        return 1;
    }
    printf("Original string: %s\n", some_string);
    printf("Copied string: %s\n", copy);
    printf("Length of original string: %zu\n", strlen(some_string));
    printf("Length of copied string: %zu\n", strlen(copy));
    printf("Original string address: %p\n", (void *)some_string);
    printf("Copied string address: %p\n", (void *)copy);

    return 0;
}
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <city.h>

int main() {
    const char *my_name = "Muhammad Farooq";
    uint64_t hash = CityHash64(my_name, strlen(my_name));
    printf("Hash of '%s': %lu\n", my_name, hash);
    printf("Hash of '%s': %lu\n", my_name, CityHash64(my_name, strlen(my_name)));

    my_name = "muhammad farooq";
    printf("Hash of '%s': %lu\n", my_name, CityHash64(my_name, strlen(my_name)));

    return 0;
}
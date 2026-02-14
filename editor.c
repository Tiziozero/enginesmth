#include "editor.h"
#include <stdio.h>
int handle_input(char* keys, size_t count, void* payload) {
    for (size_t i = 0; i < count; i++) {
        printf("Key %c\n", keys[i]);
    }
    return 1;
}
int draw(char* buf, size_t width, size_t height, void* payload) {
    *buf = 'a';
    return 1;
}

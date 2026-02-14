#ifndef EDITOR_H
#define EDITOR_H
#include <stdlib.h>
int handle_input(char* keys, size_t count, void* payload);
int draw(char* buf, size_t width, size_t height, void* payload);
#endif // EDITOR_H


#ifndef COMON_H
#define COMON_H
#include "raylib.h"
#include "vterm.h"
typedef struct {
    int code;
    Color bg;
    Color fg;
    int has_bg; // if not the use default
    int has_fg;
    VTermScreenCellAttrs attr;
} Cell;
#endif // COMON_H

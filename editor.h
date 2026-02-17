#ifndef EDITOR_H
#define EDITOR_H
#include <stdlib.h>

#include "common.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_KEYS 512

typedef struct Program Program;

typedef struct {
    int key;        // KEY_A, KEY_B, KEY_UP, etc.
    bool shift;
    bool ctrl;
    bool alt;
} KeyEvent;

typedef struct {
    KeyEvent events[MAX_KEYS];
    int count;
} KeyEventList;

// Internal static array to track previous key state
static bool key_prev[MAX_KEYS] = {0};

// Call this each frame to get newly pressed keys
static inline KeyEventList get_key_events(void) {
    KeyEventList list = { .count = 0 };

    for (int k = 0; k < MAX_KEYS; k++) {
        bool down = IsKeyDown(k);

        if (down && !key_prev[k]) {
            // Key was just pressed
            KeyEvent ev;
            ev.key   = k;
            ev.shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            ev.ctrl  = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
            ev.alt   = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

            list.events[list.count++] = ev;
            if (list.count >= MAX_KEYS) break;
        }

        // Update previous state
        key_prev[k] = down;
    }

    return list;
}

void* start(size_t w, size_t h);
int resize(void* payload, size_t w, size_t h);
int handle_input(KeyEventList kl, void* payload);
int draw(Cell* buf, size_t width, size_t height, void* payload);
#endif // EDITOR_H


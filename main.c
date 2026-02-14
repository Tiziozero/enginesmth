#include <time.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "raylib.h"
#include "editor.h"
// colors are RGBA

typedef struct {
    clock_t start;
    double duration;
    void* payload;
    void (*f)(void*);
} TimeOut;
typedef struct {
    char* screen;
    size_t screen_capacity;
    size_t width; // columns
    size_t height;
    size_t swidth; // screen
    size_t sheight;
    size_t cwidth; // char
    size_t cheight;
    Texture2D bg;
    Texture2D font;
    TimeOut** timeouts; // timeout ptr array
    size_t timeouts_cap;
} Screen;

void LoadFontTexture(Screen* s, const char *path) {
    s->font = LoadTexture(path);
}
void DrawAsciiChar(char c, size_t x, size_t y, Screen* s) {
    if (c == 0) c = ' ';
    else if (c < 32 || c > 126) {
        c = '?';
    } else {
        // printf("Char %c\n", c);
    }

    int index = c - 32;

    Rectangle source;
    source.x = index * s->cwidth;   // x in texture
    source.y = 0.0f;            // y in texture (single row)
    source.width = s->cwidth;
    source.height = s->cheight;

    Rectangle dest = {
        (float)x,
        (float)y,
        s->cwidth,
        s->cheight
    };

    // printf("%c %d %f %f\n", c, index, source.x, source.y);
    DrawTexturePro(s->font, source, dest, (Vector2){0, 0}, 0.0f, WHITE);
}

void draw_bg(Screen* s, Shader shader) {
    double screen_ratio = (double)s->width/(double)s->height;
    double image_ratio = (double)s->bg.width/(double)s->bg.height;
    Rectangle source;
    source.width = s->bg.width;
    source.height = s->bg.height;
    source.x = 0;
    source.y = 0;
    Rectangle dest;
    float width, height, x, y;
    x = y = 0;
    if (image_ratio > screen_ratio) { // centre height, fit width
        double mltp = (double)s->swidth/(double)s->bg.width;
        width = s->bg.width * mltp;
        height = s->bg.height * mltp;
        y = (float)s->sheight / 2 - height/2;
    } else { // centre width, fit height
        double mltp = (double)s->sheight/(double)s->bg.height;
        width = s->bg.width * mltp;
        height = s->bg.height * mltp;
        x = (float)s->swidth / 2 - width / 2;
    }
    dest.width = width;
    dest.height = height;
    dest.x = x;
    dest.y = y;
    // printf("fit %zu/%d=%f rect = %f %f\n",s->swidth, s->bg.width, mltp, x, y);
    BeginShaderMode(shader);
    DrawTexturePro(s->bg, source, dest, (Vector2){0,0}, 0.0f, (Color){0xff,0xff,0xff,0x33});
    EndShaderMode();
}

char* files[] = {"bg.png"};

void load_texture(Texture2D* txtr, const char *path) {
    *txtr = LoadTexture(path);
}
int img_i = 0;
int img_size = sizeof(files)/sizeof(files[0]);


int screen_add_timeout(Screen* s, TimeOut t) {
    t.start = clock();
    TimeOut* tp = malloc(sizeof(TimeOut));
    if (!tp) {
        printf("Failed to allcoate memory\n");
        return 0;
    }
    *tp = t;
    for (size_t i = 0; i < s->timeouts_cap; i++) {
        // if there's a free slot, set to tp
        if (s->timeouts[i] == 0) {
            printf("New timeout (%zu) of %lf seconds.\n", (size_t)tp, tp->duration);
            s->timeouts[i] = tp;
            return 1;
        }
    }
    // realloc + 10
    s->timeouts = realloc(s->timeouts, (s->timeouts_cap+10) * sizeof(TimeOut*));
    if (!s->timeouts) {
        printf("Failed to realloc timeouts.");
        return 0;
    }
    // set those 10 to 0
    memset(s->timeouts + s->timeouts_cap, 0, 10*sizeof(TimeOut));
    // set next timeout as first next free
    s->timeouts[s->timeouts_cap] = tp;
    // increase cap by 10
    s->timeouts_cap+= 10;
    return 1;
}

void bg_handler(void* data) {
    printf("bg handler called.");
    Screen* s = (Screen*)data;
    char* path = files[img_i++ % img_size];
    UnloadTexture(s->bg);
    printf("Unloaded texture.\n");
    load_texture(&s->bg, path);
    TimeOut t;
    t.payload = data;
    t.f = bg_handler;
    t.duration = 1.0; // seconds
    screen_add_timeout(s, t);
}

void process_input() {
    char buffer[64];     // max keys per frame
    size_t count = 0;

    int key = GetCharPressed();

    while (key > 0 && count < sizeof(buffer)) {
        buffer[count++] = (char)key;
        key = GetCharPressed();
    }

    if (count > 0) {
        handle_input(buffer, count, NULL);
    }
}

int main(void) {
    Screen s;
    s.swidth = 800;
    s.sheight = 600;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(s.swidth, s.sheight, "Hello Raylib");

    // init screen
    s.cwidth = 16;
    s.cheight = 20;
    s.width = s.swidth/s.cwidth;
    s.height = s.sheight/s.cheight;
    s.screen_capacity = s.width*s.height;
    s.screen = malloc(s.screen_capacity);
    memset(s.screen, 0, s.width*s.height);

    // init timeouts
    s.timeouts = malloc(10*sizeof(TimeOut*));
    memset(s.timeouts, 0,10*sizeof(TimeOut*)); 
    s.timeouts_cap = 10;
    // load font and bg
    // 16x20
    // 95.0 glyphs
    LoadFontTexture(&s, "font.png");
    load_texture(&s.bg, "bg.png");

    // shader
    Shader shader = LoadShader(0, "fragments.fs");
    int satLoc = GetShaderLocation(shader, "saturation");

    float saturation = 1.7f;
    SetShaderValue(shader, satLoc, &saturation, SHADER_UNIFORM_FLOAT);

    // test timeout
    TimeOut bg;
    bg.duration = 1;
    bg.payload = &s;
    bg.f = bg_handler;
    screen_add_timeout(&s, bg);

    while (!WindowShouldClose()) {
        // timeouts first ig.
        clock_t now = clock();
        for (size_t i = 0; i < s.timeouts_cap; i++) {
            if (!s.timeouts[i]) continue;
            TimeOut* t = s.timeouts[i];
            // timeout triggered
            if ((double)(now - t->start) / CLOCKS_PER_SEC >= t->duration) {
                t->f(t->payload);
                free(t); // free after use
                s.timeouts[i] = 0; // set to null to signal it's free
            } else {
            }
        }
        // handle resize
        if (IsWindowResized()) {
            printf("Resized: %d x %d\n", GetScreenWidth(), GetScreenHeight());
            int w = GetScreenWidth();
            int h = GetScreenHeight();

            size_t new_width = w / s.cwidth;
            size_t new_height = h / s.cheight;
            s.width = new_width;
            s.height = new_height;
            int snappedW = (new_width) * s.cwidth;
            int snappedH = (new_height) * s.cheight;
            // if it retriggers a resize
            if (w != snappedW || h != snappedH) {
                SetWindowSize(snappedW, snappedH);
            }
            s.swidth = snappedW;
            s.sheight = snappedH;
            // make sure fuckass screen buffers ok
            if (s.screen_capacity < s.width * s.height) {
                s.screen_capacity = s.width * s.height;
                s.screen = realloc(s.screen, s.screen_capacity);
            }
        }
        // input
        process_input();
        // draw
        memset(s.screen, 0, s.screen_capacity);
        draw(s.screen, s.width, s.height, NULL);
        BeginDrawing();
        ClearBackground(BLACK);
        draw_bg(&s, shader);
        for (size_t i = 0; i < s.width*s.height; i++) {
            size_t x = i % s.width;
            size_t y = i / s.width;
            DrawAsciiChar(s.screen[i],x*s.cwidth,y*s.cheight, &s);
            if (s.screen[i] != 0) {
                DrawPixel(x*s.cwidth, y*s.cheight, WHITE);
            }
        }
        EndDrawing();
    }
    printf("end\n");
    return 0;
}

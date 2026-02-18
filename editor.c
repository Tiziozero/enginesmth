#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "vterm.h"
#include <stdio.h>
#include <stdlib.h>
#include "commands.h"


#include <pty.h>
struct Program {
    int master_fd;
    int input_fd;
    VTerm* vt;
    VTermScreen *screen;
    size_t rows, cols;
};
void* start(size_t w, size_t h) {

    int master;
    Program* p = malloc(sizeof(Program));
    pid_t pid = forkpty(&master, NULL, NULL, NULL);

    if (pid == 0) {
        // child — this IS vim
        // execlp("./test", "vim", "main.txt", NULL);
        execlp("zsh", "zsh", NULL);
        perror("execlp failed");  // only reached if exec failed
        exit(1);
        return 0;
    }
    // after opening master_fd in start():
    fcntl(master, F_SETFL, O_NONBLOCK);
    // int fd = open("/dev/input/event4", O_RDONLY | O_NONBLOCK);
    // if (!fd) {
        // exit(1);
    // }

    p->master_fd = master;
    // p->input_fd = fd;
    p->rows = h;
    p->cols = w;
    printf("New vterm %zu %zun", w, h);

    p->vt = vterm_new(p->rows, p->cols);
    vterm_set_utf8(p->vt, 1);  // Vim uses UTF-8

    p->screen = vterm_obtain_screen(p->vt);
    vterm_screen_reset(p->screen, 1);
    // p->screen = vterm_obtain_screen(p->vt);

    return p;
}

int resize(void* payload, size_t w, size_t h) {
    fflush(stdout);
    Program* p = payload;
    p->rows = h;
    p->cols = w;
    vterm_set_size(p->vt, (int)p->rows, (int)p->cols);
    printf("resize to %zu %zu\n", w, h);
    fflush(stdout);
    // critical — vim reads this to know its terminal size
    struct winsize ws = {
        .ws_row = h,
        .ws_col = w,
    };
    ioctl(p->master_fd, TIOCSWINSZ, &ws);
    return 1;
}
#include "raylib.h"
#include "vterm.h"

// Returns 1 if handled, 0 if not
int raylib_key_to_vterm(VTerm* vt, Program* p) {
    int key = GetKeyPressed();
    if (key == 0) return 0;
    printf("Key %d\n", key);

    VTermModifier mod = VTERM_MOD_NONE;

    // Check modifiers
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

    if (shift) mod |= VTERM_MOD_SHIFT;
    if (ctrl)  mod |= VTERM_MOD_CTRL;
    if (alt)   mod |= VTERM_MOD_ALT;
    
    // printf("a %d f7 %d\n", KEY_A, KEY_F7); return 0;
    // Special keys that vterm knows about
    // Handle Ctrl+letter combinations
    // Letter keys with modifiers - loop through A-Z
    for (int key = KEY_A; key <= KEY_Z; key++) {
        if (IsKeyPressed(key)) {
            int letter_index = key - KEY_A;  // 0-25
            
            if (ctrl && alt) {
                // Ctrl+Alt+letter: ESC + ctrl code
                char seq[2] = {27, letter_index + 1};
                write(p->master_fd, seq, 2);
                return 1;
            } else if (alt && shift) {
                // Alt+Shift+letter: ESC + uppercase
                char seq[2] = {27, 'A' + letter_index};
                write(p->master_fd, seq, 2);
                return 1;
            } else if (alt) {
                // Alt+letter: ESC + lowercase
                char seq[2] = {27, 'a' + letter_index};
                write(p->master_fd, seq, 2);
                return 1;
            } else if (ctrl) {
                // Ctrl+letter: just the control code
                char c = letter_index + 1;
                write(p->master_fd, &c, 1);
                return 1;
            }
        }
    }
    switch (key) {
        case KEY_ENTER:      printf("KEY_ENTER\n");vterm_keyboard_key(vt, VTERM_KEY_ENTER, mod); return 1;
        case KEY_TAB:        printf("KEY_TAB\n");vterm_keyboard_key(vt, VTERM_KEY_TAB, mod); return 1;
        case KEY_BACKSPACE:  printf("KEY_BACKSPACE\n");vterm_keyboard_key(vt, VTERM_KEY_BACKSPACE, mod); return 1;
        case KEY_ESCAPE:     printf("KEY_ESCAPE\n");vterm_keyboard_key(vt, VTERM_KEY_ESCAPE, mod); return 1;
        case KEY_UP:         printf("KEY_UP\n");vterm_keyboard_key(vt, VTERM_KEY_UP, mod); return 1;
        case KEY_DOWN:       printf("KEY_DOWN\n");vterm_keyboard_key(vt, VTERM_KEY_DOWN, mod); return 1;
        case KEY_LEFT:       printf("KEY_LEFT\n");vterm_keyboard_key(vt, VTERM_KEY_LEFT, mod); return 1;
        case KEY_RIGHT:      printf("KEY_RIGHT\n");vterm_keyboard_key(vt, VTERM_KEY_RIGHT, mod); return 1;
        case KEY_HOME:       printf("KEY_HOME\n");vterm_keyboard_key(vt, VTERM_KEY_HOME, mod); return 1;
        case KEY_END:        printf("KEY_END\n");vterm_keyboard_key(vt, VTERM_KEY_END, mod); return 1;
        case KEY_PAGE_UP:    printf("KEY_PAGE_UP\n");vterm_keyboard_key(vt, VTERM_KEY_PAGEUP, mod); return 1;
        case KEY_PAGE_DOWN:  printf("KEY_PAGE_DOWN\n");vterm_keyboard_key(vt, VTERM_KEY_PAGEDOWN, mod); return 1;
        case KEY_INSERT:     printf("KEY_INSERT\n");vterm_keyboard_key(vt, VTERM_KEY_INS, mod); return 1;
        case KEY_DELETE:     printf("KEY_DELETE\n");vterm_keyboard_key(vt, VTERM_KEY_DEL, mod); return 1;
        
        // Function keys
        case KEY_F1:  printf("KEY_F1\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(1), mod); return 1;
        case KEY_F2:  printf("KEY_F2\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(2), mod); return 1;
        case KEY_F3:  printf("KEY_F3\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(3), mod); return 1;
        case KEY_F4:  printf("KEY_F4\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(4), mod); return 1;
        case KEY_F5:  printf("KEY_F5\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(5), mod); return 1;
        case KEY_F6:  printf("KEY_F6\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(6), mod); return 1;
        case KEY_F7:  printf("KEY_F7\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(7), mod); return 1;
        case KEY_F8:  printf("KEY_F8\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(8), mod); return 1;
        case KEY_F9:  printf("KEY_F9\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(9), mod); return 1;
        case KEY_F10: printf("KEY_F10\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(10), mod); return 1;
        case KEY_F11: printf("KEY_F11\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(11), mod); return 1;
        case KEY_F12: printf("KEY_F12\n");vterm_keyboard_key(vt, VTERM_KEY_FUNCTION(12), mod); return 1;
    }
    // Character input
    int c = GetCharPressed();
    while (c > 0) {
        // vterm_keyboard_key(vt, c, mod);
        vterm_keyboard_unichar(vt, c, mod);
        c = GetCharPressed();
        continue;
        // If Ctrl is held and it's a letter, send the control character directly
        if (ctrl && c >= 'a' && c <= 'z') {
            // Ctrl+A = 1, Ctrl+B = 2, ..., Ctrl+Z = 26
            vterm_keyboard_unichar(vt, c - 'a' + 1, VTERM_MOD_NONE);
            printf("control.\n");
        } else if (ctrl && c >= 'A' && c <= 'Z') {
            vterm_keyboard_unichar(vt, c - 'A' + 1, VTERM_MOD_NONE);
        } else {
            // Normal character
            vterm_keyboard_unichar(vt, c, VTERM_MOD_NONE);
        }
        c = GetCharPressed();
    }
    return 1;
    
    // Normal character input — use GetCharPressed for proper text
    int _c = GetCharPressed();
    if (key == KEY_F && c == 'f') {
        printf("match\n");
    }
    if (c > 0) {
        printf("C  %c\n", c);
        vterm_keyboard_unichar(vt, c, mod);
    }
    
    return 1;
}
int handle_input(KeyEventList kl, void* payload) {
    Program* p = payload;
    while (raylib_key_to_vterm(p->vt, p)) {
        char out[256];
        size_t len = vterm_output_read(p->vt, out, sizeof(out));
        if (len > 0) {
            printf("Sending %zu bytes to nvim: ", len);
            for (size_t i = 0; i < len; i++) {
                if (out[i] >= 32 && out[i] < 127) {
                    printf("'%c' ", out[i]);
                } else {
                    printf("0x%02x ", (unsigned char)out[i]);
                }
            }
            printf("\n");

            write(p->master_fd, out, len);
        }
    }
    fflush(stdout);
    int master = p->master_fd;
    static char* buf;
    static size_t capacity;
    if (!buf) {
        capacity = 1024;
        buf = malloc(1025);
    }

    fd_set fds;
    while (1) {
        FD_ZERO(&fds);
        FD_SET(p->master_fd, &fds);
        // FD_SET(p->input_fd,  &fds);

        fflush(stdout);
        // non-blocking poll — return right away if nothing ready
        struct timeval tv = {0, 0};
        select(p->master_fd + 1, &fds, NULL, NULL, &tv);
        fflush(stdout);

        if (FD_ISSET(p->master_fd, &fds)) {
    fflush(stdout);
            // vim wrote output — feed to vterm
            char buf[4096];
            ssize_t n = read(p->master_fd, buf, sizeof(buf));
            if (n > 0) {
                fflush(stdout);
                vterm_input_write(p->vt, buf, n);
                fflush(stdout);
                vterm_screen_flush_damage(p->screen);
                fflush(stdout);
            }
        }

        /*if (FD_ISSET(p->input_fd, &fds)) {
            fflush(stdout);
            // you got a hardware input event — translate and send to vim
            struct input_event ev;
            read(p->input_fd, &ev, sizeof(ev));
            if (ev.type == EV_KEY && ev.value == 1) {  // key down
                                                       // translate ev.code → vterm key, then:
                char c = 'c';
                vterm_keyboard_unichar(p->vt, c, VTERM_MOD_NONE);
                // flush vterm's encoded output to vim's pty
                char out[64];
                size_t len = vterm_output_read(p->vt, out, sizeof(out));
                if (len > 0) write(p->master_fd, out, len);
                fflush(stdout);
            }
        }*/
        fflush(stdout);
        break;
    }
        fflush(stdout);
    return 1;
}
Color vterm_indexed_to_color(uint8_t idx) {
    // Standard 16 colors (0-15)
    static const Color ansi_colors[16] = {
        // Normal colors (0–7)
        {0x1E, 0x1E, 0x2E, 0xFF},  // 0: Black (soft dark, not pure black)
        {0xE6, 0x45, 0x53, 0xFF},  // 1: Red (soft coral)
        {0x89, 0xD9, 0x4A, 0xFF},  // 2: Green (fresh lime)
        {0xF5, 0xC2, 0x58, 0xFF},  // 3: Yellow (warm amber)
        {0x5A, 0xA9, 0xE6, 0xFF},  // 4: Blue (clean sky blue)
        {0xC6, 0x78, 0xDD, 0xFF},  // 5: Magenta (soft purple)
        {0x56, 0xB6, 0xC2, 0xFF},  // 6: Cyan (muted teal)
        {0xD4, 0xD4, 0xD4, 0xFF},  // 7: White (light gray, not harsh white)

        // Bright colors (8–15)
        {0x6C, 0x70, 0x8A, 0xFF},  // 8: Bright Black (cool gray)
        {0xFF, 0x6E, 0x6E, 0xFF},  // 9: Bright Red
        {0xA6, 0xE3, 0xA1, 0xFF},  // 10: Bright Green
        {0xF9, 0xE2, 0xAF, 0xFF},  // 11: Bright Yellow
        {0x89, 0xB4, 0xFA, 0xFF},  // 12: Bright Blue
        {0xF5, 0xC2, 0xE7, 0xFF},  // 13: Bright Magenta
        {0x94, 0xE2, 0xD5, 0xFF},  // 14: Bright Cyan
        {0xF5, 0xF5, 0xF5, 0xFF},  // 15: Bright White (soft white)
    };

    
    // 0-15: Standard ANSI colors
    if (idx < 16) {
        return ansi_colors[idx];
    }
    
    // 16-231: 6x6x6 RGB cube
    if (idx >= 16 && idx <= 231) {
        int cube_idx = idx - 16;
        int r = (cube_idx / 36) % 6;
        int g = (cube_idx / 6) % 6;
        int b = cube_idx % 6;
        
        // Map 0-5 to 0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF
        const uint8_t levels[6] = {0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF};
        
        return (Color){levels[r], levels[g], levels[b], 0xFF};
    }
    
    // 232-255: Grayscale ramp
    if (idx >= 232 && idx <= 255) {
        uint8_t gray = 8 + (idx - 232) * 10;
        return (Color){gray, gray, gray, 0xFF};
    }
    
    // Fallback (shouldn't happen)
    return (Color){0xFF, 0x00, 0xFF, 0xFF};  // Magenta for "error"
}
int draw(Cell* buf, size_t width, size_t height, void* payload) {
    Program* p = payload;
    int rows, cols;
    if (width != p->cols || height != p->rows) {
        printf("state != prog\n");
        printf("%zu %zu : %zu %zu\n", width, height, p->cols, p->rows);
        // exit(1);
    }
    rows = p->rows;
    cols = p->cols;
    // clamp to what actually fits — vterm may not have resized yet
    // rows = MIN((int)height, (int)p->rows);
    // cols = MIN((int)width,  (int)p->cols);
    // printf("Drawing %d %d\n", cols, rows);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            VTermPos pos;
            pos.row = r;
            pos.col = c;
            VTermScreenCell cell;
            if (!vterm_screen_get_cell(p->screen, pos, &cell)) {
                printf("Failed to get cell %d %d (c:%d r:%d) (given %zu:%zu) (prog %zu:%zu).\n",c,r, cols, rows, width, height, p->cols, p->rows);
                buf[r*width + c].code = 'x';
                continue;
            }
            Cell* _c = &(buf[r*width + c]);
            _c->code = cell.chars[0];
            _c->attr = cell.attrs;
            _c->has_bg = 1;
            if (cell.bg.type & VTERM_COLOR_DEFAULT_BG) {
                _c->has_bg = 0;
            }
            _c->has_fg = 1;
            if (cell.bg.type & VTERM_COLOR_DEFAULT_FG) {
                _c->has_fg = 0;
            }

            if (_c->has_bg) {
                if (cell.bg.type == VTERM_COLOR_RGB) {
                    _c->bg.r = cell.bg.rgb.red;
                    _c->bg.g = cell.bg.rgb.green;
                    _c->bg.b = cell.bg.rgb.blue;
                    _c->bg.a = 0xff;
                    // printf("%.2x %.2x %.2x %.2x rgba\n", _c->bg.r, _c->bg.g,_c->bg.b,_c->bg.a);
                } else if (cell.bg.type == VTERM_COLOR_DEFAULT_BG) {
                    _c->bg.r = 0xff;
                    _c->bg.g = 0xff;
                    _c->bg.b = 0xff;
                    _c->bg.a = 0xff;
                    // printf("(default)%.2x %.2x %.2x %.2x rgba\n", _c->bg.r, _c->bg.g,_c->bg.b,_c->bg.a);
                } else if (cell.bg.type == VTERM_COLOR_INDEXED) {
                    _c->bg = vterm_indexed_to_color(cell.bg.indexed.idx);
                } else {
                printf("bg not implemented."); exit(1);
                }
            }
            _c->has_fg = 1;
            if(cell.fg.type == VTERM_COLOR_RGB) { 
                _c->fg.r = cell.fg.rgb.red;
                _c->fg.g = cell.fg.rgb.green;
                _c->fg.b = cell.fg.rgb.blue;
                _c->fg.a = 0xff;
                // printf("(rgba)%.2x %.2x %.2x %.2x rgba\n", _c->fg.r, _c->fg.g,_c->fg.b,_c->fg.a);
            } else if (cell.fg.type == VTERM_COLOR_DEFAULT_FG) {
                _c->fg.r = 0xff;
                _c->fg.g = 0xff;
                _c->fg.b = 0xff;
                _c->fg.a = 0xff;
            } else if (cell.fg.type == VTERM_COLOR_INDEXED) {
                _c->fg = vterm_indexed_to_color(cell.fg.indexed.idx);
            } else {
                printf("fg not implemented."); exit(1);
            }
        }
    }
    VTermPos cursor_pos;
    VTermState* state = vterm_obtain_state(p->vt);
    vterm_state_get_cursorpos(state, &cursor_pos);
    buf[cursor_pos.col + width * cursor_pos.row].bg =GetColor(0x777777ff);
    buf[cursor_pos.col + width * cursor_pos.row].has_bg = 1;
    buf[cursor_pos.col + width * cursor_pos.row].has_fg = 1;
    buf[cursor_pos.col + width * cursor_pos.row].fg = GetColor(0x080808ff);
    // printf("CUrsor %d %d\n", cursor_pos.col, cursor_pos.row);

    return 1;
}

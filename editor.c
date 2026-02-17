#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input.h>
#include <fcntl.h>
#include "vterm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>


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
        execlp("nvim", "nvim", "main.txt", NULL);
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
int raylib_key_to_vterm(VTerm* vt) {
    int key = GetKeyPressed();
    if (key == 0) return 0;
    
    VTermModifier mod = VTERM_MOD_NONE;
    
    // Check modifiers
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
        mod |= VTERM_MOD_SHIFT;
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        mod |= VTERM_MOD_CTRL;
    if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT))
        mod |= VTERM_MOD_ALT;
    
    printf("Key %d\n", key);
    // Special keys that vterm knows about
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
    
    KEY_F;
    // Normal character input — use GetCharPressed for proper text
    int c = GetCharPressed();
    if (key == KEY_F && c == 'f') {
        printf("match\n");
    }
    while (c > 0) {
        printf("C  %c\n", c);
        vterm_keyboard_unichar(vt, c, mod);
        c = GetCharPressed();
    }
    
    return 1;
}
int handle_input(KeyEventList kl, void* payload) {
    Program* p = payload;
    while (raylib_key_to_vterm(p->vt)) {
        char out[256];
        size_t len = vterm_output_read(p->vt, out, sizeof(out));
        if (len > 0) {
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
        select(MAX(p->master_fd, 0) + 1, &fds, NULL, NULL, &tv);
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
    rows = MIN((int)height, (int)p->rows);
    cols = MIN((int)width,  (int)p->cols);
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
                // exit(1);
            }
            buf[r*width + c].code = cell.chars[0];
        }
    }

    return 1;
}

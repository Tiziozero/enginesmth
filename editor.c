#include "editor.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    enum { ORIGINAL, ADD } source;
    size_t offset, length;
} Piece;
typedef struct {
    char* source, *add_buf;
    Piece* pieces;
    size_t count, cap;
} PieceTable;
typedef struct {
    Vector2 cursor_pos;
    size_t position;
    PieceTable pt;
    char* current_buf;
    size_t cur_buf_count, cur_buf_cap;
    size_t line_draw_offset_x, line_draw_offset_y;
    enum {INPUT, NORMAL, VISUAL} mode;
} ProgramState;

PieceTable new_buf(char* src, size_t size) {
    PieceTable p;
    p.source = src;
    p.add_buf = (char*)malloc(1024*sizeof(char));
    p.add_buf[0] = '\0'; // Initialize as empty string
    p.pieces = (Piece*)malloc(1024*sizeof(Piece));
    p.count = 0;
    p.cap = 1024;
    
    if (strlen(src) > 0) {
        Piece piece_0;
        piece_0.source = ORIGINAL;
        piece_0.offset = 0;
        piece_0.length = size;
        p.pieces[0] = piece_0;  // Add this line!
        p.count = 1;             // And this line!
    }
    
    return p;
}
// Helper function to get total length
size_t get_length(PieceTable* pt) {
    size_t len = 0;
    for (size_t i = 0; i < pt->count; i++) {
        len += pt->pieces[i].length;
    }
    return len;
}
// Helper function to find which piece contains a given position
static int find_piece(PieceTable* pt, size_t pos, size_t* piece_offset) {
    size_t current_pos = 0;
    for (size_t i = 0; i < pt->count; i++) {
        if (current_pos + pt->pieces[i].length > pos) {
            *piece_offset = pos - current_pos;
            return i;
        }
        current_pos += pt->pieces[i].length;
    }
    return -1; // Position out of bounds
}

// Helper function to ensure capacity
static void ensure_capacity(PieceTable* pt) {
    if (pt->count >= pt->cap) {
        pt->cap *= 2;
        // printf("readlloc %zu\n", pt->cap * sizeof(Piece));
        pt->pieces = (Piece*)realloc(pt->pieces, pt->cap * sizeof(Piece));
    }
}

// Add text at a given position
int add(PieceTable* pt, size_t pos, const char* text, size_t text_len) {
    if (!text || text_len == 0) return -1;
    
    // Append text to add_buf
    size_t add_buf_len = strlen(pt->add_buf);
    // printf("readlloc %zu\n", add_buf_len + text_len + 1);
    pt->add_buf = (char*)realloc(pt->add_buf, add_buf_len + text_len + 1);
    memcpy(pt->add_buf + add_buf_len, text, text_len);
    pt->add_buf[add_buf_len + text_len] = '\0';
    
    // Handle insertion at the end
    if (pos >= get_length(pt)) {
        ensure_capacity(pt);
        Piece new_piece = {ADD, add_buf_len, text_len};
        pt->pieces[pt->count++] = new_piece;
        return 0;
    }
    
    // Find the piece containing the position
    size_t piece_offset;
    int piece_idx = find_piece(pt, pos, &piece_offset);
    if (piece_idx < 0) return -1;
    
    // Split the piece if necessary
    Piece* piece = &pt->pieces[piece_idx];
    
    if (piece_offset == 0) {
        // Insert before this piece
        ensure_capacity(pt);
        memmove(&pt->pieces[piece_idx + 1], &pt->pieces[piece_idx], 
                (pt->count - piece_idx) * sizeof(Piece));
        Piece new_piece = {ADD, add_buf_len, text_len};
        pt->pieces[piece_idx] = new_piece;
        pt->count++;
    } else if (piece_offset == piece->length) {
        // Insert after this piece
        ensure_capacity(pt);
        memmove(&pt->pieces[piece_idx + 2], &pt->pieces[piece_idx + 1], 
                (pt->count - piece_idx - 1) * sizeof(Piece));
        Piece new_piece = {ADD, add_buf_len, text_len};
        pt->pieces[piece_idx + 1] = new_piece;
        pt->count++;
    } else {
        // Split the piece in the middle
        ensure_capacity(pt);
        ensure_capacity(pt);
        
        Piece left = {piece->source, piece->offset, piece_offset};
        Piece middle = {ADD, add_buf_len, text_len};
        Piece right = {piece->source, piece->offset + piece_offset, 
                       piece->length - piece_offset};
        
        memmove(&pt->pieces[piece_idx + 3], &pt->pieces[piece_idx + 1], 
                (pt->count - piece_idx - 1) * sizeof(Piece));
        pt->pieces[piece_idx] = left;
        pt->pieces[piece_idx + 1] = middle;
        pt->pieces[piece_idx + 2] = right;
        pt->count += 2;
    }
    
    return 0;
}

// Delete text from start_pos to end_pos (exclusive)
int delete(PieceTable* pt, size_t start_pos, size_t end_pos) {
    if (start_pos >= end_pos) return -1;
    
    size_t start_piece_offset, end_piece_offset;
    int start_idx = find_piece(pt, start_pos, &start_piece_offset);
    int end_idx = find_piece(pt, end_pos - 1, &end_piece_offset);
    
    if (start_idx < 0 || end_idx < 0) return -1;
    
    end_piece_offset++; // Make it exclusive
    
    if (start_idx == end_idx) {
        // Deletion within a single piece
        Piece* piece = &pt->pieces[start_idx];
        
        if (start_piece_offset == 0 && end_piece_offset == piece->length) {
            // Delete entire piece
            memmove(&pt->pieces[start_idx], &pt->pieces[start_idx + 1],
                    (pt->count - start_idx - 1) * sizeof(Piece));
            pt->count--;
        } else if (start_piece_offset == 0) {
            // Delete from start
            piece->offset += end_piece_offset;
            piece->length -= end_piece_offset;
        } else if (end_piece_offset == piece->length) {
            // Delete to end
            piece->length = start_piece_offset;
        } else {
            // Delete from middle - split into two pieces
            ensure_capacity(pt);
            
            Piece left = {piece->source, piece->offset, start_piece_offset};
            Piece right = {piece->source, piece->offset + end_piece_offset,
                          piece->length - end_piece_offset};
            
            pt->pieces[start_idx] = left;
            memmove(&pt->pieces[start_idx + 2], &pt->pieces[start_idx + 1],
                    (pt->count - start_idx - 1) * sizeof(Piece));
            pt->pieces[start_idx + 1] = right;
            pt->count++;
        }
    } else {
        // Deletion spans multiple pieces
        
        // Trim start piece
        if (start_piece_offset == 0) {
            // Remove entire start piece
        } else {
            pt->pieces[start_idx].length = start_piece_offset;
            start_idx++;
        }
        
        // Trim end piece
        Piece* end_piece = &pt->pieces[end_idx];
        if (end_piece_offset == end_piece->length) {
            // Remove entire end piece
            end_idx++;
        } else {
            end_piece->offset += end_piece_offset;
            end_piece->length -= end_piece_offset;
        }
        
        // Remove pieces in between
        int pieces_to_remove = end_idx - start_idx;
        if (pieces_to_remove > 0) {
            memmove(&pt->pieces[start_idx], &pt->pieces[end_idx],
                    (pt->count - end_idx) * sizeof(Piece));
            pt->count -= pieces_to_remove;
        }
    }
    
    return 0;
}

size_t buf_to_cstr(PieceTable* pt, char** buf_ptr) {
    size_t count, cap;
    count = 0;
    cap = 1024;
    char* buf = malloc(1024);
    for (size_t i = 0; i < pt->count; i++) {
        // printf("Piece %zu ", i);
        Piece p = pt->pieces[i];
        char* source = 0;
        if (p.source == ORIGINAL) {
            source = pt->source;
        } else {
            source = pt->add_buf;
        }
        size_t offset = p.offset, len = p.length;
        // printf(" piece \"%.*s\" ", (int)len, source + offset);
        while (count + len + 1 > cap) {  // +1 for '\0'
            cap *= 2;
            // printf("Reacllo %zu\n", cap);
            char* tmp = realloc(buf, cap);
            if (!tmp) {
                free(buf);
                return 0;
            }
            buf = tmp;
        }
        memcpy(buf + count, source+offset, len);
        count += len;
        // printf("Count %zu\n", count);
    }
    if (count + 1 >= cap) {
        // printf("readlloc %zu\n", cap + 1);
        buf = realloc(buf, cap += 1);
    }
    buf[count] = '\0';
    *buf_ptr = buf;
    return count;
}
void* start() {
    // printf("start\n");
    fflush(stdout);
    ProgramState* ps = malloc(sizeof(ProgramState));
    if (!ps) {
        printf("Failed to create ps.");
        fflush(stdout);
    }
    fflush(stdout);
    ps->position = 0;
    fflush(stdout);
    
    ps->cur_buf_count = 0;
    ps->cur_buf_cap = 1024;
    ps->current_buf = malloc(ps->cur_buf_cap);
    fflush(stdout);
    ps->pt = new_buf("", 0);
    fflush(stdout);
    ps->mode = NORMAL;
    ps->line_draw_offset_x = 0;
    ps->line_draw_offset_y = 0;
    char*s;
    size_t size = buf_to_cstr(&ps->pt, &s);
    fflush(stdout);
    printf("buf \"%.*s\" piece count: %zu\n", (int)size, s, ps->pt.count);
    return ps;
}
int handle_normal_mode(KeyEventList kl, void* payload) {
    ProgramState* ps = (ProgramState*)payload;
    // printf("Key count %d\n", kl.count); 
    for (size_t i = 0; i < kl.count; i++) {
        if (kl.events[i].key == 'I') {
            printf("Input Mode\n");
            ps->mode = INPUT;
        }
        if (kl.events[i].key== 'q') {
            printf("quit");
            exit(0);
        }
    }
    return 0;
}
char keycode_to_char(int key, bool shift, bool ctrl, bool alt) {
    // Ctrl combinations (common shortcuts)
    if (ctrl && !alt) {
        if (key >= KEY_A && key <= KEY_Z) {
            // Ctrl+A through Ctrl+Z = ASCII control codes 1-26
            return (char)(key - KEY_A + 1);
        }
        return 0;
    }
    
    // Alt combinations (typically not printable)
    if (alt) {
        return 0;
    }
    
    // Letters
    if (key >= KEY_A && key <= KEY_Z) {
        char c = (char)(key - KEY_A + 'a');
        if (shift) c -= 32; // uppercase
        return c;
    }
    
    // Numbers and their shifted symbols (UK layout)
    if (key >= KEY_ZERO && key <= KEY_NINE) {
        if (shift) {
            switch(key) {
                case KEY_ONE:   return '!';
                case KEY_TWO:   return '"';   // UK: " not @
                case KEY_THREE: return '#';   // UK: £ not # £ not printable
                case KEY_FOUR:  return '$';
                case KEY_FIVE:  return '%';
                case KEY_SIX:   return '^';
                case KEY_SEVEN: return '&';
                case KEY_EIGHT: return '*';
                case KEY_NINE:  return '(';
                case KEY_ZERO:  return ')';
            }
        }
        return (char)(key - KEY_ZERO + '0');
    }
    
    // Special keys
    if (key == KEY_SPACE) return ' ';
    if (key == KEY_ENTER) return '\n';
    if (key == KEY_TAB) return '\t';
    if (key == KEY_BACKSPACE) return '\b';
    
    // Punctuation and symbols (UK layout differences)
    switch(key) {
        case KEY_MINUS:         return shift ? '_' : '-';
        case KEY_EQUAL:         return shift ? '+' : '=';
        case KEY_LEFT_BRACKET:  return shift ? '{' : '[';
        case KEY_RIGHT_BRACKET: return shift ? '}' : ']';
        case KEY_SEMICOLON:     return shift ? ':' : ';';
        case KEY_APOSTROPHE:    return shift ? '@' : '\'';  // UK: @ on Shift+'
        case KEY_GRAVE:         return shift ? '-' : '`';   // UK: ¬ on Shift+`
        case KEY_BACKSLASH:     return shift ? '~' : '#';  // UK: same position
        case KEY_COMMA:         return shift ? '<' : ',';
        case KEY_PERIOD:        return shift ? '>' : '.';
        case KEY_SLASH:         return shift ? '?' : '/';
        
        // UK-specific: # key (usually next to Enter, might be KEY_BACKSLASH on some systems)
        case 35:                return shift ? '#' : '~';   // # key
    }
    
    return 0;
}
int handle_input(KeyEventList kl, void* payload) {
    ProgramState* ps = (ProgramState*)payload;
    switch (ps->mode) {
        case NORMAL:
            return handle_normal_mode(kl, payload);
        case INPUT:
            for (size_t i = 0; i < kl.count; i++) {
                int kc /* keycode */ = kl.events[i].key;
                if (kc == KEY_LEFT) ps->position--;
                else if (kc == KEY_RIGHT) ps->position++;
                char k = keycode_to_char(kl.events[i].key, kl.events[i].shift,kl.events[i].ctrl,kl.events[i].alt);
                if (k != 0) { // anything printable
                    char key[2] = {k, 0};
                    add(&ps->pt, ps->position, key, 1);
                    ps->position += 1;
                    char*s;
                    size_t size = buf_to_cstr(&ps->pt, &s);
                    printf("buf \"%.*s\" piece count: %zu\n", (int)size, s, ps->pt.count);
                    free(s);
                }
            }
            break;
        case VISUAL: break;
    }
    return 1;
}

typedef struct {
    char* text;      // pointer to start of line in original buffer
    size_t length;   // length of line (excluding \n)
} Line;

typedef struct {
    Line* lines;
    size_t count;
    size_t cap;
} LineArray;

LineArray split_buffer(char* buffer, size_t len) {
    char* copy = malloc(len + 1);
    memcpy(copy, buffer, len);
    copy[len] = '\0';
    // buffer = copy;
    LineArray lines;
    lines.cap = 1024;
    lines.count = 0;
    lines.lines = malloc(lines.cap * sizeof(Line));
    
    char* line_start = buffer;
    char* p = buffer;
    
    while (*p != '\0') {
        if (*p == '\n') {
            // Found end of line
            if (lines.count >= lines.cap) {
                lines.cap *= 2;
                lines.lines = realloc(lines.lines, lines.cap * sizeof(Line));
            }
            
            lines.lines[lines.count].text = line_start;
            lines.lines[lines.count].length = p - line_start;
            lines.count++;
            
            line_start = p + 1;  // Next line starts after \n
        }
        p++;
    }
    
    // Last line (if no trailing newline)
    if (p > line_start) {
        if (lines.count >= lines.cap) {
            lines.cap *= 2;
            lines.lines = realloc(lines.lines, lines.cap * sizeof(Line));
        }
        
        lines.lines[lines.count].text = line_start;
        lines.lines[lines.count].length = p - line_start;
        lines.count++;
    }
    
    free(copy);
    return lines;
}

void free_lines(LineArray* lines) {
    free(lines->lines);
}

int draw(Cell* buf, size_t width, size_t height, void* payload) {
    ProgramState* ps = (ProgramState*)payload;
    char*s;
    size_t size = buf_to_cstr(&ps->pt, &s);
    printf("buf \"%.*s\" piece count: %zu\n", (int)size, s, ps->pt.count);
    LineArray lines = split_buffer(s, size);
    printf("after lines (%zu) (buflen %zu)\n", lines.count, size);
    fflush(stdout);
    if (lines.count == 0) return 1;
    size_t x = ps->line_draw_offset_x, y = ps->line_draw_offset_y;
    LineArray draw_lines = lines;
    if (y > draw_lines.count) {
        y = draw_lines.count - 1;
        ps->line_draw_offset_y = y;
    }
    draw_lines.count -= y;
    draw_lines.lines += y*sizeof(Line);
    printf("y = %zu\n", y);

    for (size_t i = 0; i < draw_lines.count; i++) {
        Line line = draw_lines.lines[i];
        printf("%.*s \n", (int)(
                line.length > width ? width : line.length),line.text);
        for (size_t j = 0; j < 
                (line.length > width ? width : line.length); j++) {
            buf[i].code = line.text[i];
        }
    }

    free_lines(&lines);
    free(s);
    return 1;
}

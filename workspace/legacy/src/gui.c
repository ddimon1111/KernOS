#include "gui.h"
#include "fs.h"
#include "vga.h"
#include <stdint.h>
#include <stddef.h>

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 768
#define TASKBAR_Y 740
#define TASKBAR_HEIGHT 28
#define WINDOW_TITLEBAR_HEIGHT 20
#define CURSOR_WIDTH 12
#define CURSOR_HEIGHT 19
#define MIN_WINDOW_WIDTH 150
#define MIN_WINDOW_HEIGHT 150

struct glyph_entry { char c; const uint8_t d[8]; };

static const struct glyph_entry font[] = {
    { ' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    { '-', {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00} },
    { ':', {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00} },
    { '0', {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00} },
    { '1', {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00} },
    { '2', {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00} },
    { '3', {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00} },
    { '4', {0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C,0x00} },
    { '5', {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00} },
    { '6', {0x3C,0x60,0x7C,0x66,0x66,0x66,0x3C,0x00} },
    { '7', {0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00} },
    { '8', {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00} },
    { '9', {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00} },
    { 'A', {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00} },
    { 'B', {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00} },
    { 'C', {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00} },
    { 'D', {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00} },
    { 'E', {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00} },
    { 'F', {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00} },
    { 'G', {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00} },
    { 'H', {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00} },
    { 'I', {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00} },
    { 'J', {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00} },
    { 'K', {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00} },
    { 'L', {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00} },
    { 'M', {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00} },
    { 'N', {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00} },
    { 'O', {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00} },
    { 'P', {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00} },
    { 'Q', {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00} },
    { 'R', {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00} },
    { 'S', {0x3C,0x66,0x30,0x18,0x0C,0x66,0x3C,0x00} },
    { 'T', {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00} },
    { 'U', {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00} },
    { 'V', {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00} },
    { 'W', {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00} },
    { 'X', {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00} },
    { 'Y', {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00} },
    { 'Z', {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00} },
};

static uint32_t *framebuffer = 0;
static uint32_t back_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static uint32_t fb_width = SCREEN_WIDTH;
static uint32_t fb_height = SCREEN_HEIGHT;
static uint32_t fb_pitch = SCREEN_WIDTH;

int mouse_mode = 0;
int mouse_x = 200;
int mouse_y = 200;
int active_window_id = 0;
int left_click = 0;
static int alt_pressed = 0;
static int shift_pressed = 0;

struct Window {
    int x, y, w, h;
    const char *title;
    int is_active;
    int id;
    void (*draw_content)(struct Window *win);
    int cursor_pos_x;
    int cursor_pos_y;
    char buffer[1024];
    int vi_mode;
};

static struct Window windows[3];
static int window_count = 3;
static uint64_t tick_count = 0;

static const uint8_t cursor_bitmap[CURSOR_HEIGHT][CURSOR_WIDTH] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,1,1,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,0,0,0},
    {1,1,1,1,1,1,1,1,1,1,0,0},
    {1,1,1,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,0,1,0,0,0,0,0,0,0,0,0},
    {1,0,0,1,0,0,0,0,0,0,0,0},
    {1,0,0,0,1,0,0,0,0,0,0,0},
    {1,0,0,0,0,1,0,0,0,0,0,0},
    {1,0,0,0,0,0,1,0,0,0,0,0},
    {1,0,0,0,0,0,0,1,0,0,0,0}
};

static void draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)fb_width || y < 0 || y >= (int)fb_height) return;
    back_buffer[y * fb_width + x] = color;
}

static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)fb_width) w = fb_width - x;
    if (y + h > (int)fb_height) h = fb_height - y;
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            back_buffer[(y + j) * fb_width + x + i] = color;
        }
    }
}

static const uint8_t *find_glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    for (size_t i = 0; i < sizeof(font) / sizeof(font[0]); ++i) {
        if (font[i].c == c) return font[i].d;
    }
    return font[0].d;
}

static void draw_char(int x, int y, char c, uint32_t color) {
    const uint8_t *glyph = find_glyph(c);
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (glyph[row] & (0x80 >> col)) {
                draw_pixel(x + col, y + row, color);
            }
        }
    }
}

static void draw_text(int x, int y, const char *text, uint32_t color) {
    int offset = 0;
    while (*text) {
        draw_char(x + offset * 8, y, *text, color);
        ++text;
        ++offset;
    }
}

static char scancode_to_ascii(uint8_t scancode) {
    switch (scancode) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        default: return 0;
    }
}

static void gui_draw_window(struct Window *win) {
    uint32_t title_color = win->is_active ? 0xFF0000FFU : 0xFF888888U;
    fill_rect(win->x, win->y, win->w, win->h, 0xFFAAAAAAU);
    fill_rect(win->x, win->y, win->w, WINDOW_TITLEBAR_HEIGHT, title_color);
    for (int i = 0; i < 2; ++i) {
        int x0 = win->x + i;
        int y0 = win->y + i;
        int x1 = win->x + win->w - 1 - i;
        int y1 = win->y + win->h - 1 - i;
        for (int x = x0; x <= x1; ++x) {
            draw_pixel(x, y0, 0xFFFFFFFFU);
            draw_pixel(x, y1, 0xFFFFFFFFU);
        }
        for (int y = y0; y <= y1; ++y) {
            draw_pixel(x0, y, 0xFFFFFFFFU);
            draw_pixel(x1, y, 0xFFFFFFFFU);
        }
    }
    draw_text(win->x + 4, win->y + 4, win->title, 0x00FFFFFFU);
    if (win->draw_content) {
        win->draw_content(win);
    }
}

static void gui_draw_cursor(void) {
    for (int y = 0; y < CURSOR_HEIGHT; ++y) {
        for (int x = 0; x < CURSOR_WIDTH; ++x) {
            if (cursor_bitmap[y][x]) {
                draw_pixel(mouse_x + x, mouse_y + y, 0xFFFFFFFFU);
            }
        }
    }
}

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void app_terminal_draw(struct Window *win) {
    fill_rect(win->x + 4, win->y + WINDOW_TITLEBAR_HEIGHT + 4,
              win->w - 8, win->h - WINDOW_TITLEBAR_HEIGHT - 8, 0xFF000000U);
    draw_text(win->x + 8, win->y + WINDOW_TITLEBAR_HEIGHT + 8,
              "KernOS Terminal v0.1", 0x00FFFFFFU);
    draw_text(win->x + 8, win->y + WINDOW_TITLEBAR_HEIGHT + 24,
              "> ", 0x00FFFFFFU);
}

static void app_filemanager_draw(struct Window *win) {
    fill_rect(win->x + 4, win->y + WINDOW_TITLEBAR_HEIGHT + 4,
              win->w - 8, win->h - WINDOW_TITLEBAR_HEIGHT - 8, 0xFF101010U);
    draw_text(win->x + 8, win->y + WINDOW_TITLEBAR_HEIGHT + 8,
              "File Manager", 0x00FFFFFFU);
    int y = win->y + WINDOW_TITLEBAR_HEIGHT + 26;
    int x = win->x + 8;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!file_system[i].is_used) continue;
        char label[40];
        int pos = 0;
        const char *name = file_system[i].name;
        while (*name && pos < 36) {
            label[pos++] = *name++;
        }
        label[pos] = '\0';
        draw_text(x, y, label, 0x00FFFFFFU);
        y += 16;
        if (y + 16 > win->y + win->h - 12) break;
    }
}

static void app_vi_draw(struct Window *win) {
    fill_rect(win->x + 4, win->y + WINDOW_TITLEBAR_HEIGHT + 4,
              win->w - 8, win->h - WINDOW_TITLEBAR_HEIGHT - 8, 0xFF000020U);
    draw_text(win->x + 8, win->y + WINDOW_TITLEBAR_HEIGHT + 8,
              "Vi Editor", 0x00FFFFFFU);
    int base_y = win->y + WINDOW_TITLEBAR_HEIGHT + 24;
    int text_x = win->x + 8;
    int text_y = base_y;
    const char *p = win->buffer;
    int col = 0;
    while (*p && text_y + 10 < win->y + win->h - 20) {
        if (*p == '\n' || col >= (win->w - 16) / 8) {
            col = 0;
            text_y += 10;
            if (*p == '\n') {
                ++p;
                continue;
            }
        }
        char ch[2] = {*p, '\0'};
        draw_text(text_x + col * 8, text_y, ch, 0x00FFFFFFU);
        ++col;
        ++p;
    }
    if (win->vi_mode == 0) {
        draw_text(win->x + 8, win->y + win->h - 18, "-- COMMAND --", 0x00FFFF00U);
    } else {
        draw_text(win->x + 8, win->y + win->h - 18, "-- INSERT --", 0x0000FF00U);
    }
    int cursor_screen_x = win->x + 8 + win->cursor_pos_x * 8;
    int cursor_screen_y = base_y + win->cursor_pos_y * 10;
    if (cursor_screen_x < win->x + win->w - 8 && cursor_screen_y < win->y + win->h - 12) {
        fill_rect(cursor_screen_x, cursor_screen_y, 8, 10, 0xFFFFFFFFU);
    }
}

static void gui_init_windows(void) {
    windows[0].x = 50;
    windows[0].y = 60;
    windows[0].w = 420;
    windows[0].h = 300;
    windows[0].title = "TERM";
    windows[0].is_active = 1;
    windows[0].id = 0;
    windows[0].draw_content = app_terminal_draw;
    windows[0].cursor_pos_x = 0;
    windows[0].cursor_pos_y = 0;
    windows[0].buffer[0] = '\0';
    windows[0].vi_mode = 0;

    windows[1].x = 500;
    windows[1].y = 60;
    windows[1].w = 420;
    windows[1].h = 300;
    windows[1].title = "FILES";
    windows[1].is_active = 0;
    windows[1].id = 1;
    windows[1].draw_content = app_filemanager_draw;
    windows[1].cursor_pos_x = 0;
    windows[1].cursor_pos_y = 0;
    windows[1].buffer[0] = '\0';
    windows[1].vi_mode = 0;

    windows[2].x = 50;
    windows[2].y = 380;
    windows[2].w = 520;
    windows[2].h = 330;
    windows[2].title = "VI";
    windows[2].is_active = 0;
    windows[2].id = 2;
    windows[2].draw_content = app_vi_draw;
    windows[2].cursor_pos_x = 0;
    windows[2].cursor_pos_y = 0;
    windows[2].buffer[0] = '\0';
    windows[2].vi_mode = 0;
}

static void draw_taskbar(void) {
    fill_rect(0, TASKBAR_Y, SCREEN_WIDTH, TASKBAR_HEIGHT, 0xFF444444U);
    draw_text(8, TASKBAR_Y + 6, "TERM", 0x00FFFFFFU);
    draw_text(64, TASKBAR_Y + 6, "FILES", 0x00FFFFFFU);
    draw_text(132, TASKBAR_Y + 6, "VI", 0x00FFFFFFU);
    char time_text[32];
    int seconds = tick_count % 60;
    int minutes = (tick_count / 60) % 60;
    int hours = (tick_count / 3600) % 24;
    time_text[0] = '0' + (hours / 10);
    time_text[1] = '0' + (hours % 10);
    time_text[2] = ':';
    time_text[3] = '0' + (minutes / 10);
    time_text[4] = '0' + (minutes % 10);
    time_text[5] = ':';
    time_text[6] = '0' + (seconds / 10);
    time_text[7] = '0' + (seconds % 10);
    time_text[8] = '\0';
    draw_text( SCREEN_WIDTH - 8*9, TASKBAR_Y + 6, time_text, 0x00FFFFFFU);
}

static void window_copy(struct Window *dst, const struct Window *src) {
    dst->x = src->x;
    dst->y = src->y;
    dst->w = src->w;
    dst->h = src->h;
    dst->title = src->title;
    dst->is_active = src->is_active;
    dst->id = src->id;
    dst->draw_content = src->draw_content;
    dst->cursor_pos_x = src->cursor_pos_x;
    dst->cursor_pos_y = src->cursor_pos_y;
    dst->vi_mode = src->vi_mode;
    for (int i = 0; i < (int)sizeof(dst->buffer); ++i) {
        dst->buffer[i] = src->buffer[i];
    }
}

static void bring_window_to_front(int id) {
    if (id < 0 || id >= window_count) return;
    struct Window temp;
    window_copy(&temp, &windows[id]);
    for (int i = id; i < window_count - 1; ++i) {
        window_copy(&windows[i], &windows[i + 1]);
    }
    window_copy(&windows[window_count - 1], &temp);
    active_window_id = window_count - 1;
    for (int i = 0; i < window_count; ++i) {
        windows[i].is_active = (i == active_window_id);
    }
}

static void handle_window_click(int x, int y) {
    for (int i = 0; i < window_count; ++i) {
        int idx = window_count - 1 - i;
        if (point_in_rect(x, y, windows[idx].x, windows[idx].y,
                          windows[idx].w, windows[idx].h)) {
            bring_window_to_front(idx);
            return;
        }
    }
}

static void handle_taskbar_click(int x, int y) {
    if (point_in_rect(x, y, 8, TASKBAR_Y + 6, 32, 12)) {
        bring_window_to_front(0);
    } else if (point_in_rect(x, y, 64, TASKBAR_Y + 6, 40, 12)) {
        bring_window_to_front(1);
    } else if (point_in_rect(x, y, 132, TASKBAR_Y + 6, 24, 12)) {
        bring_window_to_front(2);
    }
}

static void process_mouse_actions(void) {
    if (!mouse_mode) return;
    if (left_click) {
        if (mouse_y >= TASKBAR_Y) {
            handle_taskbar_click(mouse_x, mouse_y);
        } else {
            handle_window_click(mouse_x, mouse_y);
        }
        left_click = 0;
    }
}

static void gui_process_keyboard(int keycode) {
    int is_release = !!(keycode & 0x80);
    uint8_t base = keycode & 0x7F;

    if (base == 0x38) {
        alt_pressed = is_release ? 0 : 1;
        if (is_release) return;
    }
    if (base == 0x2A || base == 0x36) {
        shift_pressed = is_release ? 0 : 1;
        if (is_release) return;
    }
    if (base == 0x0F && !is_release && mouse_mode) {
        active_window_id = (active_window_id + 1) % window_count;
        for (int i = 0; i < window_count; ++i) {
            windows[i].is_active = (i == active_window_id);
        }
        return;
    }
    if (base == 0x1C && !is_release && mouse_mode) {
        left_click = 1;
        return;
    }
    if (base == 0x2F && !is_release && alt_pressed) {
        mouse_mode = !mouse_mode;
        return;
    }

    if (!mouse_mode) return;
    struct Window *win = &windows[active_window_id];
    int dx = 0;
    int dy = 0;
    if (base == 0x4B) dx = -8; /* left */
    if (base == 0x4D) dx = 8;  /* right */
    if (base == 0x48) dy = -8; /* up */
    if (base == 0x50) dy = 8;  /* down */
    if (dx || dy) {
        if (shift_pressed) {
            if (win->w + dx >= MIN_WINDOW_WIDTH) win->w += dx;
            if (win->h + dy >= MIN_WINDOW_HEIGHT) win->h += dy;
        } else if (point_in_rect(mouse_x, mouse_y, win->x, win->y, win->w, WINDOW_TITLEBAR_HEIGHT)) {
            win->x += dx;
            win->y += dy;
        } else {
            mouse_x += dx;
            mouse_y += dy;
        }
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x > (int)fb_width - 1) mouse_x = fb_width - 1;
        if (mouse_y > (int)fb_height - 1) mouse_y = fb_height - 1;
    }
}

static void gui_handle_vi_input(int keycode) {
    struct Window *win = &windows[active_window_id];
    if (win->id != 2) return;
    if (win->vi_mode == 0) {
        if (keycode == 0x17) { /* i */
            win->vi_mode = 1;
            return;
        }
        if (keycode == 0x4B) {
            if (win->cursor_pos_x > 0) {
                win->cursor_pos_x--;
            }
            return;
        }
        if (keycode == 0x4D) {
            win->cursor_pos_x++;
            return;
        }
        if (keycode == 0x48) {
            if (win->cursor_pos_y > 0) {
                win->cursor_pos_y--;
            }
            return;
        }
        if (keycode == 0x50) {
            win->cursor_pos_y++;
            return;
        }
    } else {
        if (keycode == 0x01) { /* Esc */
            win->vi_mode = 0;
            return;
        }
        if (keycode >= 0x02 && keycode <= 0x1D) {
            int letter = keycode - 0x02;
            char c = 'a' + letter;
            int len = 0;
            while (win->buffer[len]) len++;
            if (len < (int)sizeof(win->buffer) - 1) {
                win->buffer[len] = c;
                win->buffer[len + 1] = '\0';
            }
        }
    }
}

static void gui_update_tick(void) {
    tick_count++;
}

void gui_init(struct limine_framebuffer *fb) {
    framebuffer = (uint32_t *)(uintptr_t) fb->address;
    fb_width = (uint32_t) fb->width;
    fb_height = (uint32_t) fb->height;
    fb_pitch = (uint32_t)(fb->pitch / 4);
    if (fb_pitch == 0) {
        fb_pitch = fb_width;
    }
    gui_init_windows();
}

void gui_draw(void) {
    fill_rect(0, 0, fb_width, fb_height, 0xFF202020U);
    draw_taskbar();
    for (int i = 0; i < window_count; ++i) {
        gui_draw_window(&windows[i]);
    }
    process_mouse_actions();
    gui_draw_cursor();
    for (uint32_t y = 0; y < fb_height; ++y) {
        for (uint32_t x = 0; x < fb_width; ++x) {
            framebuffer[y * fb_pitch + x] = back_buffer[y * fb_width + x];
        }
    }
    gui_update_tick();
}

int gui_handle_scancode(uint8_t scancode) {
    uint8_t base = scancode & 0x7F;
    int handled = 0;

    if (base == 0x01 || base == 0x02 || base == 0x03 || base == 0x04 || base == 0x05 || base == 0x06 ||
        base == 0x07 || base == 0x08 || base == 0x09 || base == 0x0A || base == 0x0B || base == 0x0C ||
        base == 0x0D || base == 0x10 || base == 0x11 || base == 0x12 || base == 0x13 || base == 0x14 ||
        base == 0x15 || base == 0x16 || base == 0x17 || base == 0x18 || base == 0x19 || base == 0x1A ||
        base == 0x1B || base == 0x1E || base == 0x1F || base == 0x20 || base == 0x21 || base == 0x22 ||
        base == 0x23 || base == 0x24 || base == 0x25 || base == 0x26 || base == 0x27 || base == 0x28 ||
        base == 0x2C || base == 0x2D || base == 0x2E || base == 0x2F || base == 0x30 || base == 0x31 ||
        base == 0x32 || base == 0x33 || base == 0x34 || base == 0x35 || base == 0x39 || base == 0x48 ||
        base == 0x4B || base == 0x4D || base == 0x50 || base == 0x38 || base == 0x2A || base == 0x36 ||
        base == 0x0F) {
        gui_process_keyboard(scancode);
        gui_handle_vi_input(scancode);
        handled = 1;
    }

    if (!handled && !(scancode & 0x80) && mouse_mode && windows[active_window_id].id == 2) {
        char ch = scancode_to_ascii(base);
        if (ch) {
            struct Window *win = &windows[active_window_id];
            int len = 0;
            while (win->buffer[len]) len++;
            if (len < (int)sizeof(win->buffer) - 1) {
                win->buffer[len] = ch;
                win->buffer[len + 1] = '\0';
            }
            handled = 1;
        }
    }

    return handled;
}

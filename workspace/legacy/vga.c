#include "vga.h"
#include "limine.h"
#include <stddef.h>

#include <stdint.h>

/* Character cell size in pixels */
#define CHAR_W 8
#define CHAR_H 8

/* Backing framebuffer info (filled by vga_init) */
static uint8_t *fb_ptr = 0;
static uint32_t fb_pitch = 0;    /* bytes per scanline */
static uint32_t fb_width = 0;    /* pixels */
static uint32_t fb_height = 0;   /* pixels */
static uint32_t fb_bpp = 32;     /* bits per pixel (assume 32 by default) */

/* Cursor in character cells */
static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

/* Background color last used by vga_clear (used when scrolling) */
static uint32_t bg_color = 0x00000000;

/* Forward declarations */
static const uint8_t *get_glyph(char c);
static void set_pixel(uint32_t x, uint32_t y, uint32_t color);
static void scroll_up(void);

/* Simple 8x8 bitmap font for ASCII characters used here. Each glyph is 8
 * bytes, each byte corresponds to one row; MSB is leftmost pixel. We provide
 * digits, uppercase letters and a few punctuation. Lowercase letters map to
 * their uppercase counterparts for simplicity.
 */
struct glyph_entry { char c; const uint8_t d[8]; };

static const struct glyph_entry font[] = {
    /* space */ { ' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00} },
    /* hyphen '-' */ { '-', {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00} },
    /* colon ':' */ { ':', {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00} },

    /* digits 0-9 */
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

    /* Uppercase A-Z */
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

static const size_t font_count = sizeof(font) / sizeof(font[0]);

/* Initialize the framebuffer pointer and geometry from Limine's framebuffer
 * structure. We cache useful values for faster pixel writes.
 */
void vga_init(struct limine_framebuffer *fb) {
    if (!fb)
        return;

    fb_ptr = (uint8_t *)(uintptr_t)(fb->address);
    fb_pitch = (uint32_t)(fb->pitch);
    fb_width = (uint32_t)(fb->width);
    fb_height = (uint32_t)(fb->height);
    fb_bpp = fb->bpp ? fb->bpp : 32;

    cursor_x = 0;
    cursor_y = 0;
}

/* Fill the entire screen with a 32-bit color. The driver writes raw bytes
 * according to the framebuffer's bytes-per-pixel derived from bpp.
 */
void vga_clear(uint32_t color) {
    bg_color = color;
    if (!fb_ptr)
        return;

    uint32_t bytes_per_pixel = fb_bpp / 8;
    uint32_t y, x;

    for (y = 0; y < fb_height; ++y) {
        uint8_t *line = fb_ptr + (uint64_t)y * fb_pitch;
        for (x = 0; x < fb_width; ++x) {
            uint8_t *pixel = line + x * bytes_per_pixel;
            /* Store color as little-endian bytes */
            if (bytes_per_pixel >= 4) {
                pixel[0] = (uint8_t)(color & 0xFF);
                pixel[1] = (uint8_t)((color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((color >> 16) & 0xFF);
                pixel[3] = (uint8_t)((color >> 24) & 0xFF);
            } else if (bytes_per_pixel == 3) {
                pixel[0] = (uint8_t)(color & 0xFF);
                pixel[1] = (uint8_t)((color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((color >> 16) & 0xFF);
            } else if (bytes_per_pixel == 2) {
                uint16_t c16 = (uint16_t)(color & 0xFFFF);
                pixel[0] = (uint8_t)(c16 & 0xFF);
                pixel[1] = (uint8_t)((c16 >> 8) & 0xFF);
            } else {
                /* 1 byte per pixel: write lower 8 bits */
                pixel[0] = (uint8_t)(color & 0xFF);
            }
        }
    }
}

/* Set a single pixel at (x,y). Method explains the mapping from (x,y) to
 * a linear framebuffer address: address = fb_ptr + y * fb_pitch + x * (bpp/8)
 * where fb_pitch is bytes per scanline (may be >= fb_width * bytes_per_pixel).
 */
static void set_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!fb_ptr)
        return;
    if (x >= fb_width || y >= fb_height)
        return;

    uint32_t bpp_bytes = fb_bpp / 8;
    uint8_t *pixel = fb_ptr + (uint64_t)y * fb_pitch + (uint64_t)x * bpp_bytes;

    if (bpp_bytes >= 4) {
        pixel[0] = (uint8_t)(color & 0xFF);
        pixel[1] = (uint8_t)((color >> 8) & 0xFF);
        pixel[2] = (uint8_t)((color >> 16) & 0xFF);
        pixel[3] = (uint8_t)((color >> 24) & 0xFF);
    } else if (bpp_bytes == 3) {
        pixel[0] = (uint8_t)(color & 0xFF);
        pixel[1] = (uint8_t)((color >> 8) & 0xFF);
        pixel[2] = (uint8_t)((color >> 16) & 0xFF);
    } else if (bpp_bytes == 2) {
        uint16_t c16 = (uint16_t)(color & 0xFFFF);
        pixel[0] = (uint8_t)(c16 & 0xFF);
        pixel[1] = (uint8_t)((c16 >> 8) & 0xFF);
    } else {
        pixel[0] = (uint8_t)(color & 0xFF);
    }
}

/* Scroll the framebuffer up by CHAR_H pixels. We copy scanlines manually
 * because standard library functions are not available in freestanding
 * environment. After shifting, the bottom CHAR_H rows are cleared with
 * background color.
 */
static void scroll_up(void) {
    if (!fb_ptr)
        return;

    uint32_t row_bytes = fb_pitch;
    uint32_t y;

    /* Move each scanline up by CHAR_H rows */
    for (y = 0; y + CHAR_H < fb_height; ++y) {
        uint8_t *dst = fb_ptr + (uint64_t)y * fb_pitch;
        uint8_t *src = fb_ptr + (uint64_t)(y + CHAR_H) * fb_pitch;
        uint32_t i;
        for (i = 0; i < row_bytes; ++i)
            dst[i] = src[i];
    }

    /* Clear the bottom CHAR_H rows */
    for (y = fb_height - CHAR_H; y < fb_height; ++y) {
        uint8_t *line = fb_ptr + (uint64_t)y * fb_pitch;
        uint32_t x;
        uint32_t bpp_bytes = fb_bpp / 8;
        for (x = 0; x < fb_width; ++x) {
            uint8_t *pixel = line + x * bpp_bytes;
            if (bpp_bytes >= 4) {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
                pixel[1] = (uint8_t)((bg_color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((bg_color >> 16) & 0xFF);
                pixel[3] = (uint8_t)((bg_color >> 24) & 0xFF);
            } else if (bpp_bytes == 3) {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
                pixel[1] = (uint8_t)((bg_color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((bg_color >> 16) & 0xFF);
            } else if (bpp_bytes == 2) {
                uint16_t c16 = (uint16_t)(bg_color & 0xFFFF);
                pixel[0] = (uint8_t)(c16 & 0xFF);
                pixel[1] = (uint8_t)((c16 >> 8) & 0xFF);
            } else {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
            }
        }
    }
}

/* Clear a character cell at (char_x,char_y) by drawing background pixels */
static void clear_char_cell(uint32_t char_x, uint32_t char_y) {
    uint32_t base_x = char_x * CHAR_W;
    uint32_t base_y = char_y * CHAR_H;
    uint32_t bpp_bytes = fb_bpp / 8;

    for (uint32_t row = 0; row < CHAR_H; ++row) {
        uint8_t *line = fb_ptr + (uint64_t)(base_y + row) * fb_pitch;
        for (uint32_t col = 0; col < CHAR_W; ++col) {
            uint8_t *pixel = line + (base_x + col) * bpp_bytes;
            if (bpp_bytes >= 4) {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
                pixel[1] = (uint8_t)((bg_color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((bg_color >> 16) & 0xFF);
                pixel[3] = (uint8_t)((bg_color >> 24) & 0xFF);
            } else if (bpp_bytes == 3) {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
                pixel[1] = (uint8_t)((bg_color >> 8) & 0xFF);
                pixel[2] = (uint8_t)((bg_color >> 16) & 0xFF);
            } else if (bpp_bytes == 2) {
                uint16_t c16 = (uint16_t)(bg_color & 0xFFFF);
                pixel[0] = (uint8_t)(c16 & 0xFF);
                pixel[1] = (uint8_t)((c16 >> 8) & 0xFF);
            } else {
                pixel[0] = (uint8_t)(bg_color & 0xFF);
            }
        }
    }
}

void vga_backspace(void) {
    if (!fb_ptr)
        return;

    if (cursor_x > 0) {
        cursor_x -= 1;
    } else if (cursor_y > 0) {
        cursor_y -= 1;
        cursor_x = (fb_width / CHAR_W);
        if (cursor_x > 0) cursor_x -= 1;
    }

    clear_char_cell(cursor_x, cursor_y);
}

/* Get pointer to glyph data for character c. Lowercase are mapped to
 * uppercase to reduce font table size. If glyph not found, returns the
 * glyph for '?' (represented here as blank with a middle line).
 */
static const uint8_t unknown_glyph[8] = {0x00,0x3C,0x42,0x5A,0x66,0x42,0x3C,0x00};

static const uint8_t *get_glyph(char c) {
    if (c >= 'a' && c <= 'z')
        c = (char)(c - ('a' - 'A'));

    for (size_t i = 0; i < font_count; ++i) {
        if (font[i].c == c)
            return font[i].d;
    }
    return unknown_glyph;
}

/* Render a single character at the current cursor position. If character
 * overflows vertically, the framebuffer is scrolled.
 */
void vga_putchar(char c, uint32_t color) {
    if (!fb_ptr)
        return;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y += 1;
        if ((cursor_y + 1) * CHAR_H > fb_height)
            scroll_up();
        return;
    }

    const uint8_t *glyph = get_glyph(c);

    uint32_t base_x = cursor_x * CHAR_W;
    uint32_t base_y = cursor_y * CHAR_H;

    for (uint32_t row = 0; row < CHAR_H; ++row) {
        uint8_t row_bits = glyph[row];
        for (uint32_t bit = 0; bit < CHAR_W; ++bit) {
            /* MSB is leftmost pixel in our glyph convention */
            uint8_t mask = (uint8_t)(0x80 >> bit);
            if (row_bits & mask) {
                set_pixel(base_x + bit, base_y + row, color);
            } else {
                /* Optionally, could set background; we leave existing pixels
                 * untouched to preserve whatever background was present. */
            }
        }
    }

    cursor_x += 1;
    /* Wrap to next line if needed */
    if ((cursor_x + 1) * CHAR_W > fb_width) {
        cursor_x = 0;
        cursor_y += 1;
    }

    if ((cursor_y + 1) * CHAR_H > fb_height)
        scroll_up();
}

/* Print a NUL-terminated string */
void vga_print(const char *str, uint32_t color) {
    if (!str)
        return;
    while (*str) {
        vga_putchar(*str++, color);
    }
}

/* Print unsigned decimal number. Manual conversion: extract digits into
 * a small buffer, then print in correct order. Works for 0 too.
 */
void vga_print_num(uint64_t num, uint32_t color) {
    char buf[32]; /* enough for 64-bit decimal */
    int pos = 0;

    if (num == 0) {
        vga_putchar('0', color);
        return;
    }

    while (num > 0 && pos < (int)sizeof(buf) - 1) {
        uint64_t rem = num % 10ULL;
        buf[pos++] = (char)('0' + (int)rem);
        num /= 10ULL;
    }

    /* digits are in reverse order */
    for (int i = pos - 1; i >= 0; --i)
        vga_putchar(buf[i], color);
}

/* Mark vga tasks done in TODO list by updating state externally (caller).
 * This file intentionally avoids any dynamic allocation and depends only on
 * simple integer types so it is safe for freestanding environments.
 */

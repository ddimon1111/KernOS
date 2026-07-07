#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* Initialize the VGA text driver in BIOS mode. Must be called before any
 * other VGA output function.
 */
void vga_init(void);

/* Fill the whole framebuffer with a 32-bit color value. Color format is
 * assumed little-endian 0xAARRGGBB or 0x00RRGGBB depending on usage; the
 * exact channel ordering depends on the framebuffer, but this driver writes
 * raw 32-bit words for bpp==32.
 */
void vga_clear(uint32_t color);

/* Serial debug output for headless QEMU. Initializes COM1 and prints to
 * the serial device so kernel startup can be observed in -nographic mode.
 */
void serial_init(void);
void serial_putchar(char c);
void serial_print(const char *str);

/* Put a single character at the current cursor position (characters are
 * rendered 8x8 pixels). Handles '\n' by moving to the next line.
 */
void vga_putchar(char c, uint32_t color);

/* Print a NUL-terminated string. */
void vga_print(const char *str, uint32_t color);

/* Print an unsigned decimal number. */
void vga_print_num(uint64_t num, uint32_t color);

/* Remove previous character (backspace): move cursor back one character and
 * clear that character cell. Useful for simple line editing in the shell.
 */
void vga_backspace(void);

#endif /* VGA_H */

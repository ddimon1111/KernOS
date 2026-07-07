/* Minimal kernel for KernOS.
 * This kernel is loaded by Stage2 at 0x100000 and runs in Long Mode.
 * It prints status text directly to the VGA text buffer and enters CLI mode.
 */

static volatile unsigned short *const VGA = (unsigned short *)0xB8000;
static unsigned int cursor_row = 0;
static unsigned int cursor_col = 0;

static void update_cursor(void)
{
    unsigned short pos = cursor_row * 80 + cursor_col;
    asm volatile ("mov $0x0E, %%ah\n"
                  "mov $0x0F, %%al\n"
                  "out %%al, $0x3D4\n"
                  "mov %%ah, %%al\n"
                  "out %%al, $0x3D5\n"
                  :
                  :
                  : "rax");
}

static void clear_screen(void)
{
    unsigned short blank = 0x0720;
    for (unsigned int i = 0; i < 80 * 25; ++i) {
        VGA[i] = blank;
    }
    cursor_row = 0;
    cursor_col = 0;
    update_cursor();
}

void kputc(char c)
{
    if (c == '\r') {
        cursor_col = 0;
        return;
    }
    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        if (cursor_row >= 25) cursor_row = 24;
        return;
    }
    VGA[cursor_row * 80 + cursor_col] = (unsigned short)c | 0x0700;
    cursor_col++;
    if (cursor_col >= 80) {
        cursor_col = 0;
        cursor_row++;
        if (cursor_row >= 25) cursor_row = 24;
    }
}

void kprint(const char *str)
{
    while (*str) {
        kputc(*str++);
    }
}

int init_graphics(void)
{
    /* Graphics initialization is not implemented yet.
     * If video text output is available, we stay in CLI mode.
     */
    return 0;
}

void run_cli(void)
{
    kprint("No GUI available. Entering CLI mode...\r\n");
    kprint("> ");
    for (;;) {
        asm volatile ("hlt");
    }
}

void kmain(void)
{
    clear_screen();
    kprint("Kernel loaded at 0x100000\r\n");
    if (!init_graphics()) {
        run_cli();
    }
    for (;;) {
        asm volatile ("hlt");
    }
}

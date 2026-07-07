#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "limine.h"

struct Window;

/* Initialize GUI with Limine framebuffer information. */
void gui_init(struct limine_framebuffer *fb);

/* Render one GUI frame. */
void gui_draw(void);

/* Handle a raw keyboard scancode. Returns 1 if the event was consumed. */
int gui_handle_scancode(uint8_t scancode);

/* Global GUI state for mouse, focus and click handling. */
extern int mouse_mode;
extern int mouse_x;
extern int mouse_y;
extern int active_window_id;
extern int left_click;

#endif /* GUI_H */

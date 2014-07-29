#ifndef DISPLAY_H
#define DISPLAY_H

int display_init(int width, int height);
void display_exit(void);
void update_texture(void *data, int width, int height);
void render_frame(void);

#endif

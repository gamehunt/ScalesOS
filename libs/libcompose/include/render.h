#ifndef __LIB_COMPOSE_RENDER_H
#define __LIB_COMPOSE_RENDER_H

#include "compose.h"

#define COMPOSE_RENDER_PIXEL    0
#define COMPOSE_RENDER_LINE     1
#define COMPOSE_RENDER_RECT     2
#define COMPOSE_RENDER_FRECT    3
#define COMPOSE_RENDER_CIRCLE   4
#define COMPOSE_RENDER_ELLIPSE  5
#define COMPOSE_RENDER_FELLIPSE 6
#define COMPOSE_RENDER_FILL     7
#define COMPOSE_RENDER_FLUSH    8
#define COMPOSE_RENDER_STRING   9

void compose_cl_draw(compose_client_t* cli, id_t ctx, int op, uint32_t* data, int param_amount);
void compose_cl_draw_pixel(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, color_t color);
void compose_cl_line(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color);
void compose_cl_rect(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t w, size_t h, color_t border);
void compose_cl_filled_rect(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t w, size_t h, color_t border, color_t fill);
void compose_cl_circle(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t radius, color_t border);
void compose_cl_filled_circle(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t radius, color_t border, color_t fill);
void compose_cl_ellipse(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t a, size_t b, color_t border);
void compose_cl_filled_ellipse(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t a, size_t b, color_t border, color_t fill);
void compose_cl_fill(compose_client_t* cli, id_t ctx, color_t color);
void compose_cl_flush(compose_client_t* cli, id_t ctx);
void compose_cl_string(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, color_t bg, color_t fg, const char* string);

void compose_sv_draw(compose_window_t* ctx, int op, uint32_t* data);

#endif

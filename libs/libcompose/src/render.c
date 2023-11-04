#include "render.h"
#include "compose.h"
#include "request.h"
#include <stdlib.h>
#include <string.h>

void compose_cl_draw(compose_client_t* cli, id_t ctx, int op, uint32_t* data, int params_amount) {
	size_t sz = sizeof(compose_draw_req_t) + params_amount * sizeof(uint32_t);
	compose_draw_req_t* d = malloc(sz);
	d->req.type = COMPOSE_REQ_DRAW;
	d->req.size = sz;
	d->win = ctx;
	d->op  = op;
	if(params_amount) {
		memcpy(d->params, data, params_amount * sizeof(uint32_t));
	}
	compose_cl_send_request(cli, d);
	free(d);
}

void compose_cl_draw_pixel(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, color_t color){
	uint32_t data[3];
	data[0] = x0;
	data[1] = y0;
	data[2] = color;

	compose_cl_draw(cli, ctx, COMPOSE_RENDER_PIXEL, data, 3);
}

void compose_cl_line(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, coord_t x1, coord_t y1, color_t color){
	uint32_t data[5];
	data[0] = x0;
	data[1] = y0;
	data[2] = x1;
	data[3] = y1;
	data[4] = color;

	compose_cl_draw(cli, ctx, COMPOSE_RENDER_PIXEL, data, 5);
}

void compose_cl_rect(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t w, size_t h, color_t border){
	uint32_t data[5];
	data[0] = x0;
	data[1] = y0;
	data[2] = w;
	data[3] = h;
	data[4] = border;

	compose_cl_draw(cli, ctx, COMPOSE_RENDER_RECT, data, 5);
}

void compose_cl_filled_rect(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t w, size_t h, color_t border, color_t fill){
	uint32_t data[6];
	data[0] = x0;
	data[1] = y0;
	data[2] = w;
	data[3] = h;
	data[4] = border;
	data[5] = border;

	compose_cl_draw(cli, ctx, COMPOSE_RENDER_FRECT, data, 6);
}

void compose_cl_circle(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t radius, color_t border){

}

void compose_cl_filled_circle(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t radius, color_t border, color_t fill){

}

void compose_cl_ellipse(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t a, size_t b, color_t border){

}

void compose_cl_filled_ellipse(compose_client_t* cli, id_t ctx, coord_t x0, coord_t y0, size_t a, size_t b, color_t border, color_t fill){

}

void compose_cl_fill(compose_client_t* cli, id_t ctx, color_t color){
	uint32_t data[1];
	data[0] = color;

	compose_cl_draw(cli, ctx, COMPOSE_RENDER_FILL, data, 1);
}

void compose_cl_flush(compose_client_t* cli, id_t ctx){
	compose_cl_draw(cli, ctx, COMPOSE_RENDER_FLUSH, NULL, 0);
}

#define VERIFY_POS(ctx, x, y) \
	if( x >= ctx->sizes.w || \
		y >= ctx->sizes.h  \
		) { \
		return; \
	} 

void compose_sv_draw(compose_window_t* ctx, int op, uint32_t* data) {
	if(op == COMPOSE_RENDER_PIXEL) {
		uint32_t x = data[0] + ctx->sizes.b;
		uint32_t y = data[1] + ctx->sizes.b;
		VERIFY_POS(ctx, x, y)


		fb_pixel(&ctx->ctx, x, y, data[2]);
	} else if(op == COMPOSE_RENDER_LINE) {
		uint32_t x0 = data[0] + ctx->sizes.b;
		uint32_t y0 = data[1] + ctx->sizes.b;
		uint32_t x1 = data[2] + ctx->sizes.b;
		uint32_t y1 = data[3] + ctx->sizes.b;

		VERIFY_POS(ctx, x0, y0)
		VERIFY_POS(ctx, x1, y1)


		fb_line(&ctx->ctx, x0, y0, x1, y1, data[4]);
	} else if(op == COMPOSE_RENDER_RECT) {
		uint32_t x0 = data[0] + ctx->sizes.b;
		uint32_t y0 = data[1] + ctx->sizes.b;
	
		uint32_t w = data[2];
		uint32_t h = data[3];

		uint32_t x1 = x0 + w;
		uint32_t y1 = y0 + h;

		VERIFY_POS(ctx, x0, y0)
		VERIFY_POS(ctx, x1, y1)

		fb_rect(&ctx->ctx, x0, y0, w, h, data[4]);
	} else if (op == COMPOSE_RENDER_FRECT) {
		uint32_t x0 = data[0] + ctx->sizes.b;
		uint32_t y0 = data[1] + ctx->sizes.b;
	
		uint32_t w = data[2];
		uint32_t h = data[3];

		uint32_t x1 = x0 + w;
		uint32_t y1 = y0 + h;

		VERIFY_POS(ctx, x0, y0)
		VERIFY_POS(ctx, x1, y1)

		fb_filled_rect(&ctx->ctx, x0, y0, w, h, data[4], data[5]);
	} else if (op == COMPOSE_RENDER_FILL) {
		fb_fill(&ctx->ctx, data[0]);
	} else if (op == COMPOSE_RENDER_FLUSH) {
		fb_flush(&ctx->ctx);
	}
}
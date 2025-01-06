#pragma once

#include "common.h"
#include <wlr/interfaces/wlr_buffer.h>
#include <cairo.h>

struct sonde_cairo_buffer {
  struct wlr_buffer buffer;
  cairo_surface_t *cairo_surface;
  cairo_t *cairo;
};

struct sonde_cairo_buffer *sonde_cairo_buffer_create(int width, int height);
void sonde_cairo_buffer_destroy(struct sonde_cairo_buffer *buffer);

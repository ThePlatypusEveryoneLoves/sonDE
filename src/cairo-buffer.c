#include "cairo-buffer.h"
#include <drm_fourcc.h>

static struct sonde_cairo_buffer *sonde_cairo_buffer_from_wlr_buffer(struct wlr_buffer *buffer) {
  struct sonde_cairo_buffer *sonde_buffer = wl_container_of(buffer, sonde_buffer, buffer);
  return sonde_buffer;
}

static void impl_destroy(struct wlr_buffer *buffer) {
  struct sonde_cairo_buffer *sonde_buffer = sonde_cairo_buffer_from_wlr_buffer(buffer);
  cairo_surface_destroy(sonde_buffer->cairo_surface);
  cairo_destroy(sonde_buffer->cairo);
  
  free(sonde_buffer);
}

static bool impl_begin(
  struct wlr_buffer *buffer,
  uint32_t flags, void **data,
  uint32_t *format, size_t *stride) {
  struct sonde_cairo_buffer *sonde_buffer = sonde_cairo_buffer_from_wlr_buffer(buffer);
  // set stride/format
  *stride = cairo_image_surface_get_stride(sonde_buffer->cairo_surface);
  *format = DRM_FORMAT_ARGB8888;
  *data = cairo_image_surface_get_data(sonde_buffer->cairo_surface);
  return true;
}

void sonde_cairo_buffer_destroy(struct sonde_cairo_buffer *buffer) {
  // dropping the buffer hanldes dropping the sonde buffer
  wlr_buffer_drop(&buffer->buffer);
}

void impl_noop(struct wlr_buffer *buffer) {}

static const struct wlr_buffer_impl sonde_cairo_buffer_impl = {
  .destroy = impl_destroy,
  .begin_data_ptr_access = impl_begin,
  .end_data_ptr_access = impl_noop
};

struct sonde_cairo_buffer *sonde_cairo_buffer_create(int width, int height) {
  struct sonde_cairo_buffer *sonde_buffer = calloc(1, sizeof(*sonde_buffer));

  sonde_buffer->cairo_surface = cairo_image_surface_create(
    CAIRO_FORMAT_ARGB32,
    width, height);

  wlr_buffer_init(&sonde_buffer->buffer, &sonde_cairo_buffer_impl, width, height);

  sonde_buffer->cairo = cairo_create(sonde_buffer->cairo_surface);

  return sonde_buffer;
}

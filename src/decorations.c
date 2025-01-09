#include "decorations.h"
#include "cairo-buffer.h"
#include <cairo.h>
#include "pango/pango-font.h"
#include "pango/pango-layout.h"
#include "pango/pangocairo.h"
#include "view.h"
#include "xdg-shell.h"

static inline sonde_view_t sonde_view_from_sonde_xdg_decoration(
    struct sonde_xdg_decoration *decoration) {
  sonde_view_t view = wl_container_of(decoration, view, decoration);
  return view;
}

void sonde_apply_decorations(struct sonde_xdg_decoration *decoration) {
  // if the decoration mode is still NONE, set to default
  // TODO: config
  if (decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE) {
    decoration->client_mode = WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
  }
  
  sonde_view_t view = sonde_view_from_sonde_xdg_decoration(decoration);
  sonde_server_t server = view->server;

  if (decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
    // already exists
    if (decoration->scene_tree) return;

    decoration->scene_tree = wlr_scene_tree_create(view->scene_tree);
    wlr_scene_node_lower_to_bottom(&decoration->scene_tree->node);

    float bg_color[4] = {1, 1, 0.2, 1.0}; // yellow

    decoration->rect = wlr_scene_rect_create(decoration->scene_tree, 0, 0, bg_color);
    wlr_scene_node_set_position(&decoration->rect->node, 0, SONDE_DECORATION_TITLEBAR_HEIGHT);
    wlr_scene_node_set_position(&view->surface_scene_tree->node, SONDE_DECORATION_BORDER_WIDTH, SONDE_DECORATION_TITLEBAR_HEIGHT + SONDE_DECORATION_BORDER_WIDTH);

    // titlebar
    decoration->titlebar_node = wlr_scene_buffer_create(
        decoration->scene_tree, NULL);

    sonde_decoration_update_size(decoration);
    //sonde_decoration_update_title(decoration);
  }
}

void sonde_decoration_update_title(struct sonde_xdg_decoration *decoration) {
  const char *title = sonde_xdg_view_from_sonde_view(sonde_view_from_sonde_xdg_decoration(decoration))->toplevel->title;

  // not ready yet
  if (decoration->titlebar == NULL) return;

  //cairo_save(decoration->titlebar->cairo);
  //cairo_set_operator(decoration->titlebar->cairo, CAIRO_OPERATOR_CLEAR);
  cairo_set_source_rgb(decoration->titlebar->cairo, 1, 0, 0);
  cairo_paint(decoration->titlebar->cairo);
  //cairo_restore(decoration->titlebar->cairo);
  
  cairo_set_source_rgb(decoration->titlebar->cairo, 0, 0, 0);
  pango_layout_set_text(decoration->titlebar_pango, title, -1);
  pango_cairo_show_layout(decoration->titlebar->cairo, decoration->titlebar_pango);
  cairo_surface_flush(decoration->titlebar->cairo_surface);
  wlr_scene_buffer_set_buffer(decoration->titlebar_node, &decoration->titlebar->buffer);
}

void sonde_decoration_update_size(struct sonde_xdg_decoration *decoration) {
  wlr_log(WLR_INFO, "update decoration size");
  // destroy and recreate the titlebar
  if (decoration->titlebar != NULL)
    sonde_cairo_buffer_destroy(decoration->titlebar);
  if (decoration->titlebar_pango != NULL)
    g_object_unref(decoration->titlebar_pango);

  sonde_view_t view = sonde_view_from_sonde_xdg_decoration(decoration);
  sonde_xdg_view_t xdg_view = sonde_xdg_view_from_sonde_view(view);

  struct wlr_box toplevel_box;
  wlr_xdg_surface_get_geometry(xdg_view->toplevel->base, &toplevel_box);

  int overall_width = toplevel_box.width + SONDE_DECORATION_BORDER_WIDTH * 2;

  // recreate
  decoration->titlebar = sonde_cairo_buffer_create(overall_width, SONDE_DECORATION_TITLEBAR_HEIGHT);
  decoration->titlebar_pango = pango_cairo_create_layout(decoration->titlebar->cairo);
  
  // set font
  // TODO: config
  PangoFontDescription *desc = pango_font_description_from_string("Fira Code 10");
  pango_layout_set_font_description(decoration->titlebar_pango, desc);
  pango_font_description_free(desc);

  // repaint
  sonde_decoration_update_title(decoration);

  // update the box size
  wlr_scene_rect_set_size(decoration->rect, overall_width, toplevel_box.height + SONDE_DECORATION_BORDER_WIDTH * 2);
}

void sonde_decoration_set_focus(struct sonde_xdg_decoration *decoration,
                                bool focused) {
  sonde_view_t view = sonde_view_from_sonde_xdg_decoration(decoration);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);

  // TODO: config
  float unfocused_color[4] = {1, 1, 0.2, 1.0}; // yellow
  float focused_color[4] = {1, 0.2, 0.2, 1.0}; // red

  wlr_scene_rect_set_color(decoration->rect, focused ? focused_color : unfocused_color);
}


void sonde_decoration_destroy(
    struct sonde_xdg_decoration *sonde_xdg_decoration) {
  if (sonde_xdg_decoration->titlebar != NULL) {
    sonde_xdg_decoration->titlebar = NULL;
    sonde_cairo_buffer_destroy(sonde_xdg_decoration->titlebar);
    g_object_unref(sonde_xdg_decoration->titlebar_pango);

    // these could be null if they were never set (non xdg decoration), or unset already
    if (sonde_xdg_decoration->destroy.notify != NULL)
      wl_list_remove(&sonde_xdg_decoration->destroy.link);
    if (sonde_xdg_decoration->request_mode.notify != NULL)
      wl_list_remove(&sonde_xdg_decoration->request_mode.link);
    if (sonde_xdg_decoration->surface_commit.notify != NULL)
      wl_list_remove(&sonde_xdg_decoration->surface_commit.link);
    // destroy the entire scene tree
    wlr_scene_node_destroy(&sonde_xdg_decoration->scene_tree->node);
  }
}

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
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);
  sonde_server_t server = view->server;

  if (decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
    // already exists
    if (decoration->scene_tree) return;

    decoration->scene_tree = wlr_scene_tree_create(view->scene_tree);
    wlr_scene_node_lower_to_bottom(&decoration->scene_tree->node);

    float bg_color[4] = {1, 1, 0.2, 1.0}; // yellow

    sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);
    

    struct wlr_box toplevel_box;
    
    wlr_xdg_surface_get_geometry(sonde_xdg_view->toplevel->base, &toplevel_box);
    wlr_log(WLR_DEBUG, "LIFECYCLE toplevel extents %dx%d @ (%d, %d)", toplevel_box.width, toplevel_box.height, toplevel_box.x, toplevel_box.y);

    decoration->rect = wlr_scene_rect_create(decoration->scene_tree, toplevel_box.width, toplevel_box.height, bg_color);
    
    wlr_scene_node_set_position(&decoration->scene_tree->node, 0, 0);
    wlr_scene_node_set_position(&view->surface_scene_tree->node, 5, 20);

    wlr_xdg_toplevel_set_size(sonde_xdg_view->toplevel, toplevel_box.width - 10, toplevel_box.height - 25);

    // 5 margin on each side
    int titlebar_width = toplevel_box.width - 10;
    // total space on top is 20, 2.5 margin on top/bottom
    int titlebar_height = 15;

    // titlebar
    decoration->titlebar = sonde_cairo_buffer_create(titlebar_width, titlebar_height);
    decoration->titlebar_node = wlr_scene_buffer_create(
        decoration->scene_tree,
        &decoration->titlebar->buffer);
    wlr_scene_node_set_position(
      &decoration->titlebar_node->node,
      5, 2);
    decoration->titlebar_pango = pango_cairo_create_layout(decoration->titlebar->cairo);

    // set font
    // TODO: config
    PangoFontDescription *desc = pango_font_description_from_string("Fira Code 10");
    pango_layout_set_font_description(decoration->titlebar_pango, desc);
    pango_font_description_free(desc);

    sonde_decoration_update_title(decoration);
  }
}

void sonde_decoration_update_title(struct sonde_xdg_decoration *decoration) {
  const char *title = sonde_xdg_view_from_sonde_view(sonde_view_from_sonde_xdg_decoration(decoration))->toplevel->title;

  // not ready yet
  if (decoration->titlebar == NULL) return;

  cairo_save(decoration->titlebar->cairo);
  cairo_set_operator(decoration->titlebar->cairo, CAIRO_OPERATOR_CLEAR);
  cairo_paint(decoration->titlebar->cairo);
  cairo_restore(decoration->titlebar->cairo);
  
  cairo_set_source_rgb(decoration->titlebar->cairo, 0, 0, 0);
  pango_layout_set_text(decoration->titlebar_pango, title, -1);
  pango_cairo_show_layout(decoration->titlebar->cairo, decoration->titlebar_pango);
  cairo_surface_flush(decoration->titlebar->cairo_surface);
  wlr_scene_buffer_set_buffer(decoration->titlebar_node, &decoration->titlebar->buffer);
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
  // we use titlebar as a "marker" for whether this has been destroyed
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

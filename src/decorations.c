#include "decorations.h"
#include "view.h"
#include "xdg-shell.h"

static inline sonde_view_t sonde_xdg_view_from_sonde_xdg_decoration(
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
  
  sonde_view_t view = sonde_xdg_view_from_sonde_xdg_decoration(decoration);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);

  if (decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
    if (!decoration->scene_tree)
      decoration->scene_tree = wlr_scene_tree_create(view->scene_tree);
    
    wlr_scene_node_lower_to_bottom(&decoration->scene_tree->node);

    float bg_color[4] = {1, 1, 0.2, 1.0}; // yellow

    sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);
    

    struct wlr_box toplevel_box;
    
    wlr_xdg_surface_get_geometry(sonde_xdg_view->toplevel->base, &toplevel_box);
    wlr_log(WLR_DEBUG, "LIFECYCLE toplevel extents %dx%d @ (%d, %d)", toplevel_box.width, toplevel_box.height, toplevel_box.x, toplevel_box.y);

    if (!decoration->rect)
      decoration->rect = wlr_scene_rect_create(decoration->scene_tree, toplevel_box.width, toplevel_box.height, bg_color);
    
    wlr_scene_node_set_position(&decoration->scene_tree->node, 0, 0);
    wlr_scene_node_set_position(&view->surface_scene_tree->node, 5, 5);

    wlr_xdg_toplevel_set_size(sonde_xdg_view->toplevel, toplevel_box.width - 10, toplevel_box.height - 10);
  }
}

void sonde_decoration_set_focus(struct sonde_xdg_decoration *decoration,
                                bool focused) {
  sonde_view_t view = sonde_xdg_view_from_sonde_xdg_decoration(decoration);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);

  float unfocused_color[4] = {1, 0.2, 0.2, 1.0}; // yellow
  float focused_color[4] = {1, 1, 0.2, 1.0}; // red

  wlr_scene_rect_set_color(decoration->rect, focused ? focused_color : unfocused_color);
}


void sonde_decoration_destroy(
    struct sonde_xdg_decoration *sonde_xdg_decoration) {
  if (sonde_xdg_decoration->destroy.notify != NULL) {
    wl_list_remove(&sonde_xdg_decoration->destroy.link);
    sonde_xdg_decoration->destroy.notify = NULL;
  }
  if (sonde_xdg_decoration->request_mode.notify != NULL) {
    wl_list_remove(&sonde_xdg_decoration->request_mode.link);
    sonde_xdg_decoration->request_mode.notify = NULL;
  }
  // remove commit listener if we haven't already
  if (sonde_xdg_decoration->surface_commit.notify != NULL) {
    wl_list_remove(&sonde_xdg_decoration->surface_commit.link);
    sonde_xdg_decoration->surface_commit.notify = NULL;
  }
}

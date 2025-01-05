#include "decorations.h"
#include "view.h"
#include "xdg-shell.h"

void sonde_apply_decorations(struct sonde_xdg_decoration *decoration) {  
  wlr_log(WLR_INFO, "applying decorations %s", decoration->sonde_xdg_view->toplevel->app_id);

  if (decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {    
    sonde_view_t view = &decoration->sonde_xdg_view->base;
    
    struct wlr_scene_tree *ssd_scene_tree = wlr_scene_tree_create(view->scene_tree);
    wlr_scene_node_lower_to_bottom(&ssd_scene_tree->node);

    float bg_color[4] = {1, 0.2, 0.2, 1.0}; // red

    sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(view);
    

    struct wlr_box toplevel_box;
    
    wlr_xdg_surface_get_geometry(sonde_xdg_view->toplevel->base, &toplevel_box);
    wlr_log(WLR_DEBUG, "LIFECYCLE toplevel extents %dx%d @ (%d, %d)", toplevel_box.width, toplevel_box.height, toplevel_box.x, toplevel_box.y);
    decoration->rect = wlr_scene_rect_create(ssd_scene_tree, toplevel_box.width, toplevel_box.height, bg_color);
    
    wlr_scene_node_set_position(&ssd_scene_tree->node, 0, 0);
    wlr_scene_node_set_position(&view->surface_scene_tree->node, 5, 5);

    wlr_xdg_toplevel_set_size(sonde_xdg_view->toplevel, toplevel_box.width - 10, toplevel_box.height - 10);
  }
}

void sonde_decoration_set_focus(struct sonde_xdg_decoration *decoration,
                                bool focused) {
  
}

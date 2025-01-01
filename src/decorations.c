#include "decorations.h"

void sonde_apply_decorations(
    struct sonde_xdg_decoration *sonde_xdg_decoration) {
  wlr_log(WLR_INFO, "applying decorations %s", sonde_xdg_decoration->sonde_xdg_view->toplevel->app_id);

  if (sonde_xdg_decoration->client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE) {
    struct wlr_scene_tree *ssd_scene_tree = wlr_scene_tree_create(sonde_xdg_decoration->sonde_xdg_view->base.scene_tree);
    wlr_scene_node_lower_to_bottom(&ssd_scene_tree->node);
  }
}

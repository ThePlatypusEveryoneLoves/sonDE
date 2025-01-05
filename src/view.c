#include "view.h"
#include "cursor.h"
#include "xdg-shell.h"
#include "decorations.h"

sonde_view_t sonde_view_at(sonde_server_t server, double lx,
                                         double ly,
                                         struct wlr_surface **surface,
                                         double *sx, double *sy) {
  struct wlr_scene_node *node =
      wlr_scene_node_at(&server->scene->tree.node, lx, ly, sx, sy);
  // only looking for surface nodes
  if (node == NULL || node->type != WLR_SCENE_NODE_BUFFER)
    return NULL;
  // get surface from buffer
  struct wlr_scene_buffer *scene_buffer = wlr_scene_buffer_from_node(node);
  struct wlr_scene_surface *scene_surface =
      wlr_scene_surface_try_from_buffer(scene_buffer);
  if (!scene_surface)
    return NULL;

  if (surface != NULL)
    *surface = scene_surface->surface;

  // find the root scene node (we set the data field on this)
  struct wlr_scene_tree *tree = node->parent;
  while (tree != NULL && tree->node.data == NULL)
    tree = tree->node.parent;
  return tree == NULL ? NULL : tree->node.data;
}

// move a toplevel to front
void sonde_view_focus(sonde_view_t sonde_view) {
  if (sonde_view == NULL)
    return;

  struct wlr_surface *current_surface =
      sonde_view->server->seat->keyboard_state.focused_surface;
  struct wlr_surface *target_surface = sonde_view->surface;

  if (current_surface == target_surface)
    return;

  if (current_surface != NULL) {
    // deactivate focus
    struct wlr_xdg_toplevel *current_toplevel =
        wlr_xdg_toplevel_try_from_wlr_surface(current_surface);
    if (current_toplevel != NULL) {
      wlr_xdg_toplevel_set_activated(current_toplevel, false);

      // get the sonde_xdg_view
      sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_wlr_xdg_toplevel(current_toplevel);
      sonde_decoration_set_focus(&sonde_xdg_view->base.decoration, false);
    }
  }

  // move to front
  wlr_scene_node_raise_to_top(&sonde_view->scene_tree->node);
  // move to front of server.toplevels
  wl_list_remove(&sonde_view->link);
  wl_list_insert(&sonde_view->server->views,
                 &sonde_view->link);

  // activate
  if (sonde_view->type == SONDE_XDG) {
    wlr_xdg_toplevel_set_activated(sonde_xdg_view_from_sonde_view(sonde_view)->toplevel, true);
  }

  #ifdef SONDE_XWAYLAND
  else if (sonde_view->type == SONDE_XWAYLAND) {
    // TODO: Xwayland
    // wlr_xwayalnd_surface_offer_focus
  }
  #endif

  sonde_decoration_set_focus(&sonde_view->decoration, true);

  // move keyboard focus
  struct wlr_keyboard *keyboard =
      wlr_seat_get_keyboard(sonde_view->server->seat);
  if (keyboard != NULL)
    wlr_seat_keyboard_notify_enter(
        sonde_view->server->seat, target_surface, keyboard->keycodes,
        keyboard->num_keycodes, &keyboard->modifiers);

  // apply (or unapply) the pointer constraint
  struct wlr_pointer_constraint_v1 *pointer_constraint = wlr_pointer_constraints_v1_constraint_for_surface(sonde_view->server->pointer_constraints, target_surface, sonde_view->server->seat);
  sonde_cursor_set_pointer_constraint(sonde_view->server, pointer_constraint);
}

#include "xdg-shell.h"
#include "common.h"
#include "view.h"
#include "wlr/types/wlr_xdg_decoration_v1.h"
#include <assert.h>
// move a toplevel to front
void sonde_xdg_view_focus(struct sonde_xdg_view *sonde_xdg_view) {
  if (sonde_xdg_view == NULL)
    return;

  struct wlr_surface *current_surface =
      sonde_xdg_view->base.server->seat->keyboard_state.focused_surface;
  struct wlr_surface *target_surface = sonde_xdg_view->toplevel->base->surface;

  if (current_surface == target_surface)
    return;

  if (current_surface != NULL) {
    // deactivate focus
    struct wlr_xdg_toplevel *current_toplevel =
        wlr_xdg_toplevel_try_from_wlr_surface(current_surface);
    if (current_toplevel != NULL)
      wlr_xdg_toplevel_set_activated(current_toplevel, false);
  }

  // move to front
  wlr_scene_node_raise_to_top(&sonde_xdg_view->base.scene_tree->node);
  // move to front of server.toplevels
  wl_list_remove(&sonde_xdg_view->base.link);
  wl_list_insert(&sonde_xdg_view->base.server->toplevels,
                 &sonde_xdg_view->base.link);

  // activate
  wlr_xdg_toplevel_set_activated(sonde_xdg_view->toplevel, true);

  // move keyboard focus
  struct wlr_keyboard *keyboard =
      wlr_seat_get_keyboard(sonde_xdg_view->base.server->seat);
  if (keyboard != NULL)
    wlr_seat_keyboard_notify_enter(
        sonde_xdg_view->base.server->seat, target_surface, keyboard->keycodes,
        keyboard->num_keycodes, &keyboard->modifiers);
}

struct sonde_xdg_view *sonde_xdg_view_at(sonde_server_t server, double lx,
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

WL_CALLBACK(on_toplevel_map) {
  struct sonde_xdg_view *sonde_xdg_view =
      wl_container_of(listener, sonde_xdg_view, map);

  wl_list_insert(&sonde_xdg_view->base.server->toplevels,
                 &sonde_xdg_view->base.link);

  sonde_xdg_view_focus(sonde_xdg_view);
}

WL_CALLBACK(on_toplevel_unmap) {
  struct sonde_view *sonde_view = wl_container_of(listener, sonde_view, unmap);

  // TODO: reset cursor mode if this was being grabbed

  wl_list_remove(&sonde_view->link);
}

WL_CALLBACK(on_toplevel_commit) {
  struct sonde_xdg_view *sonde_xdg_view = wl_container_of(
      wl_container_of(listener, sonde_view, commit), sonde_xdg_view, base);

  if (sonde_xdg_view->toplevel->base->initial_commit) {
    // configuration for the first commit  (we could change geometry here)
    wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
    /*Let the client decide the size that they want to be*/
    wlr_xdg_toplevel_set_size(sonde_xdg_view->toplevel, 0, 0);
  }
}

WL_CALLBACK(on_toplevel_destroy) {
  struct sonde_view *sonde_view = wl_container_of(listener, sonde_view, unmap);

  wl_list_remove(&sonde_view->map.link);
  wl_list_remove(&sonde_view->unmap.link);
  wl_list_remove(&sonde_view->commit.link);
  wl_list_remove(&sonde_view->destroy.link);
  wl_list_remove(&sonde_view->request_move.link);
  wl_list_remove(&sonde_view->request_resize.link);
  wl_list_remove(&sonde_view->request_maximize.link);
  wl_list_remove(&sonde_view->request_fullsize.link);

  free(sonde_xdg_view);
}

WL_CALLBACK(on_toplevel_request_move) {
  struct sonde_xdg_view *sonde_xdg_view =
      wl_container_of(listener, sonde_xdg_view, request_move);
}

WL_CALLBACK(on_toplevel_request_resize) {
  struct sonde_xdg_view *sonde_xdg_view =
      wl_container_of(listener, sonde_xdg_view, request_resize);
}

WL_CALLBACK(on_toplevel_request_maximize) {
  struct sonde_xdg_view *sonde_xdg_view =
      wl_container_of(listener, sonde_xdg_view, request_maximize);
}

WL_CALLBACK(on_toplevel_request_fullscreen) {
  struct sonde_xdg_view *sonde_xdg_view =
      wl_container_of(listener, sonde_xdg_view, request_fullscreen);
}

WL_CALLBACK(on_new_toplevel) {
  sonde_server_t server = wl_container_of(listener, server, new_toplevel);

  struct wlr_xdg_toplevel *toplevel = data;

  // make a sonde_xdg_view
  struct sonde_xdg_view *sonde_xdg_view = calloc(1, sizeof(*sonde_xdg_view));
  sonde_xdg_view->base.server = server;
  sonde_xdg_view->toplevel = toplevel;
  sonde_xdg_view->base.scene_tree =
      wlr_scene_xdg_surface_create(&server->scene->tree, toplevel->base);
  // set the data field on the scene tree node
  sonde_xdg_view->base.scene_tree->node.data = sonde_xdg_view;
  // set the user data field to the scene tree so we can use in on_new_popup
  // below
  toplevel->base->data = sonde_xdg_view->base.scene_tree;

  // event listeners
  LISTEN(&toplevel->base->surface->events.map, &sonde_xdg_view->base.map,
         on_toplevel_map);
  LISTEN(&toplevel->base->surface->events.unmap, &sonde_xdg_view->base.unmap,
         on_toplevel_unmap);
  LISTEN(&toplevel->base->surface->events.commit, &sonde_xdg_view->base.commit,
         on_toplevel_commit);
  LISTEN(&toplevel->events.destroy, &sonde_xdg_view->base.destroy,
         on_toplevel_destroy);
  LISTEN(&toplevel->events.request_move, &sonde_xdg_view->base.request_move,
         on_toplevel_request_move);
  LISTEN(&toplevel->events.request_resize, &sonde_xdg_view->base.request_resize,
         on_toplevel_request_resize);
  LISTEN(&toplevel->events.request_maximize,
         &sonde_xdg_view->base.request_maximize, on_toplevel_request_maximize);
  LISTEN(&toplevel->events.request_fullscreen,
         &sonde_xdg_view->base.request_fullsize,
         on_toplevel_request_fullscreen);
}

WL_CALLBACK(on_popup_commit) {
  struct sonde_popup *sonde_popup =
      wl_container_of(listener, sonde_popup, commit);

  if (sonde_popup->popup->base->initial_commit) {
    // configuration for the first commit  (we could change geometry here)
    wlr_xdg_surface_schedule_configure(sonde_popup->popup->base);
  }
}

WL_CALLBACK(on_popup_destroy) {
  struct sonde_popup *sonde_popup =
      wl_container_of(listener, sonde_popup, destroy);

  wl_list_remove(&sonde_popup->commit.link);
  wl_list_remove(&sonde_popup->destroy.link);

  free(sonde_popup);
}

WL_CALLBACK(on_new_popup) {
  sonde_server_t server = wl_container_of(listener, server, new_popup);

  struct wlr_xdg_popup *popup = data;

  // make a sonde popup
  struct sonde_popup *sonde_popup = calloc(1, sizeof(*sonde_popup));
  sonde_popup->server = server;
  sonde_popup->popup = popup;

  struct wlr_xdg_surface *parent =
      wlr_xdg_surface_try_from_wlr_surface(popup->parent);
  assert(parent != NULL);
  // fetch the scene tree from the parent user data
  struct wlr_scene_tree *parent_tree = parent->data;
  popup->base->data = wlr_scene_xdg_surface_create(parent_tree, popup->base);

  LISTEN(&popup->base->surface->events.commit, &sonde_popup->commit,
         on_popup_commit);
  LISTEN(&popup->base->surface->events.destroy, &sonde_popup->destroy,
         on_popup_destroy);
}

int sonde_xdg_shell_initialize(sonde_server_t server) {
  // XDG Shell
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  if (server->xdg_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create xdg-shell");
    return 1;
  }

  LISTEN(&server->xdg_shell->events.new_toplevel, &server->new_toplevel,
         on_new_toplevel);
  LISTEN(&server->xdg_shell->events.new_popup, &server->new_popup,
         on_new_popup);

  server->server_decoration_manager =
      wlr_server_decoration_manager_create(server->display);
  wlr_server_decoration_manager_set_default_mode(
      server->server_decoration_manager,
      WLR_SERVER_DECORATION_MANAGER_MODE_SERVER);
  server->decoration_manager =
      wlr_xdg_decoration_manager_v1_create(server->display);

  return 0;
}

void sonde_xdg_shell_destroy(sonde_server_t server) {
  // nothing to do here for now
}

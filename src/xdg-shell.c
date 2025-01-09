#include "xdg-shell.h"
#include "common.h"
#include "view.h"
#include "decorations.h"
#include "decoration-manager.h"
#include <assert.h>

WL_CALLBACK(on_toplevel_map) {
  sonde_view_t sonde_view = wl_container_of(listener, sonde_view, map);

  // insert into server toplevels list
  wl_list_insert(&sonde_view->server->views, &sonde_view->link);

  // decorate
  sonde_apply_decorations(&sonde_view->decoration);
  
  sonde_view_focus(sonde_view);

  wlr_log(WLR_DEBUG, "LIFECYCLE %s map", sonde_xdg_view_from_sonde_view(sonde_view)->toplevel->app_id);
}

WL_CALLBACK(on_toplevel_unmap) {
  sonde_view_t sonde_view = wl_container_of(listener, sonde_view, unmap);

  // TODO: reset cursor mode if this was being grabbed

  wl_list_remove(&sonde_view->link);

  wlr_log(WLR_DEBUG, "LIFECYCLE %s unmap", sonde_xdg_view_from_sonde_view(sonde_view)->toplevel->app_id);
}

WL_CALLBACK(on_toplevel_commit) {
  sonde_view_t sonde_view = wl_container_of(listener, sonde_view, commit);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);

  if (sonde_xdg_view->toplevel->base->initial_commit) {
    // configuration for the first commit  (we could change geometry here)
    wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
    /*Let the client decide the size that they want to be*/
    // TODO: tiling stuff, prespecified size
    wlr_xdg_toplevel_set_size(sonde_xdg_view->toplevel, 0, 0);
  }
}

WL_CALLBACK(on_toplevel_destroy) {
  sonde_view_t sonde_view = wl_container_of(listener, sonde_view, destroy);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);

  sonde_decoration_destroy(&sonde_view->decoration);
  wlr_scene_node_destroy(&sonde_view->scene_tree->node);

  wl_list_remove(&sonde_view->map.link);
  wl_list_remove(&sonde_view->unmap.link);
  wl_list_remove(&sonde_view->commit.link);
  wl_list_remove(&sonde_view->destroy.link);
  wl_list_remove(&sonde_view->request_move.link);
  wl_list_remove(&sonde_view->request_resize.link);
  wl_list_remove(&sonde_view->request_maximize.link);
  wl_list_remove(&sonde_view->request_fullscreen.link);

  free(sonde_xdg_view);
}

WL_CALLBACK(on_toplevel_request_move) {
  sonde_view_t sonde_view = wl_container_of(listener, sonde_view, request_move);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);
  wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
}

WL_CALLBACK(on_toplevel_request_resize) {
  wlr_log(WLR_INFO, "request resize");
  sonde_view_t sonde_view =
      wl_container_of(listener, sonde_view, request_resize);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);
  wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
  sonde_decoration_update_size(&sonde_view->decoration);
}

WL_CALLBACK(on_toplevel_request_maximize) {
  sonde_view_t sonde_view =
      wl_container_of(listener, sonde_view, request_maximize);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);
  wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
}

WL_CALLBACK(on_toplevel_request_fullscreen) {
  sonde_view_t sonde_view =
      wl_container_of(listener, sonde_view, request_fullscreen);
  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_sonde_view(sonde_view);
  wlr_xdg_surface_schedule_configure(sonde_xdg_view->toplevel->base);
}

WL_CALLBACK(on_toplevel_set_title) {
  sonde_view_t sonde_view =
      wl_container_of(listener, sonde_view, set_title);
  sonde_decoration_update_title(&sonde_view->decoration);
}

WL_CALLBACK(on_new_toplevel) {
  sonde_server_t server = wl_container_of(listener, server, new_toplevel);

  struct wlr_xdg_toplevel *toplevel = data;

  // make a sonde_xdg_view/sonde_view
  sonde_xdg_view_t sonde_xdg_view = calloc(1, sizeof(*sonde_xdg_view));
  sonde_xdg_view->base.server = server;
  sonde_xdg_view->toplevel = toplevel;
  sonde_xdg_view->base.scene_tree = wlr_scene_tree_create(&server->scene->tree);
  // create toplevel surface tree
  sonde_xdg_view->base.surface_scene_tree = wlr_scene_xdg_surface_create(sonde_xdg_view->base.scene_tree, toplevel->base);
  // set the data field on the scene tree node
  sonde_xdg_view->base.scene_tree->node.data = sonde_xdg_view;
  sonde_xdg_view->base.surface = toplevel->base->surface;
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
         &sonde_xdg_view->base.request_fullscreen,
         on_toplevel_request_fullscreen);
  LISTEN(&toplevel->events.set_title,
         &sonde_xdg_view->base.set_title,
         on_toplevel_set_title);
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
  wl_list_init(&server->views);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  if (server->xdg_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create xdg-shell");
    return 1;
  }

  LISTEN(&server->xdg_shell->events.new_toplevel, &server->new_toplevel,
         on_new_toplevel);
  LISTEN(&server->xdg_shell->events.new_popup, &server->new_popup,
         on_new_popup);

  return 0;
}

void sonde_xdg_shell_destroy(sonde_server_t server) {
  // nothing to do here for now
}

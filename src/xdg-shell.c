#include <assert.h>
#include <wayland-server-core.h>
#include "xdg-shell.h"

void on_toplevel_map(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, map);

  wl_list_insert(&sonde_toplevel->server->toplevels, &sonde_toplevel->link);

  // TODO: keyboard focus
}

void on_toplevel_unmap(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, unmap);

  // TODO: reset cursor mode if this was being grabbed

  wl_list_remove(&sonde_toplevel->link);
}

void on_toplevel_commit(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, commit);

  if (sonde_toplevel->toplevel->base->initial_commit) {
    // configuration for the first commit  (we could change geometry here)
    wlr_xdg_surface_schedule_configure(sonde_toplevel->toplevel->base);
  }
}

void on_toplevel_destroy(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, destroy);

  wl_list_remove(&sonde_toplevel->map.link);
  wl_list_remove(&sonde_toplevel->unmap.link);
  wl_list_remove(&sonde_toplevel->commit.link);
  wl_list_remove(&sonde_toplevel->destroy.link);
  wl_list_remove(&sonde_toplevel->request_move.link);
  wl_list_remove(&sonde_toplevel->request_resize.link);
  wl_list_remove(&sonde_toplevel->request_maximize.link);
  wl_list_remove(&sonde_toplevel->request_fullscreen.link);

  free(sonde_toplevel);
}

void on_toplevel_request_move(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, request_move);
}

void on_toplevel_request_resize(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, request_resize);
}

void on_toplevel_request_maximize(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, request_maximize);
}

void on_toplevel_request_fullscreen(struct wl_listener *listener, void *data) {
  struct sonde_toplevel *sonde_toplevel = wl_container_of(listener, sonde_toplevel, request_fullscreen);
}

void on_new_toplevel(struct wl_listener *listener, void *data) {
  sonde_server_t server = wl_container_of(listener, server, new_toplevel);

  struct wlr_xdg_toplevel *toplevel = data;

  // make a sonde_toplevel
  struct sonde_toplevel *sonde_toplevel = calloc(1, sizeof(*sonde_toplevel));
  sonde_toplevel->server = server;
  sonde_toplevel->toplevel = toplevel;
  sonde_toplevel->scene_tree = wlr_scene_xdg_surface_create(&server->scene->tree, toplevel->base);
  sonde_toplevel->scene_tree->node.data = toplevel;
  // set the user data field to the scene tree so we can use in on_new_popup below
  toplevel->base->data = sonde_toplevel->scene_tree;

  // event listeners
  sonde_toplevel->map.notify = on_toplevel_map;
  sonde_toplevel->unmap.notify = on_toplevel_unmap;
  sonde_toplevel->commit.notify = on_toplevel_commit;
  sonde_toplevel->destroy.notify = on_toplevel_destroy;
  sonde_toplevel->request_move.notify = on_toplevel_request_move;
  sonde_toplevel->request_resize.notify = on_toplevel_request_resize;
  sonde_toplevel->request_maximize.notify = on_toplevel_request_maximize;
  sonde_toplevel->request_fullscreen.notify = on_toplevel_request_fullscreen;

  wl_signal_add(&toplevel->base->surface->events.map, &sonde_toplevel->map);
  wl_signal_add(&toplevel->base->surface->events.unmap, &sonde_toplevel->unmap);
  wl_signal_add(&toplevel->base->surface->events.commit, &sonde_toplevel->commit);
  wl_signal_add(&toplevel->events.destroy, &sonde_toplevel->destroy);
  wl_signal_add(&toplevel->events.request_move, &sonde_toplevel->request_move);
  wl_signal_add(&toplevel->events.request_resize, &sonde_toplevel->request_resize);
  wl_signal_add(&toplevel->events.request_maximize, &sonde_toplevel->request_maximize);
  wl_signal_add(&toplevel->events.request_fullscreen, &sonde_toplevel->request_fullscreen);
}

void on_popup_commit(struct wl_listener *listener, void *data) {
  struct sonde_popup *sonde_popup = wl_container_of(listener, sonde_popup, commit);

  if (sonde_popup->popup->base->initial_commit) {
    // configuration for the first commit  (we could change geometry here)
    wlr_xdg_surface_schedule_configure(sonde_popup->popup->base);
  }
}

void on_popup_destroy(struct wl_listener *listener, void *data) {
  struct sonde_popup *sonde_popup = wl_container_of(listener, sonde_popup, destroy);

  wl_list_remove(&sonde_popup->commit.link);
  wl_list_remove(&sonde_popup->destroy.link);

  free(sonde_popup);
}

void on_new_popup(struct wl_listener *listener, void *data) {
  sonde_server_t server = wl_container_of(listener, server, new_popup);

  struct wlr_xdg_popup *popup = data;

  // make a sonde popup
  struct sonde_popup *sonde_popup = calloc(1, sizeof(*sonde_popup));
  sonde_popup->server = server;
  sonde_popup->popup = popup;

  struct wlr_xdg_surface *parent = wlr_xdg_surface_try_from_wlr_surface(popup->parent);
  assert(parent != NULL);
  // fetch the scene tree from the parent user data
  struct wlr_scene_tree *parent_tree = parent->data;
  popup->base->data = wlr_scene_xdg_surface_create(parent_tree, popup->base);

  sonde_popup->commit.notify = on_popup_commit;
  sonde_popup->destroy.notify = on_popup_destroy;

  wl_signal_add(&popup->base->surface->events.commit, &sonde_popup->commit);
  wl_signal_add(&popup->base->surface->events.destroy, &sonde_popup->destroy);
}


int sonde_xdg_shell_initialize(sonde_server_t server) {
  // XDG Shell
  wl_list_init(&server->toplevels);
  server->xdg_shell = wlr_xdg_shell_create(server->display, 3);
  if (server->xdg_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create xdg-shell");
    return 1;
  }
  
  server->new_toplevel.notify = on_new_toplevel;
  server->new_popup.notify = on_new_popup;

  wl_signal_add(&server->xdg_shell->events.new_toplevel, &server->new_toplevel);
  wl_signal_add(&server->xdg_shell->events.new_popup, &server->new_popup);

  return 0;
}

void sonde_xdg_shell_destroy(sonde_server_t server) {
  // nothing to do here for now
}

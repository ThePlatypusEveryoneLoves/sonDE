#include "decoration-manager.h"
#include "decorations.h"
#include "xdg-shell.h"

WL_CALLBACK(on_decoration_surface_commit) {
  struct sonde_xdg_decoration *sonde_xdg_decoration = wl_container_of(listener, sonde_xdg_decoration, surface_commit);
  // we only care about the initial commit
  if (sonde_xdg_decoration->xdg_decoration->toplevel->base->initial_commit) {
    wlr_xdg_toplevel_decoration_v1_set_mode(sonde_xdg_decoration->xdg_decoration, sonde_xdg_decoration->client_mode);
  }

  // remove listener
  wl_list_remove(&sonde_xdg_decoration->surface_commit.link);
  sonde_xdg_decoration->surface_commit.notify = NULL;
}

// Client requests a decoration mode
WL_CALLBACK(on_decoration_request_mode) {
  struct sonde_xdg_decoration *sonde_xdg_decoration = wl_container_of(listener, sonde_xdg_decoration, request_mode);

  // the requested mode will have been set
  enum wlr_xdg_toplevel_decoration_v1_mode client_mode = sonde_xdg_decoration->xdg_decoration->requested_mode;

  // default to server side if no preference
  // TODO: config
  // TODO: allow user to _force_ ssd
  if (client_mode == WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_NONE)
    client_mode = WLR_XDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE;
  
  sonde_xdg_decoration->client_mode = client_mode;
}

WL_CALLBACK(on_decoration_destroy) {
  struct sonde_xdg_decoration *sonde_xdg_decoration = wl_container_of(listener, sonde_xdg_decoration, destroy);
  sonde_decoration_destroy(sonde_xdg_decoration);
}

WL_CALLBACK(on_new_toplevel_decoration) {
  struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration = data;
  struct wlr_xdg_surface *xdg_surface = xdg_decoration->toplevel->base;

  // surface needs to be non-null
  if (xdg_surface == NULL || xdg_surface->data == NULL) {
    wlr_log(WLR_ERROR, "invalid surface for XDG decoration (app_id: %s)", xdg_surface->toplevel->app_id);
    return;
  }

  sonde_xdg_view_t sonde_xdg_view = sonde_xdg_view_from_wlr_xdg_toplevel(xdg_decoration->toplevel);

  // initialize the sonde_decoration events
  struct sonde_xdg_decoration *sonde_xdg_decoration = &sonde_xdg_view->base.decoration;
  sonde_xdg_decoration->xdg_decoration = xdg_decoration;

  // events
  LISTEN(&xdg_decoration->events.request_mode, &sonde_xdg_decoration->request_mode, on_decoration_request_mode);
  LISTEN(&xdg_decoration->events.destroy, &sonde_xdg_decoration->destroy, on_decoration_destroy);
  LISTEN(&xdg_surface->surface->events.commit, &sonde_xdg_decoration->surface_commit, on_decoration_surface_commit);
}

int sonde_decoration_manager_initialize(sonde_server_t server) {
    server->decoration_manager =
      wlr_xdg_decoration_manager_v1_create(server->display);

  if (server->decoration_manager == NULL) {
    wlr_log(WLR_ERROR, "failed to create kde decoration manager");
    return 1;
  }

  // events
  LISTEN(&server->decoration_manager->events.new_toplevel_decoration, &server->new_toplevel_decoration, on_new_toplevel_decoration);

  return 0;
}

void sonde_decoration_manager_destroy(sonde_server_t server) {
  // nothing here for now
}

#include "layer-shell.h"
#include "decorations.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

WL_CALLBACK(on_surface_map) {
  struct sonde_layer_surface *layer_surface = wl_container_of(listener, layer_surface, map);

  // insert into server toplevels list
  wl_list_insert(&layer_surface->server->layer_shell_surfaces, &layer_surface->link);
}

WL_CALLBACK(on_surface_unmap) {
  struct sonde_layer_surface *layer_surface = wl_container_of(listener, layer_surface, unmap);

  wl_list_remove(&layer_surface->link);
}

WL_CALLBACK(on_surface_commit) {
  struct sonde_layer_surface *layer_surface = wl_container_of(listener, layer_surface, commit);

  if (layer_surface->layer_surface->initial_commit) {
    struct wlr_output *output = layer_surface->layer_surface->output;
    switch (layer_surface->layer_surface->current.layer) {
    case ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND:
      // size to full w/h
      wlr_layer_surface_v1_configure(layer_surface->layer_surface, output->width, output->height);
      // move to back
      wlr_scene_node_lower_to_bottom(&layer_surface->scene_tree->tree->node);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM:
      wlr_layer_surface_v1_configure(layer_surface->layer_surface, output->width, SONDE_BOTTOM_HEIGHT);
      // move to front
      wlr_scene_node_raise_to_top(&layer_surface->scene_tree->tree->node);
      // place at bottom of output
      wlr_scene_node_set_position(&layer_surface->scene_tree->tree->node, 0, output->height - SONDE_BOTTOM_HEIGHT);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_TOP:
      wlr_layer_surface_v1_configure(layer_surface->layer_surface, output->width, SONDE_BOTTOM_HEIGHT);
      // move to front
      wlr_scene_node_raise_to_top(&layer_surface->scene_tree->tree->node);
      break;
    case ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY:
      wlr_layer_surface_v1_configure(layer_surface->layer_surface, output->width, output->height);
      // move to front
      wlr_scene_node_raise_to_top(&layer_surface->scene_tree->tree->node);
      break;
    }
  }
}

WL_CALLBACK(on_surface_destroy) {
  struct sonde_layer_surface *layer_surface = wl_container_of(listener, layer_surface, destroy);

  wl_list_remove(&layer_surface->map.link);
  wl_list_remove(&layer_surface->unmap.link);
  wl_list_remove(&layer_surface->commit.link);
  wl_list_remove(&layer_surface->destroy.link);

  free(layer_surface);
}

WL_CALLBACK(on_new_surface) {
  struct wlr_layer_surface_v1 *surface = data;
  sonde_server_t server = wl_container_of(listener, server, new_layer_shell_surface);

  // make a sonde_xdg_view/sonde_view
  struct sonde_layer_surface *sonde_layer_surface = calloc(1, sizeof(*sonde_layer_surface));
  sonde_layer_surface->server = server;
  sonde_layer_surface->layer_surface = surface;
  sonde_layer_surface->surface = surface->surface;
  sonde_layer_surface->scene_tree = wlr_scene_layer_surface_v1_create(&server->scene->tree, surface);
  // set the data field on the scene tree node
  sonde_layer_surface->scene_tree->tree->node.data = sonde_layer_surface;
  // set the user data field to the scene tree so we can use in on_new_popup
  // below
  surface->surface->data = sonde_layer_surface->scene_tree->tree;

  // event listeners
  LISTEN(&surface->surface->events.map, &sonde_layer_surface->map,
         on_surface_map);
  LISTEN(&surface->surface->events.commit, &sonde_layer_surface->commit,
         on_surface_commit);
  LISTEN(&surface->surface->events.unmap, &sonde_layer_surface->unmap,
         on_surface_unmap);
  LISTEN(&surface->events.destroy, &sonde_layer_surface->destroy,
         on_surface_destroy);
}

int sonde_layer_shell_initialize(sonde_server_t server) {
  wl_list_init(&server->layer_shell_surfaces);
  server->layer_shell = wlr_layer_shell_v1_create(server->display, 4);
  if (server->layer_shell == NULL) {
    wlr_log(WLR_ERROR, "failed to create wlr layer shell");
    return 1;
  }

  LISTEN(&server->layer_shell->events.new_surface, &server->new_layer_shell_surface, on_new_surface);

  return 0;
}

void sonde_layer_shell_destroy(sonde_server_t server) {
  // nothing to do here for now
}

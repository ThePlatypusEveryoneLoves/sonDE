#include "outputs.h"

WL_CALLBACK(on_output_frame) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, frame);
  struct wlr_scene *scene = sonde_output->server->scene;

  struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(scene, sonde_output->output);

  // render
  wlr_scene_output_commit(scene_output, NULL);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}
WL_CALLBACK(on_output_request_state) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, request_state);
  const struct wlr_output_event_request_state *event = data;
  // commit the state again
  wlr_output_commit_state(sonde_output->output, event->state);
}
WL_CALLBACK(on_output_destroy) {
  struct sonde_output *sonde_output = wl_container_of(listener, sonde_output, destroy);

  wl_list_remove(&sonde_output->frame.link);
  wl_list_remove(&sonde_output->request_state.link);
  wl_list_remove(&sonde_output->destroy.link);
  wl_list_remove(&sonde_output->link);
  free(sonde_output);
}

WL_CALLBACK(on_new_output) {
  sonde_server_t server = wl_container_of(listener, server, new_output);
  struct wlr_output *output = data;

  // configure output
  wlr_output_init_render(output, server->allocator, server->renderer);

  struct wlr_output_state state;
  wlr_output_state_init(&state);

  // enable
  wlr_output_state_set_enabled(&state, true);

  // mode is resolution and refresh rate
  // set mode to output's perferred if needed
  // TODO: add output configuration
  struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
  if (mode != NULL) {
    wlr_output_state_set_mode(&state, mode);
  }

  // commit this state
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);

  struct sonde_output *sonde_output = calloc(1, sizeof(*sonde_output));
  sonde_output->output = output;
  sonde_output->server = server;

  LISTEN(&output->events.frame, &sonde_output->frame, on_output_frame);
  LISTEN(&output->events.request_state, &sonde_output->request_state, on_output_request_state);
  LISTEN(&output->events.destroy, &sonde_output->destroy, on_output_destroy);

  wl_list_insert(&server->outputs, &sonde_output->link);

  // add to layout
  struct wlr_output_layout_output *l_output = wlr_output_layout_add_auto(server->output_layout, output);
  struct wlr_scene_output *scene_output = wlr_scene_output_create(server->scene, output);
  wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);
}

int sonde_outputs_initialize(sonde_server_t server) {
  // handles screen arrangement
  server->output_layout = wlr_output_layout_create(server->display);

  // outputs
  wl_list_init(&server->outputs);
  
  LISTEN(&server->backend->events.new_output, &server->new_output, on_new_output);

  server->scene = wlr_scene_create();
  server->scene_layout = wlr_scene_attach_output_layout(server->scene, server->output_layout);
  return 0;
}

void sonde_outputs_destroy(sonde_server_t server) {
  wlr_scene_node_destroy(&server->scene->tree.node);
}

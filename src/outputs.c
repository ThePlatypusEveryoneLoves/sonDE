#include "outputs.h"
#include "common.h"
#include "output-manager.h"
#include <wlr/backend/wayland.h>
#include <wlr/backend/x11.h>

WL_CALLBACK(on_output_frame) {
  struct sonde_output *sonde_output =
      wl_container_of(listener, sonde_output, frame);
  struct wlr_scene *scene = sonde_output->server->scene;

  struct wlr_scene_output *scene_output =
      wlr_scene_get_scene_output(scene, sonde_output->output);

  // render
  wlr_scene_output_commit(scene_output, NULL);

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  wlr_scene_output_send_frame_done(scene_output, &now);
}
WL_CALLBACK(on_output_request_state) {
  struct sonde_output *sonde_output =
      wl_container_of(listener, sonde_output, request_state);
  const struct wlr_output_event_request_state *event = data;
  // commit the state again
  wlr_output_commit_state(sonde_output->output, event->state);
}
WL_CALLBACK(on_output_destroy) {
  struct sonde_output *sonde_output =
      wl_container_of(listener, sonde_output, destroy);

  wl_list_remove(&sonde_output->frame.link);
  wl_list_remove(&sonde_output->request_state.link);
  wl_list_remove(&sonde_output->destroy.link);
  wl_list_remove(&sonde_output->link);

  // update output manager
  sonde_output_manager_update(sonde_output->server);
  
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

  struct wlr_output_mode *mode = sonde_get_output_or_preferred(output, &server->config);
  if (mode != NULL) {
    wlr_output_state_set_mode(&state, mode);
  }

  // If we're running inside an existing wayland session, set the title
  if (wlr_output_is_wl(output)) {
    wlr_wl_output_set_app_id(output, "sonde");
    wlr_wl_output_set_title(output, "sonDE");
  }
  if (wlr_output_is_x11(output)) {
    wlr_wl_output_set_title(output, "sonDE");
  }
  

  // commit this state
  wlr_output_commit_state(output, &state);
  wlr_output_state_finish(&state);

  struct sonde_output *sonde_output = calloc(1, sizeof(*sonde_output));
  sonde_output->output = output;
  sonde_output->server = server;

  LISTEN(&output->events.frame, &sonde_output->frame, on_output_frame);
  LISTEN(&output->events.request_state, &sonde_output->request_state,
         on_output_request_state);
  LISTEN(&output->events.destroy, &sonde_output->destroy, on_output_destroy);

  wl_list_insert(&server->outputs, &sonde_output->link);

  // add to layout
  struct wlr_output_layout_output *l_output =
      wlr_output_layout_add_auto(server->output_layout, output);
  struct wlr_scene_output *scene_output =
      wlr_scene_output_create(server->scene, output);
  wlr_scene_output_layout_add_output(server->scene_layout, l_output,
                                     scene_output);

  // update output manager
  sonde_output_manager_update(server);
}

int sonde_outputs_initialize(sonde_server_t server) {
  // handles screen arrangement
  server->output_layout = wlr_output_layout_create(server->display);

  // outputs
  wl_list_init(&server->outputs);

  LISTEN(&server->backend->events.new_output, &server->new_output,
         on_new_output);

  server->scene = wlr_scene_create();
  server->scene_layout =
      wlr_scene_attach_output_layout(server->scene, server->output_layout);
  return 0;
}

void sonde_outputs_destroy(sonde_server_t server) {
  wlr_scene_node_destroy(&server->scene->tree.node);
}

struct wlr_output_mode *
sonde_get_output_or_preferred(struct wlr_output *output,
                              struct sonde_config *config) {
  struct sonde_screen_config *screen_config = NULL;
  struct sonde_screen_config *item = NULL;
  ARRAY_FOREACH(&config->screens, item) {
    if (strcmp(output->name, item->name) == 0) {
      screen_config = item;
      break;
    }
  }
  if (!screen_config) {
    return wlr_output_preferred_mode(output);
  }
  /*Refresh rates from hertz to millihertz (not megahertz)*/
  int mhz = (int)roundf(screen_config->refresh_rate * 1000);
  struct wlr_output_mode *mode;
  wl_list_for_each(mode, &output->modes, link) {
    if (mode->width == screen_config->width &&
        mode->height == screen_config->height &&
        abs(mode->refresh - mhz) <= 100 /*tolerance*/) {
      return mode;
    }
  }
  return wlr_output_preferred_mode(output);
}

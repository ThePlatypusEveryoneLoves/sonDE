#include "common.h"
#include "seat.h"
#include "server.h"
#include "user_config.h"
#include "xdg-shell.h"
#include "keybinds.h"
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

// shift, ctrl, alt, etc
WL_CALLBACK(on_keyboard_modifiers) {
  struct sonde_keyboard *sonde_keyboard = wl_container_of(listener, sonde_keyboard, modifiers);

  // just passthrough

  // each seat can only have one keyboard, so set appropriately
  wlr_seat_set_keyboard(sonde_keyboard->server->seat, sonde_keyboard->keyboard);

  wlr_seat_keyboard_notify_modifiers(sonde_keyboard->server->seat, &sonde_keyboard->keyboard->modifiers);
}

// handle WM keybindings!!
static void handle_command(sonde_server_t server, struct sonde_keybind_command *command) {
  switch (command->type) {
  case SONDE_KEYBIND_COMMAND_EXIT:
    wl_display_terminate(server->display);
    break;
  case SONDE_KEYBIND_COMMAND_NEXT_VIEW:
    // get next toplevel
    if (wl_list_length(&server->views) < 2) break;

    sonde_view_t next_view = wl_container_of(server->views.prev, next_view, link);
    sonde_view_focus(next_view);
    break;
  case SONDE_KEYBIND_COMMAND_LAUNCH:
    // launch specified command

    if (command->data != NULL) {
      if (fork() == 0) {
        execl("/bin/sh", "/bin/sh", "-c", command->data, NULL);
      }
    }
    break;
  default:;
  }
}

WL_CALLBACK(on_keyboard_key) {
  struct sonde_keyboard *sonde_keyboard = wl_container_of(listener, sonde_keyboard, key);
  struct wlr_keyboard_key_event *event = data;

  // libinput -> xkbcommon
  uint32_t keycode = event->keycode + 8;

  // get the pressed keys
  const xkb_keysym_t *pressed_syms;
  int num_syms = xkb_state_key_get_syms(sonde_keyboard->keyboard->xkb_state, keycode, &pressed_syms);
  uint32_t modifiers = wlr_keyboard_get_modifiers(sonde_keyboard->keyboard);

  // Try WM keybindings
  if (event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
    for (int i = 0; i < num_syms; i++) {
      // try to process as a WM keybinding
      struct sonde_keybind_command *command = sonde_keybind_find(sonde_keyboard->server->config.keybinds, modifiers, pressed_syms[i]);
      if (command != NULL) {
        // handle command
        handle_command(sonde_keyboard->server, command);
        return;
      }
    }
  }

  // passthrough
  wlr_seat_set_keyboard(sonde_keyboard->server->seat, sonde_keyboard->keyboard);
  wlr_seat_keyboard_notify_key(sonde_keyboard->server->seat, event->time_msec, event->keycode, event->state);
}

WL_CALLBACK(on_keyboard_destroy) {
  struct sonde_keyboard *sonde_keyboard = wl_container_of(listener, sonde_keyboard, destroy);

  wl_list_remove(&sonde_keyboard->modifiers.link);
  wl_list_remove(&sonde_keyboard->key.link);
  wl_list_remove(&sonde_keyboard->destroy.link);
  wl_list_remove(&sonde_keyboard->link);

  free(sonde_keyboard);
}

static void new_keyboard(sonde_server_t server, struct wlr_input_device *device) {
  struct wlr_keyboard *keyboard = wlr_keyboard_from_input_device(device);

  struct sonde_keyboard *sonde_keyboard = calloc(1, sizeof(*sonde_keyboard));
  sonde_keyboard->server = server;
  sonde_keyboard->keyboard = keyboard;

  wlr_log(WLR_DEBUG, "new keyboard: %s", device->name);

  // apply XKB keymap

  struct sonde_keyboard_config *kbd_config = NULL;
  struct sonde_keyboard_config *item = NULL;
  ARRAY_FOREACH(&server->config.keyboards, item) {
    if (strcmp(device->name, item->name) == 0) {
      kbd_config = item;
      break;
    }
  }

  struct xkb_rule_names *keymap_config = kbd_config == NULL ? NULL : &kbd_config->inner.keymap;

  struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
  struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, keymap_config, XKB_KEYMAP_COMPILE_NO_FLAGS);
  wlr_keyboard_set_keymap(keyboard, keymap);
  xkb_keymap_unref(keymap);
  xkb_context_unref(context);

  if (kbd_config == NULL) {
    // default repeat rate
    wlr_keyboard_set_repeat_info(keyboard, SONDE_KEYBOARD_DEFAULT_REPEAT_RATE, SONDE_KEYBOARD_DEFAULT_REPEAT_DELAY);
  } else {
    wlr_keyboard_set_repeat_info(keyboard, kbd_config->inner.repeat_rate, kbd_config->inner.repeat_delay);
  }

  // events
  LISTEN(&keyboard->events.modifiers, &sonde_keyboard->modifiers, on_keyboard_modifiers);
  LISTEN(&keyboard->events.key, &sonde_keyboard->key, on_keyboard_key);
  LISTEN(&device->events.destroy, &sonde_keyboard->destroy, on_keyboard_destroy);

  wlr_seat_set_keyboard(server->seat, keyboard);
  wl_list_insert(&server->keyboards, &sonde_keyboard->link);
}

static void new_pointer(sonde_server_t server, struct wlr_input_device *device) {
  // attach pointer to server
  // TODO: pointer config
  wlr_cursor_attach_input_device(server->cursor, device);
}

WL_CALLBACK(on_new_input) {
  sonde_server_t server = wl_container_of(listener, server, new_input);

  struct wlr_input_device *device = data;
  switch (device->type) {
  case WLR_INPUT_DEVICE_KEYBOARD:
    new_keyboard(server, device);
    break;
  case WLR_INPUT_DEVICE_POINTER:
    new_pointer(server, device);
    break;
  default:
    break;
  }

  // set capabilities
  // TODO: no pointer capability w/o pointer
  uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
  if (!wl_list_empty(&server->keyboards)) {
    caps |= WL_SEAT_CAPABILITY_KEYBOARD;
  }
  wlr_seat_set_capabilities(server->seat, caps);
}
WL_CALLBACK(on_request_cursor) {
  sonde_server_t server = wl_container_of(listener, server, request_cursor);
}
WL_CALLBACK(on_request_set_selection) {
  sonde_server_t server = wl_container_of(listener, server, request_set_selection);
  struct wlr_seat_request_set_selection_event *event = data;
  wlr_seat_set_selection(server->seat, event->source, event->serial);
}

int sonde_seat_initialize(sonde_server_t server) {
  wl_list_init(&server->keyboards);
  server->seat = wlr_seat_create(server->display, "seat0");

  if (server->seat == NULL) {
    wlr_log(WLR_ERROR, "failed to create seat");
    return 1;
  }

  LISTEN(&server->backend->events.new_input, &server->new_input, on_new_input);
  LISTEN(&server->seat->events.request_set_cursor, &server->request_cursor, on_request_cursor);
  LISTEN(&server->seat->events.request_set_selection, &server->request_set_selection, on_request_set_selection);

  return 0;
}

void sonde_seat_destroy(sonde_server_t server) {
  wlr_seat_destroy(server->seat);
}

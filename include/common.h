#pragma once

#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>


#ifdef DEBUG_LOG
#define INIT_LOG() wlr_log_init(WLR_DEBUG, NULL);
#else
#define INIT_LOG() wlr_log_init(WLR_ERROR, NULL);
#endif

#define LISTEN(EVENT, LISTENER, CALLBACK)       \
  do {                                          \
    (LISTENER)->notify = CALLBACK;              \
    wl_signal_add((EVENT), (LISTENER));         \
  } while (false)

#define WL_CALLBACK(NAME) \
  static void NAME(struct wl_listener *listener, void *data)

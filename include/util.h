#pragma once

#ifdef DEBUG_LOG
#define INIT_LOG() wlr_log_init(WLR_DEBUG, NULL)
#else
#define INIT_LOG() wlr_log_init(WLR_ERROR, NULL)
#endif

#define LISTEN(EVENT, LISTENER, CALLBACK) \
  (LISTENER)->notify = CALLBACK; \
  wl_signal_add((EVENT), (LISTENER)); 

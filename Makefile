PKG_CONFIG?=pkg-config
WAYLAND_PROTOCOLS!=$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER!=$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS="wlroots-0.18" wayland-server xkbcommon lua pango cairo pangocairo
CFLAGS_PKG_CONFIG!=$(PKG_CONFIG) --cflags $(PKGS)
CFLAGS+=$(CFLAGS_PKG_CONFIG)
LIBS!=$(PKG_CONFIG) --libs $(PKGS)
SRCS:=$(wildcard src/*.c)
OBJS:=$(patsubst src/%.c,bin/objects/%.o,$(SRCS))
HEADERS:=$(wildcard include/*.h)

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
PROTOCOL_HEADERS=xdg-shell-protocol.h pointer-constraints-unstable-v1-protocol.h wlr-layer-shell-unstable-v1-protocol.h
PROTOCOL_HEADER_PATHS=$(addprefix include/protocols/,$(PROTOCOL_HEADERS))

all: bin/sonde

include/protocols:
	mkdir -p include/protocols

define WAYLAND_PROTOCOL_RULE
include/protocols/$(1): $(2) | include/protocols
	$(WAYLAND_SCANNER) server-header $$^ $$@
endef

bin/objects:
	mkdir -p bin/objects

# wayland headers
$(eval $(call WAYLAND_PROTOCOL_RULE,xdg-shell-protocol.h,$(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml))
$(eval $(call WAYLAND_PROTOCOL_RULE,pointer-constraints-unstable-v1-protocol.h,$(WAYLAND_PROTOCOLS)/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml))
$(eval $(call WAYLAND_PROTOCOL_RULE,wlr-layer-shell-unstable-v1-protocol.h,protocols/wlr-layer-shell-unstable-v1.xml))

bin/objects/%.o: src/%.c $(PROTOCOL_HEADER_PATHS) $(HEADERS) | bin/objects
	$(CC) -c $< -g -Werror $(CFLAGS) -DDEBUG_LOG -fsanitize=address -Iinclude -Iinclude/protocols -I/usr/include/pango1.0 -DWLR_USE_UNSTABLE -o $@

bin/sonde: $(OBJS)
	$(CC) $^ $> -g -Werror -DDEBUG_LOG $(CFLAGS) -fsanitize=address $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf bin include/protocols/

.PHONY: all clean

PKG_CONFIG?=pkg-config
WAYLAND_PROTOCOLS!=$(PKG_CONFIG) --variable=pkgdatadir wayland-protocols
WAYLAND_SCANNER!=$(PKG_CONFIG) --variable=wayland_scanner wayland-scanner

PKGS="wlroots-0.18" wayland-server xkbcommon lua
CFLAGS_PKG_CONFIG!=$(PKG_CONFIG) --cflags $(PKGS)
CFLAGS+=$(CFLAGS_PKG_CONFIG)
LIBS!=$(PKG_CONFIG) --libs $(PKGS)
SRCS:=$(wildcard src/*.c)
OBJS:=$(patsubst src/%.c,bin/objects/%.o,$(SRCS))
HEADERS:=$(wildcard include/*.h)

all: bin/sonde

bin:
	mkdir -p bin/objects

# wayland-scanner is a tool which generates C headers and rigging for Wayland
# protocols, which are specified in XML. wlroots requires you to rig these up
# to your build system yourself and provide them in the include path.
include/xdg-shell-protocol.h:
	$(WAYLAND_SCANNER) server-header $(WAYLAND_PROTOCOLS)/stable/xdg-shell/xdg-shell.xml $@

bin/objects/%.o: src/%.c include/xdg-shell-protocol.h $(HEADERS) | bin
	$(CC) -c $< -g -Werror $(CFLAGS) -DDEBUG_LOG -Iinclude -DWLR_USE_UNSTABLE -o $@

bin/sonde: $(OBJS)
	$(CC) $^ $> -g -Werror -DDEBUG_LOG $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

clean:
	rm -rf bin include/xdg-shell-protocol.h

.PHONY: all clean

# sonDE

An efficient yet easy to use Wayland window manager

*sonDE running kitty, Emacs, and Waybar*:

![screenshot](https://raw.githubusercontent.com/ThePlatypusEveryoneLoves/sonDE/refs/heads/main/contrib/screenshot.png)

## Installation

**From source:**

1. Clone the repo
2. `make release` (or just `make` for the development build)
3. `bin/sonde-release`

**Prebuilt x86_64 binaries:**

Can be found on the [releases page](https://github.com/ThePlatypusEveryoneLoves/sonDE/releases)

## Configuration

sonDE is configured through Lua. It reads config from a `sonde/config.lua` file in any XDG config directory. 

To get started, put something like this in `~/.config/sonde/config.lua`:

```lua
local wm = mod.ALT

config.keybinds[wm | key.ESCAPE] = commands.exit
config.keybinds[wm | key.RIGHT] = commands.next_window
config.keybinds[wm | key.LEFT] = commands.previous_window
config.keybinds[wm | key.RETURN] = commands.launch("/path/to/terminal")
config.keybinds[wm | key.Q] = commands.close_window
config.keybinds[wm | key.J] = commands.tile_left
config.keybinds[wm | key.K] = commands.tile_right
```

As of now, keyboards, outputs, and keybinds can be configured through the config file.

## Progress/support

sonDE supports the following Wayland extensions:

| Name | Support |
| - | - |
| XDG Shell | Full |
| XDG Output Management | Full |
| WLR Output Management | Full |
| Layer Shell | Partial |
| Pointer Constraints | Full |
| Workspaces | Planned |
| Session lock | Planned |

sonDE is **not** complete yet, so expect some bugs/crashes

# Sway Input Configurator
Create and/or apply your SwayWM configuration for input device using GUI

![Example1](doc/example1.png)

## Dependencies
- [ImGui Wrapper](https://github.com/TheRetikGM/imguiwrapper.git) (downloaded by meson if needed)
- Slurp (optional, area selecting for tablets)
- Sway (swaymsg must be in PATH)
- glfw

## Build

	git clone https://github.com/TheRetikGM/swic
	cd swic
	meson setup build && meson compile -C build

## Install
If you are using arch-based distro, then you can install this package using [AUR](https://aur.archlinux.org/packages/swic-git).

## Configuration
Configuration options are in `config.json` file which is in `$XDG_CONFIG_HOME/swic/config.json`. If the `XDG_CONFIG_HOME` env variable is not set then it is in `$HOME/swic/config.json`.

## TODO
- [ ] Command line option for disabling safe mode
- [ ] xkb options such as ~~numlock enabling~~ and keyboard languages
- [ ] Handle loading default values for parameters which cannot be retrieved from swaymsg calls
	- [ ] Implement reverting of applied values for these parameters
- [ ] Meson install setup
- [ ] Ability to save and load configuration profiles for input devices
- [ ] Add filters of devices
- [ ] Add configuration profiles

# Sway Input Configurator
Create sway configuration for your input device using GUI

![Example1](doc/example1.png)

## Dependencies
- [ImGui Wrapper](https://github.com/TheRetikGM/imguiwrapper.git)
- Slurp (optional)
- Sway
- glfw

## Build

	git clone https://github.com/TheRetikGM/swic
	cd swic
	meson setup build && meson compile -C build

## TODO
- [ ] Command line option for disabling safe mode
- [ ] xkb options such as numlock enabling and keyboard languages
- [ ] Proper handling default configurations of mapping options
- [ ] Meson install setup

.PHONY: run

run: emulator
	./emulator examples/hello-world.lua
run-movement: emulator
	./emulator examples/movement.lua
static_game_data.h: artifacts/font.lua examples/hello-world.lua Makefile to_c.py
	# need to append a null byte at the end..
	python3 to_c.py artifacts/font.lua examples/hello-world.lua examples/map.p8 > static_game_data.h

emulator: pico_backend.c main.c engine.c parser.c sdl_backend.c Makefile static_game_data.h
	gcc main.c -g -Werror -I/usr/include/lua5.2 -I/usr/include/SDL2/ -llua5.2 -lSDL2 -lSDL2_image -o emulator -DSDL_BACKEND

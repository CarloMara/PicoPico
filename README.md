# Pico Pico

Project to attempt to create hardware for the [Pico-8](https://www.lexaloffle.com/pico-8.php).

I'd like to eventually get a very basic game running on a ~[Raspberry Pi Pico](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)~[ESP32](https://www.adafruit.com/product/3979)

Be aware, the code will be terrible, this is a project to learn:

* C
* SDL
* Embedded
* CMake
* Lua
    * And how to adapt / modify languages
* JIT-ting ??


This project uses [z8lua](https://github.com/DavidVentura/z8lua) to implement Pico8's lua dialect; I've whacked the original repo to make it build 
with the Pico as a target, it was all implicit casting, which I hopefully got right.

This project also gets some "inspiration" from [tac08](https://github.com/0xcafed00d/tac08/tree/master/src) - ideas for things I don't know how to solve, and the "firmware.lua" file 
for basic implementations.

## Demo

In emulator (SDL backed)

![Hello world](artifacts/hello_world.gif?raw=1)

In Raspberry pi Pico

https://user-images.githubusercontent.com/3650670/166146124-06b8b223-27b1-47ac-931f-67ba8b523d9e.mp4

Celeste in ESP32

https://user-images.githubusercontent.com/3650670/167492170-9b68312e-e375-4b8d-a32e-3a35d07acea9.mp4


# Basic analysis / feasibility:

Memory requirements:

* Spritesheet, at 128x128 in size = 16KB 
* Fontsheet, at 128x128 in size = 16KB
* Map, at 32x128 = 4 KB
* Flags, at 2x128 = 256B
* Sound effects, 64x84 = 5376B (~5KB)
    * Sound itself should come from flash
* Music patterns, 64x6 = 384B
    * Music itself should come from flash
* Front buffer 128x128 = 16KB (x/y position -> palette; could be 8KB if using only a nibble per pixel)
* Back buffer 128x128x2 = 32KB (x/y position -> color, at 16bpp)
* Game memory = 32KB
    * Lua = ???KB
    * how to measure?

~130KB, should have plenty of space for things going wrong. However, the biggest unknown is lua overhead.  
Both Spritesheet (16KB), Fontsheet (16KB) and Map (4KB) can be squeezed to half, as they need a nibble per pixel, instead of a byte.  
Not sure how much CPU it'd take to have every single render to the backbuffer expand the pixels back bytes.

And **most importantly** 2MB for game RAM (Lua memory)

# Current performance

Running `hello_world.lua`:

* Normal: 13ms / frame; of which:
    * Lua: ~9ms / frame
    * Copying backbuffer to screen (`uint8_t`): ~4ms / frame

* Display on 2nd core: 7.5ms / frame; of which:
    * Lua: ~7ms / frame
    * ~Copying backbuffer to screen~ happens "for free" in the other core

* Multicore + overclock to 260MHz: 3.5ms/frame

# TODO

Immediate:

* ~Get basic rendering on the Pico~
    * ~split backends properly~
* ~Read code from p8 file instead of having split files~
* ~Implement map~
* ~Implement camera~
* ~Lua dialect~ (using z8lua)
* ~Unify the build systems (Make for pc / CMake for pico)~
* Pre-encode the palette colors as a RGB565 `uint16_t`; makes no sense to shift them on _every pixel write_

Later:

* ~Clock rate on SPI?~ set to 62.5MHz; not sure if it can go higher
* Implement flash
    * cartdata command could use it
    * carts could be stored in flash instead of static data
    * music, sfx
* Investigate pushing pixels to display via DMA
* Sound ??
* Look at optimizing lua bytecode for "fast function calls", for "standard library"

# Other stuff

There's a basic RLE encoding mechanism in place, to compress:

* Font data (16.5KB -> 5KB)
* Examples:
    * Hello wolrd: 4KB -> 1KB
    * Dice: 25KB -> 8KB
    * Tennis: 21KB -> 4KB

Not entirely sure yet why I was running out of memory, even with 40KB of (font+dice), it should be enough.

This whole thing (compression) is a bit of a hack -- it is nice for development to have full carts accessible; 
there is no code to read/write flash, nor any way to store the actual carts on flash yet.

# API Support

## Graphics

|    Function   | Supported |                     Notes |
|---------------|-----------|---------------------------|
|camera         |✅         |                           |
|circ           |✅         |                           |
|circfill       |✅         |                           |
|oval           |❌         |                           |
|ovalfill       |❌         |                           |
|clip           |❌         |                           |
|cls            |✅         |                           |
|color          |❌         |                           |
|cursor         |❌         |                           |
|fget           |✅         |                           |
|fillp          |❌         |                           |
|fset           |❌         |                           |
|line           |❌         |                           |
|pal            |✅         |                           |
|palt           |✅         |                           |
|pget           |✅         |                           |
|print          |⚠️          |Does not automatically scroll|
|pset           |✅         |                           |
|rect           |✅         |                           |
|rectfill       |✅         |                           |
|sget           |✅         |                           |
|spr            |✅         |                           |
|sset           |❌         |                           |
|sspr           |⚠️          |                           |
|tline          |❌         |                           |

## Tables

All implemented (z8lua)

## Input

`btn` implemented; `btnp` is just an alias to `btn`

## Sound

Not implemented

## Map

All implemented

## Memory

Not implemented

## Math

All implemented (z8lua)

## Cartridge data

cartdata/dget/dset are _technically_ implemented, there's no persistence layer though.

cstore/reload are not implemented.

## Coroutines

Implemented by aliasing (z8lua)

## Strings 

Implemented (z8lua)

## Values and objects

Implemented (z8lua)

# Hardware

Build something like the [PicoSystem](https://shop.pimoroni.com/products/picosystem?variant=32369546985555) ?

# Development (without a second Pico running OpenOCD)

Add this block to a udev rule (adjust the paths to point to this repo)
```
SUBSYSTEM=="block", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0003", ACTION=="add", SYMLINK+="rp2040upl%n"
SUBSYSTEM=="block", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0003", ACTION=="add", RUN+="/home/david/git/luatest/udev_build.sh rp2040upl%n"
```

Then having an open minicom shell 
```
sudo minicom -b 115200 -D /dev/ttyACM1
```

you can press `r` on it to reboot into mass-storage mode; which will trigger the udev rules after a second or so.

# Useful links

* [Easy intro to Embedding lua](https://lucasklassmann.com/blog/2019-02-02-how-to-embeddeding-lua-in-c/#example-of-error-handling)
* [Lua 5.2 Manual](https://www.lua.org/manual/5.2/manual.html)
* [Storing data in flash](https://kevinboone.me/picoflash.html?i=1)
* [Udev rule to copy build automatically](https://forums.raspberrypi.com/viewtopic.php?t=333160)
* [Getting started on basic Pico8 Gamedev](https://lukemerrett.com/getting-started-with-pico-8/)
* [GameTiger, another Pico-based console](https://github.com/codetiger/GameTiger-Console)
* [Espressif docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/linux-macos-setup.html)

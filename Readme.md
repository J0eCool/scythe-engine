`python build.py watch` - builds the game.dll and watches for any changes

`python build.py` - builds the kernel that loads game.dll and launches it
# How to Read this Repository

Kernel Entry Point: [scythe.cpp](src/scythe.cpp)

DLL primary class: [Program](src/program.cpp)

`main` -> `Program::` -> onLoad/update/render

> This is ~~a story about a girl~~ a game engine for personal use. Cryptic hints are the best this documentation is liable to offer.

## Why

It's a C++ thing because I like it. Native-first, port to wasm later.

It's a hobby. When bored, render cubes.

## Features

* [Hot Reload](doc/hot-reload.md) - a must have
* It's got a cool name
* Build Automation
	- watch files for changes and automatically rebuild as needed
	- no makefiles, we scan for included headers and corresponding object files
	- transitive dependencies respected

## onInit/onLoad

Why two initializers?

- `onInit` gets called before `onLoad`
- init you can't access neighbors/parent
- load you can access neighbors and they're all init-ed (but not loaded)
- oninit your parent is not inited
- onload your parent is loaded
- all loaded things have been inited

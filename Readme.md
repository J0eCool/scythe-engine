# How to Read this Repository

`./build-scythe.sh`

Entry Point: [](src/scythe.cpp)

DLL primary class: [Program](src/program.cpp)

`main` -> `Program::` -> onLoad/update/render

> This is ~~a story about a girl~~ a game engine for personal use.

## Why

It's a C++ thing because I like it. Native-first, will port to wasm later.

## Features

* [Hot Reload](doc/hot-reload.md) - a must have
* It's got a cool name

## onInit/onLoad

Why two initializers?

- `onInit` gets called before `onLoad`
- init you can't access neighbors/parent
- load you can access neighbors and they're all init-ed (but not loaded)
- oninit your parent is not inited
- onload your parent is loaded
- all loaded things have been inited

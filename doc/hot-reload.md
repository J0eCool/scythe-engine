Hot reloading is the best feature in game development.

## Design

There's a kernel for the engine that handles startup, asset loading, interfacing with the OS, generally stuff that's pretty stable. This is the entry point for the program, and is responsible for orchestrating userland modules.

Game-specific logic is put into dynamically loaded libraries.

There's a data API for persisting state between code updates. Ideally it can handle data migrations, so if fields are changed things still work. This could also be used for handling save data, since we're serializing the data anyway.

## Implementation

### Windows

```cpp
HMODULE *lib = LoadLibrary("lib.dll");
auto proc = GetProcAddress(lib, "procName");
// later
FreeLibrary(lib);
```

To reload the library, we'll probably need to first unload it, given how windows handles its processes in use. This increases the importance of having the engine able to build user code.

Currently we're kludging around this by copying the dlls to load

### Linux

`dlopen` to load the .so files, `dlsym` to look up symbols

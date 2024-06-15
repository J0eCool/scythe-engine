Hot reloading is the best feature in game development.

## Design

There's a kernel for the engine that handles startup, asset loading, interfacing with the OS, generally stuff that's pretty stable. This is the entry point for the program, and is responsible for orchestrating userland modules.

Game-specific logic is put into dynamically loaded libraries.

There's a data API for persisting state between code updates. Ideally it can handle data migrations, so if fields are changed things still work. This could also be used for handling save data, since we're serializing the data anyway.

## Implementation

There's a Dylib class responsible for handling the details of loading/reloading, as well as being the struct that holds the function pointers for the dynamically loaded code

### Windows

```cpp
#include <windows.h>
//...
HMODULE *lib = LoadLibrary("lib.dll");
auto proc = GetProcAddress(lib, "procName");
// later
FreeLibrary(lib);
```

To reload the library, we first need to unload it, given how windows handles its processes in use.

Currently we're copying the dlls to load. An alternate approach is to unload the library, rebuild it, then reload it, but that would necessitate pausing the game while the library builds. (currently it pauses anyway because we're waiting for the build to complete in the main thread, but that's fixable)

### Linux

`dlopen` to load the .so files, `dlsym` to look up symbols

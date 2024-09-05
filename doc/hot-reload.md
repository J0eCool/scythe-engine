Hot reloading is the best feature in game development.

## Implementation

There's a [Dylib](../src/dylib.h) class responsible for handling the details of loading/reloading, as well as being the struct that holds the function pointers for the dynamically loaded code

```cpp
#include <SDL2/SDL.h>
//...
void *lib = SDL_LoadObject("lib.dll");
auto proc = SDL_LoadFunction(lib, "procName");
// later
SDL_UnloadObject(lib);
```

### Windows

To reload the library, we first need to unload it, given how windows handles its processes in use.

Currently we're copying the dlls to load. An alternate approach is to unload the library, rebuild it, then reload it,
but that would necessitate pausing the game while the library builds. (currently it pauses anyway because we're waiting
for the build to complete in the main thread, but that's fixable)

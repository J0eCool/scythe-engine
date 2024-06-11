Long-term we want to have the rendering engine be effectively a separate isolated process.
So it makes sense to decouple it as much as possible.

To that end, we're passing a handle to a Renderer struct, which means dylibs don't need to link against any rendering backends (SDL, OpenGL, Vulkan, etc).
In general we want dynamically loaded code to have no external dependencies.

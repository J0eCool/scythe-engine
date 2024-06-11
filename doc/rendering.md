Long-term we want to have the rendering engine be effectively a separate isolated process.
So it makes sense to decouple it as much as possible.

To that end, we're passing a handle to a Renderer struct, which means dylibs don't need to link against any rendering backends (SDL, OpenGL, Vulkan, etc).
In general we want dynamically loaded code to have no external dependencies.

## C++ ABI dependency - vtable lookup

For convenience, we're using an abstract base class, Renderer, and implementing derived subclasses, e.g. Renderer_SDL is the SDL backend. This means that in the .dll we're calling virtual functions set up by the main application. Vtables are typically allocated program-wide, so this is fine memory-wise, but the specific mechanism by which it is called will depend on the C++ ABI, meaning we need to compile our .dlls with the same compiler we use to build the kernel. This is fine for in-house development.

For a walkthrough of how LLVM handles this, see here: https://stackoverflow.com/a/8006216

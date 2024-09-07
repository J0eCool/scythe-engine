#!/usr/bin/sh

# paths... hardcoded for now
SDL2=../../SDL2-2.0.14/i686-w64-mingw32
INCLUDE="-I${SDL2}/include"
LIB="-L${SDL2}/lib"
# FLAGS="-Wl,-subsystem,windows" # gets rid of the console window
LINK="-lmingw32 -lSDL2main -lSDL2 -lSDL2_image"
DBG_FLAGS="-fdiagnostics-color=always -g"

# cleanup
rm -f out/scythe
mkdir -p out

# build
SDLBIN="${SDL2}/bin"
cp $SDLBIN/SDL2.dll out/
cp $SDLBIN/SDL2_image.dll out/
cp $SDLBIN/libpng16-16.dll out/
cp $SDLBIN/zlib1.dll out/

g++ -c -o out/common.o src/common.cpp ${INCLUDE} ${LIB} ${FLAGS} ${LINK} ${DBG_FLAGS}
g++ -o out/scythe out/common.o src/scythe.cpp ${INCLUDE} ${LIB} ${FLAGS} ${LINK} ${DBG_FLAGS}

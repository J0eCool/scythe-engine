#!/usr/bin/sh

# paths... hardcoded for now
# and copy-pasted from build-scythe.sh... this is fine
SDL2=../../SDL2-2.0.14/i686-w64-mingw32
INCLUDE="-I${SDL2}/include"
LIB="-L${SDL2}/lib"
# FLAGS="-Wl,-subsystem,windows" # gets rid of the console window
LINK="-lmingw32 -lSDL2main -lSDL2 -lSDL2_image"
DBG_FLAGS="-fdiagnostics-color=always -g"

# cleanup
rm -f out/test
mkdir -p out

# build
SDLBIN="${SDL2}/bin"
cp -f $SDLBIN/SDL2.dll out/
cp -f $SDLBIN/SDL2_image.dll out/
cp -f $SDLBIN/libpng16-16.dll out/
cp -f $SDLBIN/zlib1.dll out/

g++ -o out/test src/test.cpp ${INCLUDE} ${LIB} ${FLAGS} ${LINK} ${DBG_FLAGS}

pushd out
./test

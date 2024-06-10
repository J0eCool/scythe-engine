#!/usr/bin/sh

# paths... hardcoded for now
SDL2=../../SDL2-2.0.14/i686-w64-mingw32
INCLUDE="-I${SDL2}/include"
LIB="-L${SDL2}/lib"
# FLAGS="-Wl,-subsystem,windows" # gets rid of the console window
LINK="-lmingw32 -lSDL2main -lSDL2"

# TODO: automatically copy relevant SDL .dlls to out dir

# cleanup
rm -f out/scythe
mkdir -p out

# build
g++ -c -o out/render.o src/render.cpp
ar rcs -o out/common.a out/render.o

g++ -o out/scythe src/scythe.cpp out/common.a ${INCLUDE} ${LIB} ${FLAGS} ${LINK}
./build-game.sh

# run
pushd out
./scythe

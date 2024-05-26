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
g++ src/scythe.cpp -o out/scythe ${INCLUDE} ${LIB} ${FLAGS} ${LINK}
g++ src/game.cpp -o out/game.dll -s -shared

# run
pushd out
./scythe

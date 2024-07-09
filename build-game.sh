#!/usr/bin/sh

g++ -o out/game.dll src/game.cpp -s -shared -lmingw32 -lSDL2 -lSDL2_image

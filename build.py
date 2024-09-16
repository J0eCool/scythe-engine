import os
import sys

class BuildTarget(object):
    def __init__(self, cmd, deps):
        self.cmd = cmd
        self.deps = deps

    def needs_build(self):
        return False

SDL2_PATH = '../../SDL2-2.0.14/i686-w64-mingw32'
INCLUDE='-I{}/include'.format(SDL2_PATH)
LIB='-L{}/lib'.format(SDL2_PATH)
FLAGS=''
LINK='-lmingw32 -lSDL2main -lSDL2 -lSDL2_image'
DBG_FLAGS='-fdiagnostics-color=always -g'

def cleanup():
    # TODO
    pass

def copy_dlls():
    # TODO
    pass

def build_scythe():
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    print('building common.o...')
    sys.stdout.flush()
    if os.system('g++ -c -o out/common.o src/common.cpp ' + flags) != 0:
        print('  error building common.o')
        return False
    print('building scythe...')
    sys.stdout.flush()
    if os.system('g++ -o out/scythe out/common.o src/scythe.cpp ' + flags) != 0:
        print('  error building scythe')
        return False
    return True

def main(args: list[str]):
    if not build_scythe():
        return 1

    # run
    os.chdir('out')
    os.system('scythe')
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

import os
import shutil
import sys
import time

class BuildTarget(object):
    def __init__(self, cmd, deps):
        self.cmd = cmd
        self.deps = deps

    def needs_build(self):
        return False

SDL2_PATH = os.path.join('..', '..', 'SDL2-2.0.14', 'i686-w64-mingw32')
INCLUDE='-I{}/include'.format(SDL2_PATH)
LIB='-L{}/lib'.format(SDL2_PATH)
FLAGS=''
LINK='-lmingw32 -lSDL2main -lSDL2 -lSDL2_image'
DBG_FLAGS='-fdiagnostics-color=always -g'

LOCKFILE='out/game.dll.lock'

def create_outdir():
    os.makedirs('out', exist_ok=True)

def create_lock():
    with open(LOCKFILE, 'w') as file:
        pass

def release_lock():
    try:
        os.remove(LOCKFILE)
    except Exception as e:
        print("exception releasing lock: ", e)

def copy_dlls():
    """Copies third-party SDL .dll files from SDL2_PATH"""
    sdlbin = os.path.join(SDL2_PATH, 'bin')
    dlls = [
        'SDL2.dll',
        'SDL2_image.dll',
        'libpng16-16.dll',
        'zlib1.dll',
    ]
    for dll in dlls:
        shutil.copy(os.path.join(sdlbin, dll), 'out')

def run_cmd(cmd):
    start = time.time()
    # os.system outputs go to the same terminal, so flush any pending prints first
    sys.stdout.flush()

    # critical section
    create_lock()
    code = os.system(cmd)
    release_lock()

    # logging
    dt = time.time()-start
    print('cmd finished in {:.2f}s : {}'.format(dt, cmd))
    if code != 0:
        print('[Error] exited with code={}'.format(code))
    return code == 0

def build_obj(objname):
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    print('building {0}.o...'.format(objname))
    return run_cmd('g++ -c -o out/{0}.o src/{0}.cpp {1}'.format(objname, flags))

def link_game():
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    flags += ' -s -shared'
    objfiles = ' '.join(['out/{}.o'.format(x) for x in [
        "common",
        "eyeGenScene",
        "particleScene",
        "program",
    ]])
    print('linking game.dll...')
    return run_cmd('g++ -o out/game.dll {0} {1}'.format(objfiles, flags))

def build_scythe():
    if not build_obj('common'):
        return False
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    print('building scythe...')
    sys.stdout.flush()
    if os.system('g++ -o out/scythe out/common.o src/scythe.cpp ' + flags) != 0:
        print('  error building scythe')
        return False
    return True

def build_and_run():
    create_outdir()
    copy_dlls()
    if not build_scythe():
        return 1

    # run
    os.chdir('out')
    os.system('scythe')
    return 0

def watch_and_build():
    while True:
        if os.path.getmtime('src/particleScene.cpp') > os.path.getmtime('out/particleScene.o'):
            if not build_obj('particleScene'):
                continue
            if not link_game():
                continue
        time.sleep(0.1)
    return 0

def run_program(prog):
    start = time.time()
    ret = prog()
    dt = time.time() - start
    print("Total time elapsed: {:.2f}s".format(dt))
    return ret

def main(args: list[str]):
    # clear lockfile, in case the builder crashes or w/e
    release_lock()
    if len(args) < 1:
        return run_program(build_and_run)
    cmd = args[0]
    if cmd == 'watch':
        return run_program(watch_and_build)
    print('[Error] unknown command:', cmd)
    return 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

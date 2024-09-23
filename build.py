import os
import pprint
import shutil
import sys
import time

SDL2_PATH = os.path.join('..', '..', 'SDL2-2.0.14', 'i686-w64-mingw32')
INCLUDE='-I{}/include'.format(SDL2_PATH)
LIB='-L{}/lib'.format(SDL2_PATH)
FLAGS=''
LINK='-lmingw32 -lSDL2main -lSDL2 -lSDL2_image'
DBG_FLAGS='-fdiagnostics-color=always -g'

LOCKFILE='out/game.dll.lock'

def Program(prog):
    def f():
        start = time.time()
        ret = prog()
        dt = time.time() - start
        print("Total time elapsed: {:.2f}s".format(dt))
        return ret
    return f

def create_outdir():
    os.makedirs('out', exist_ok=True)

def create_lock():
    with open(LOCKFILE, 'w') as file:
        pass

def release_lock():
    try:
        os.remove(LOCKFILE)
    except Exception as e:
        pass

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

@Program
def build_and_run():
    create_outdir()
    copy_dlls()
    if not build_scythe():
        return 1

    # run
    os.chdir('out')
    os.system('scythe')
    return 0

def cpp_to_obj(cpp: str) -> str:
    """the name of the .o file for a given .cpp file"""
    assert cpp.startswith('src')
    assert cpp.endswith('.cpp')
    return 'out'+cpp[3:-3]+'o'

@Program
def watch_and_build():
    build = BuildTree('program')
    modified = {}
    for file in build.files:
        modified[file] = os.path.getmtime(file)
    while True:
        # update file-changed times
        # if files changed, re-scan dependencies (which can find new files)
        # for all known files, rebuild only if stale
        if os.path.getmtime('src/particleScene.cpp') > os.path.getmtime('out/particleScene.o'):
            if not build_obj('particleScene'):
                continue
            if not link_game():
                continue
        time.sleep(0.1)
    return 0

class BuildTree(object):
    def __init__(self, root_name):
        self.dependencies = self.scan_dependencies(root_name)
        self.files = [f for f in self.dependencies.keys()]
        self.objs = self.init_objs()

    def scan_dependencies(self, component_name: str) -> dict[str, list[str]]:
        obj = os.path.join('out', component_name+'.o')
        src = os.path.join('src', component_name+'.cpp')
        to_scan = [src]
        dependencies = {}
        for filename in to_scan:
            deps = []
            dependencies[filename] = deps
            with open(filename) as f:
                if filename.endswith('.h') and os.path.exists(filename[:-1]+'cpp'):
                    to_scan.append(filename[:-1]+'cpp')
                for line in f.readlines():
                    if line.startswith('#include "') and line.endswith('"\n'):
                        # TODO: need to actually locate files in question
                        # vs assuming they're all in src/
                        inc = os.path.join('src', line[10:-2])
                        deps.append(inc)
                        if inc not in dependencies:
                            to_scan.append(inc)
        return dependencies
    
    def init_objs(self) -> list[tuple[str, set[str]]]:
        """Gets all the object files to build, along with their (recursive)
        dependencies"""
        objs = []
        for src in self.dependencies.keys():
            if src.endswith('.cpp'):
                objs.append((src, self.flatten_dependencies(src)))
        return objs
    
    def flatten_dependencies(self, src: str) -> set[str]:
        """Gets the set of recursive dependencies for a given source file"""
        assert src in self.dependencies, "No file found for #include: {}".format(src)
        deps = set(self.dependencies[src])
        for dep in list(deps):
            for rec in self.flatten_dependencies(dep):
                deps.add(rec)
        return deps

@Program
def walk_build_tree():
    # given program.o, can we find all dependencies?
    build = BuildTree('program')
    print('files:',)
    pprint.pprint(build.files)
    print('')
    print('dependencies:',)
    pprint.pprint(build.dependencies)
    print('')
    print('objs:')
    pprint.pprint(build.objs)
    pprint.pprint([cpp_to_obj(cpp) for cpp, _ in build.objs])

def main(args: list[str]):
    # clear lockfile, in case the builder crashes or w/e
    release_lock()
    if len(args) < 1:
        return build_and_run()
    cmd = args[0]
    if cmd == 'watch':
        return watch_and_build()
    if cmd == 'walk':
        return walk_build_tree()
    print('[Error] unknown command:', cmd)
    return 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

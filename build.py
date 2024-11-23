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

# console colors enum
class Color(object):
    DEFAULT = '\033[0m'

    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    PURPLE = '\033[95m'
    CYAN = '\033[96m'

    BOLD = '\033[1m'
    FAINT = '\033[2m'
    ITALIC = '\033[3m'
    UNDERLINE = '\033[4m'

Color.INFO = Color.BLUE
Color.OK = Color.GREEN
Color.WARN = Color.YELLOW
Color.ERR = Color.RED

LOG_STATE = '' #ideally this would be an object but this'll do for now
def log(color, *args, header=None):
    header = header or LOG_STATE
    print(color, " [", header, "] ", Color.DEFAULT, sep='', end='')
    print(*args)
def logh(header, *args):
    log(*args, header=header)

COMMANDS = []
# Program is a meta-decorator
# @Program('foo') def bar():
#  ->
# bar = decorator(bar)
def Program(name):
    """Top-level program to run from cmdline arg"""

    def decorator(program_fn):
        global COMMANDS
        def f(*args):
            # common setup
            global LOG_STATE
            LOG_STATE = name
            start = time.time()

            # run program
            ret = program_fn(*args)

            # exit logging
            dt = time.time() - start
            log(Color.INFO, "Total time elapsed: {:.2f}s".format(dt))
            return ret

        COMMANDS.append((name, f))
        return f
    return decorator

def ensure_outdir():
    os.makedirs('./out', exist_ok=True)

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
    elapsed = time.time()-start

    color = Color.OK if code == 0 else Color.ERR
    time_color = Color.OK
    if elapsed > 2.0:
        time_color = Color.WARN
    if elapsed > 5.0:
        time_color = Color.ERR
    log(color, 'cmd finished in {}{:.2f}{}s : {}'.format(
        time_color, elapsed, Color.DEFAULT, cmd
    ))
    if code != 0:
        logh('Error', Color.ERR, 'exited with code={}'.format(code))
    return code == 0

def build_obj(cpp, obj):
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    log(Color.INFO, 'building {}...'.format(obj))
    return run_cmd('g++ -c -o {} {} {}'.format(obj, cpp, flags))

def build_scythe():
    if not build_obj('src/common.cpp', 'out/common.o'):
        return False
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    log(Color.INFO, 'building scythe...')
    sys.stdout.flush()
    if os.system('g++ -o out/scythe out/common.o src/scythe.cpp ' + flags) != 0:
        log(Color.ERR, '  error building scythe')
        return False
    log(Color.OK, '  build successful')
    return True

@Program('run')
def build_and_run():
    ensure_outdir()
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

@Program('watch')
def watch_and_build(*args):
    ensure_outdir()

    force_build = False
    for arg in args:
        if arg == '--force-build':
            force_build = True
        else:
            assert False, "unknown argument: "+arg
    build = BuildTree('program')
    modified = {}
    for file in build.files:
        modified[file] = os.path.getmtime(file)
    for cpp, _ in build.objs:
        # initialize .cpp modified times to .o file times, in case we made changes
        # before starting the watch script
        obj = cpp_to_obj(cpp)
        if os.path.exists(obj):
            modified[cpp] = os.path.getmtime(obj)
        else:
            modified[cpp] = 0

    while True:
        # for all known files, rebuild only if stale
        changed = set()
        for file in build.files:
            mod = os.path.getmtime(file)
            if mod > modified[file]:
                modified[file] = mod
                changed.add(file)

        # if files changed, re-scan dependencies (which can find new files)
        if changed or force_build:
            print('')
            log(Color.INFO, 'files changed:', len(changed))
            build = BuildTree('program')
            # needing to update modified times is kinda gross but aight
            for f in build.files:
                if f not in modified:
                    modified[f] = os.path.getmtime(f)

            # build any changes
            did_build = False
            t_start = time.time()
            for cpp, deps in build.objs:
                obj = cpp_to_obj(cpp)
                code_change = cpp in changed or any(dep in changed for dep in deps)
                obj_missing = not os.path.exists(obj)
                if force_build or code_change or obj_missing:
                    if build_obj(cpp, obj):
                        did_build = True
            force_build = False

            # relink the .dll if any objs changed
            if did_build:
                link_game(build)
                elapsed = time.time() - t_start
                color = Color.OK
                if elapsed > 5.0:
                    color = Color.WARN
                if elapsed > 30.0:
                    color = Color.ERR
                log(Color.INFO, 'Total build time: {}{:.2f}{}s'.format(
                    color, elapsed, Color.DEFAULT,
                ))
    
        sys.stdout.flush()
        time.sleep(0.1)

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

def link_game(build: BuildTree):
    flags = ' '.join([INCLUDE, LIB, FLAGS, LINK, DBG_FLAGS])
    flags += ' -s -shared'
    objfiles = ' '.join(cpp_to_obj(cpp) for cpp, _ in build.objs)
    log(Color.INFO, 'linking game.dll...')
    return run_cmd('g++ -o out/game.dll {0} {1}'.format(objfiles, flags))

@Program('walk')
def walk_build_tree():
    """Program used to visualize build info"""
    # given program.o, can we find all dependencies?
    build = BuildTree('program')
    log(Color.INFO, 'files:')
    pprint.pprint(build.files)
    print('')
    log(Color.INFO, 'dependencies:')
    pprint.pprint(build.dependencies)
    print('')
    log(Color.INFO, 'objs:')
    pprint.pprint(build.objs)
    print([cpp_to_obj(cpp) for cpp, _ in build.objs])

@Program('test')
def run_tests():
    """Automatically run unit test program when source files change"""
    build = BuildTree('program')
    modified = {}
    for file in build.files:
        modified[file] = os.path.getmtime(file)
    for cpp, _ in build.objs:
        # initialize .cpp modified times to .o file times, in case we made changes
        # before starting the watch script
        obj = cpp_to_obj(cpp)
        if os.path.exists(obj):
            modified[cpp] = os.path.getmtime(obj)
        else:
            modified[cpp] = 0

    first_run = True
    while True:
        # for all known files, rebuild only if stale
        changed = set()
        for file in build.files:
            mod = os.path.getmtime(file)
            if mod > modified[file]:
                modified[file] = mod
                changed.add(file)

        if changed or first_run:
            print('')
            if not first_run:
                log(Color.INFO, 'files changed:', len(changed))
            first_run = False
            # if files changed, re-scan dependencies (which can find new files)
            build = BuildTree('program')
            # needing to update modified times is kinda gross but aight
            for f in build.files:
                if f not in modified:
                    modified[f] = os.path.getmtime(f)

            t_start = time.time()
            run_cmd('sh ./test.sh')

            elapsed = time.time() - t_start
            color = Color.OK
            if elapsed > 2.5:
                color = Color.WARN
            if elapsed > 10.0:
                color = Color.ERR
            log(Color.INFO, 'Total build time: {}{:.2f}{}s'.format(
                color, elapsed, Color.DEFAULT,
            ))
    
        sys.stdout.flush()
        time.sleep(0.1)

def main(args: list[str]):
    # clear lockfile, in case the builder crashes or w/e
    release_lock()
    if len(args) < 1:
        print('usage: python build.py [command]')
        print('valid commands:')
        for (name, f) in COMMANDS:
            print('  -', name)
        return 0
    cmd = args[0]
    for (name, f) in COMMANDS:
        if cmd == name:
            return f(*args[1:])
    logh('Error', Color.ERR, 'unknown command:', cmd)
    return 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

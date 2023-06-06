#! /usr/bin/env python3
'''
This script will do these things.
- List all dependencies in a dylib
- Filter dependencies starting with '/opt/homebrew/'
- Archive the directory of the required libraries.
'''

import argparse
import collections
import copy
import os
import re
import sys
import subprocess


link_dirs = (
    '/opt/homebrew/lib',
    '/opt/homebrew/include',
    '/opt/homebrew/bin',
    '/opt/homebrew/opt',
    '/opt/homebrew/sbin',
    '/opt/homebrew/share',
    '/opt/homebrew/var',
)

def list_dependencies(dylib_name, verbosity=0):
    p = subprocess.run(['otool', '-L', dylib_name], stdout=subprocess.PIPE)
    ret = []
    for line in p.stdout.decode('ascii').split('\n')[1:]:
        line = line.split()
        if line:
            dep = os.path.realpath(line[0])
            if verbosity > 0:
                print(f'{dylib_name} depends {dep}')
            ret.append(dep)
    return ret


def is_brew_dependency(dylib_name):
    return dylib_name.startswith('/opt/homebrew/')


def dylib_to_dirname(dylib_name):
    pp = dylib_name.rsplit(sep='/', maxsplit=2)
    if pp[1] != 'lib':
        raise ValueError(f'{dylib_name}: The 2nd base name must be "lib"')
    return pp[0]


def package_files(output_name, dirs):
    cmd = ['tar',
           '--uid', '0',
           '--gid', '0',
           '-czf', output_name,
    ]
    for d in dirs:
        cmd.append(d)
    subprocess.run(cmd)


def find_links(dir_name, cond_func, verbosity):
    ret = []
    for root, dirs, files in os.walk(dir_name):
        if verbosity > 1:
            print(f'root="{root}" dirs="{dirs}" files="{files}"')
        for f in files + dirs:
            fp = os.path.join(root, f)
            if not os.path.islink(fp):
                if verbosity > 1:
                    print(f'{fp} is not a link')
                continue
            target = os.path.realpath(fp)
            if verbosity > 1:
                print(f'Checking a link {fp} -> {target} satisfies the condition.')
            if cond_func(target):
                if verbosity > 0:
                    print(f'Packaging a link {fp}')
                ret.append(fp)
    return ret


def _main():
    parser = argparse.ArgumentParser(
            description='Archive homebrew dependencies of dylib')
    parser.add_argument('-o', '--output')
    parser.add_argument('-v', '--verbose', action='count', default=0)
    parser.add_argument('dylib_name')
    args = parser.parse_args()

    deps = set()
    queue = collections.deque()
    queue.append(args.dylib_name)
    while len(queue) > 0:
        f = queue.popleft()
        aa = list_dependencies(f, verbosity=args.verbose-1)
        for a in aa:
            if not is_brew_dependency(a):
                continue
            if a in deps:
                continue
            deps.add(a)
            queue.append(a)

    files = set()
    for a in deps:
        files.add(dylib_to_dirname(a))

    dirs = copy.copy(files)

    def _is_packaging(target):
        if args.verbose >= 3:
            print(f'_is_packaging: target="{target}"')
        for d in dirs:
            if target.startswith(d):
                return True
        return False

    for d in link_dirs:
        if args.verbose >= 3:
            print(f'Finding symlink in "{d}"')
        for l in find_links(d, _is_packaging, verbosity=args.verbose-1):
            files.add(l)

    if args.output:
        package_files(args.output, files)


if __name__ == '__main__':
    _main()

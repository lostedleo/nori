#!/usr/bin/env python
# coding=utf-8
import os
import subprocess
import sys
import getopt

BASE_CMD = 'python ' + os.path.dirname(sys.argv[0]) + '/cpplint.py'

def RunShellCommand(cmd, cwd='.'):
    """
    executes a command
    :param cmd: A string to execute.
    :param cwd: from which folder to run.
    """

    stdout_target = subprocess.PIPE
    stderr_target = subprocess.PIPE

    proc = subprocess.Popen(cmd,
                            shell=True,
                            cwd=cwd,
                            stdout=stdout_target,
                            stderr=stderr_target)
    out, err = proc.communicate()
    return (proc.returncode, out, err)

# find directorys by filetype
class FindByFileType:
    def exist_file(self, path, suffixs, files):
        items = os.listdir(path)
        for name in items:
            full_path = path + '/' + name
            if os.path.isdir(full_path):
                self.exist_file(full_path, suffixs, files)
            else:
                for suffix in suffixs:
                    if name.endswith('.' + suffix) \
                       and not name.endswith('.pb.h') \
                       and not name.endswith('.pb.cc'):
                        files.add(full_path)
                        break

    def list_files(self, path, suffixs):
        files = set()
        self.exist_file(path, suffixs, files)
        return files

def usage():
    print sys.argv[0] + ' -p path -t suffix'
    print sys.argv[0] + ' -h get help info'

def main():
    opts, args = getopt.getopt(sys.argv[1:], "hp:t:", ["help", "path=", "type="])
    path = '.'
    suffix = ''
    for op, value in opts:
        if op == '-p' or op == '--path':
            path = value
        elif op == '-t' or op == '--type':
            suffix = value
        elif op == '-h' or op == '--help':
            usage()
            sys.exit(0)

    suffixs = set()
    if suffix == "":
        suffixs = ['h', 'hpp', 'cpp', 'cc']
    else:
        suffixs = suffix.split(',')

    find_files = FindByFileType()
    files = find_files.list_files(path, suffixs)
    if len(files):
        for value in files:
            (status, out, err) = RunShellCommand(BASE_CMD + ' ' + value)
            print err

if __name__ == '__main__':
    main()


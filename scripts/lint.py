#!/usr/bin/env python3
from os import chdir, path
from subprocess import PIPE, run
from sys import exit, stderr
from xml.etree.ElementTree import XML

import argparse
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", help="Inplace edit files, if specified.", action="store_true")
    args = parser.parse_args()

    # Set the working directory to the parent directory of this script (project root)
    chdir(path.join(path.dirname(path.abspath(__file__)), ".."))

    res = exec(["git", "diff", "--diff-filter=ACMRTUX", "--name-only", "master"])
    res.check_returncode()
    files = res.stdout.strip().split("\n")

    res = exec(["git", "ls-files", "--others", "--exclude-standard"])
    res.check_returncode()
    files += res.stdout.strip().split("\n")

    if args.i:
        format_files(files)
    else:
        check_files(files)


def format_files(files):
    for filename in files:
        formatter = get_formatter(filename)
        if formatter:
            formatter(filename, modify=True)


def check_files(files):
    count = 0
    for filename in files:
        formatter = get_formatter(filename)
        if formatter:
            if not formatter(filename):
                print("[ERROR] {} is unformatted".format(filename))
                count += 1

    if count:
        print(
            "\n\033[31mLinting failed. {} file(s) need formatting. Run "
            "\033[1mmake vlog-fix\033[m\033[31m to attempt to "
            "automatically fix this\033[m".format(count)
        )
        exit(1)


def get_formatter(filename):
    ext = path.splitext(filename)[1]
    if ext in FORMATTERS:
        return FORMATTERS[ext]
    return None


def format_cpp(filename, modify=False):
    if modify:
        res = exec(["clang-format", "-i", filename])
        res.check_returncode()
        return True
    else:
        res = exec(["clang-format", "--output-replacements-xml", filename])
        res.check_returncode()
        xml = XML(res.stdout)
        replacements = xml.findall("replacement")
        return True if not replacements else False


def format_py(filename, modify=False):
    if modify:
        res = exec(["black", "--py36", "-q", filename])
        res.check_returncode()
        return True
    else:
        res = exec(["black", "--py36", "-q", "--check", filename])
        if res.returncode > 1:
            res.check_returncode()
        return True if res.returncode == 0 else False


def print_error(msg):
    if hasattr(stderr, "isatty") and stderr.isatty():
        print("{}".format(msg))
    else:
        print(msg)


def exec(cmd):
    if not isinstance(cmd, list):
        cmd = [cmd]
    return run(cmd, stdout=PIPE, stderr=PIPE, universal_newlines=True)


FORMATTERS = {
    ".c": format_cpp,
    ".cpp": format_cpp,
    ".cu": format_cpp,
    ".h": format_cpp,
    ".hpp": format_cpp,
    ".py": format_py,
}

if __name__ == "__main__":
    main()

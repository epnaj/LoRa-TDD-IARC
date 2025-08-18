#!/usr/bin/env python3

import os
import subprocess
import sys
import argparse
from typing import List, Tuple, Dict
from dataclasses import dataclass

@dataclass
class CompileFiles:
    cpp_files: List[str]
    hpp_dirs: List[str]
    test_files: List[str]

@dataclass
class ParsedInput:
    main_file: str
    output_file: str
    start: bool
    compile_tests: bool
    debug: bool

def execute(command: str) -> None:
    try:
        subprocess.check_call(command, shell=True)
    except subprocess.CalledProcessError as e:
        print("Execution failed!")
        sys.exit(e.returncode)

def gather_lib_sources_and_headers(lib_base_dir: str = './lib') -> CompileFiles:
    """
    Scan ./lib directory for all subdirectories and gather all .cpp source files and .hpp header files
    within each lib directory.
    """
    compile_files: CompileFiles = CompileFiles([], [], [])

    if not os.path.isdir(lib_base_dir):
        print(f"Warning: '{lib_base_dir}' directory does not exist.")
        return compile_files

    for root, dirs, files in os.walk(lib_base_dir):
        compile_files.cpp_files += list(
            map(
                lambda file: os.path.join(root, file),
                filter(
                    lambda file: file.endswith(".cpp"),
                    files
                )
            )
        )
        compile_files.hpp_dirs += [ root ] if any(
            file.endswith(".hpp") 
                for file in files
        ) else []

    compile_files.test_files = list(filter(
        lambda file: "test" in os.path.split(file)[1],
        compile_files.cpp_files
    ))

    compile_files.cpp_files = list(set(compile_files.cpp_files).difference(compile_files.test_files))

    return compile_files

def compile_project(main_cpp: str = 'main.cpp', output_exec: str = 'main', flags: List[str] = []) -> List[str]:
    # Check main.cpp existence
    if not os.path.isfile(main_cpp):
        print(f"Error: '{main_cpp}' not found in the current directory.")
        sys.exit(1)

    compile_files: CompileFiles = gather_lib_sources_and_headers()
    include_command: str        = "-I " + " -I ".join(compile_files.hpp_dirs)

    # Compose compile command
    compile_cmd = ['g++', '-std=c++17', '-o', output_exec, main_cpp] + compile_files.cpp_files + [ include_command ] + flags

    print("Compiling with command:")
    print(' '.join(compile_cmd))

    execute(' '.join(compile_cmd))

    print(f"Compilation successful!")

    return [ output_exec ]

def compile_tests(lib_dir: str = "./lib", flags: List[str] = []) -> List[str]:
    test_bins_directory: str = "./test_bins"

    if not os.path.exists(test_bins_directory):
        os.makedirs(test_bins_directory)

    compile_files: CompileFiles = gather_lib_sources_and_headers()

    test_binaries: List[str] = []

    for test_main in compile_files.test_files:
        test_binaries += compile_project(
            test_main,
            os.path.join(
                test_bins_directory, 
                # ./some-path/test_main.cpp -> test_main
                os.path.splitext(os.path.split(test_main)[1])[0]
            ),
            flags
        )

    return test_binaries

def parse_input() -> ParsedInput:
    parser: argparse.ArgumentParser = argparse.ArgumentParser()

    parser.add_argument(
        "main_file", nargs="?", default="main.cpp", help="Main C++ source file (default: main.cpp)"
    )
    parser.add_argument(
        "-o", "--output", default="main.out", help="Output binary name (default: main.out)"
    )
    parser.add_argument(
        "-g", "--debug", action="store_true", help="Build with debug flags (-g)"
    )
    parser.add_argument(
        "-t", "--tests", action="store_true", help="Compile tests instead of main file"
    )
    parser.add_argument(
        "-s", "--start", action="store_true", help="Run the compiled binary after building"
    )

    args = parser.parse_args()

    return ParsedInput(
        args.main_file,
        args.output,
        args.start,
        args.tests,
        args.debug
    )

if __name__ == "__main__":
    parsed_input: ParsedInput = parse_input()

    flags: List[str] = ["-lm", "-lpthread", "-O2"]
    
    if parsed_input.debug:
        flags.append("-g")

    binaries: List[str] = []

    if parsed_input.compile_tests:
        binaries += compile_tests("./lib", flags)
    else:
        binaries += compile_project(
            parsed_input.main_file,
            parsed_input.output_file,
            flags
        )

    if parsed_input.start:
        for binary in binaries:
            execute(f"./{binary}")

import json
import os
import subprocess
from pathlib import Path
from typing import List, Dict, Optional


def find_cpp_files(project_root: Path) -> List[Path]:
    excluded_dirs = {"bazel-bin", "bazel-out", "bazel-testlogs", "bazel-External-Sort", "external"}
    cpp_files = []
    
    for pattern in ["**/*.cpp", "**/*.cc"]:
        for file in project_root.glob(pattern):
            if not any(excluded in file.parts for excluded in excluded_dirs):
                cpp_files.append(file)
    
    return sorted(cpp_files)


def find_gtest_include() -> Optional[str]:
    try:
        result = subprocess.run(
            ["bazel", "info", "output_base"],
            capture_output=True,
            text=True,
            check=True
        )
        output_base = Path(result.stdout.strip())
        external_dir = output_base / "external"
        
        for gtest_pattern in ["googletest+", "googletest~*", "googletest"]:
            for gtest_dir in external_dir.glob(gtest_pattern):
                gtest_include = gtest_dir / "googletest" / "include"
                if gtest_include.exists():
                    return str(gtest_dir)
        
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    
    return None


def get_include_paths(file_path: Path, project_root: Path, gtest_path: Optional[str]) -> List[str]:
    includes = [f"-I{project_root}"]
    
    if "serialization" in file_path.parts:
        includes.append(f"-I{project_root}/serialization/include")
    
    if "_old_project" in file_path.parts:
        includes.append(f"-I{project_root}/_old_project/include")
    
    if gtest_path:
        includes.append(f"-I{gtest_path}/googletest/include")
        includes.append(f"-I{gtest_path}/googlemock/include")
    
    return includes


def create_compile_command(file_path: Path, project_root: Path, gtest_path: Optional[str]) -> Dict:
    includes = get_include_paths(file_path, project_root, gtest_path)
    flags = ["-std=c++20", "-Wall", "-Wextra", "-pthread"]
    
    arguments = ["clang++", "-c"] + flags + includes + [str(file_path.resolve())]
    
    return {
        "directory": str(project_root.resolve()),
        "file": str(file_path.resolve()),
        "arguments": arguments
    }


def generate_compile_commands(output_file: str = "compile_commands.json") -> None:
    project_root = Path.cwd()
    cpp_files = find_cpp_files(project_root)
    gtest_path = find_gtest_include()
    
    compile_commands = [
        create_compile_command(file, project_root, gtest_path)
        for file in cpp_files
    ]
    
    with open(output_file, "w") as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f"Generated {output_file} with {len(cpp_files)} source files")
    print("Using compiler: clang++")
    print("C++ standard: c++20")
    
    if gtest_path:
        print("Found Google Test includes")


if __name__ == "__main__":
    generate_compile_commands()

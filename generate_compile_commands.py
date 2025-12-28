import json
import subprocess
from pathlib import Path
from typing import List, Dict, Optional
from dataclasses import dataclass


@dataclass
class ProjectConfig:    
    compiler: str = "clang++"
    cpp_standard: str = "c++20"
    common_flags: List[str] = None
    
    excluded_dirs: List[str] = None
    
    include_mappings: Dict[str, List[str]] = None
    
    def __post_init__(self):
        if self.common_flags is None:
            self.common_flags = ["-Wall", "-Wextra", "-pthread"]
        
        if self.excluded_dirs is None:
            self.excluded_dirs = [
                "bazel-bin",
                "bazel-out", 
                "bazel-testlogs",
                "bazel-External-Sort",
                "external",
                "_old_project"
            ]
        
        if self.include_mappings is None:
            self.include_mappings = {
                "serialization": ["serialization/include", "logging/include"],
                "io": ["io/include", "logging/include", "serialization/include"],
                "logging": ["logging/include"],
                "common": ["common/include"],
                "external_sort": ["external_sort/include", "logging/include"],
            }


def find_cpp_files(project_root: Path, excluded_dirs: List[str]) -> List[Path]:
    cpp_files = []
    
    for pattern in ["**/*.cpp", "**/*.cc"]:
        for file in project_root.glob(pattern):
            if not any(excluded in file.parts for excluded in excluded_dirs):
                cpp_files.append(file)
    
    return sorted(cpp_files)


def find_bazel_external_lib(lib_patterns: List[str], subpath: str = "") -> Optional[str]:
    try:
        result = subprocess.run(
            ["bazel", "info", "output_base"],
            capture_output=True,
            text=True,
            check=True
        )
        output_base = Path(result.stdout.strip())
        external_dir = output_base / "external"
        
        for pattern in lib_patterns:
            for lib_dir in external_dir.glob(pattern):
                check_path = lib_dir / subpath if subpath else lib_dir
                if check_path.exists():
                    return str(lib_dir)
        
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    
    return None


def find_gtest_include() -> Optional[str]:
    return find_bazel_external_lib(
        ["googletest+", "googletest~*", "googletest"],
        "googletest/include"
    )


def find_spdlog_include() -> Optional[str]:
    return find_bazel_external_lib(
        ["spdlog+", "spdlog~*", "spdlog"],
        "include/spdlog"
    )


def get_include_paths(
    file_path: Path, 
    project_root: Path, 
    gtest_path: Optional[str],
    spdlog_path: Optional[str],
    include_mappings: Dict[str, List[str]]
) -> List[str]:
    includes = [f"-I{project_root}"]
    
    for dir_marker, include_dirs in include_mappings.items():
        if dir_marker in file_path.parts:
            for include_dir in include_dirs:
                includes.append(f"-I{project_root / include_dir}")
    
    if gtest_path:
        includes.append(f"-I{gtest_path}/googletest/include")
        includes.append(f"-I{gtest_path}/googlemock/include")
    
    if spdlog_path:
        includes.append(f"-I{spdlog_path}/include")
    
    return includes


def create_compile_command(
    file_path: Path, 
    project_root: Path, 
    gtest_path: Optional[str],
    spdlog_path: Optional[str],
    config: ProjectConfig
) -> Dict:
    includes = get_include_paths(file_path, project_root, gtest_path, spdlog_path, config.include_mappings)
    flags = [f"-std={config.cpp_standard}"] + config.common_flags
    
    arguments = [config.compiler, "-c"] + flags + includes + [str(file_path.resolve())]
    
    return {
        "directory": str(project_root.resolve()),
        "file": str(file_path.resolve()),
        "arguments": arguments
    }


def generate_compile_commands(
    output_file: str = "compile_commands.json",
    config: Optional[ProjectConfig] = None
) -> None:
    if config is None:
        config = ProjectConfig()
    
    project_root = Path.cwd()
    cpp_files = find_cpp_files(project_root, config.excluded_dirs)
    gtest_path = find_gtest_include()
    spdlog_path = find_spdlog_include()
    
    compile_commands = [
        create_compile_command(file, project_root, gtest_path, spdlog_path, config)
        for file in cpp_files
    ]
    
    with open(output_file, "w") as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f"Generated {output_file} with {len(cpp_files)} source files")
    print(f"Using compiler: {config.compiler}")
    print(f"C++ standard: {config.cpp_standard}")
    print(f"Common flags: {' '.join(config.common_flags)}")
    
    if gtest_path:
        print("Found Google Test includes")
    
    if spdlog_path:
        print("Found spdlog includes")
    
    print(f"\nConfigured include mappings:")
    for dir_marker, includes in config.include_mappings.items():
        print(f"  - {dir_marker}: {', '.join(includes)}")


if __name__ == "__main__":
    generate_compile_commands()

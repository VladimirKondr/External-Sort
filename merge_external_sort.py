#!/usr/bin/env python3
"""
Скрипт для объединения всех файлов C++ проекта в один файл,
автоматически обрабатывая локальные #include и связанные .cpp файлы.
Системные #include собираются и помещаются в начало итогового файла.
"""

import os
import re
from pathlib import Path

def find_cpp_for_header(header_path_str, base_dir_str):
    """
    Ищет соответствующий .cpp файл в директории src для данного .hpp/.tpp файла.
    Пример: для include/my_class.hpp ищет src/my_class.cpp.
    """
    header_path = Path(header_path_str)
    base_dir = Path(base_dir_str)
    
    if not (header_path.suffix in ['.hpp', '.tpp', '.h'] and \
            ( (base_dir / "include") in header_path.parents or \
              (base_dir / "src") in header_path.parents) ):
        return None

    stem_name = header_path.stem
    
    cpp_path = base_dir / "src" / f"{stem_name}.cpp"
    
    if cpp_path.exists():
        return str(cpp_path.resolve())
    return None

def process_content_recursive(content, processed_files, all_system_includes, base_dir, current_file_path_str):
    """
    Рекурсивно обрабатывает содержимое файла:
    - Заменяет локальные #include на содержимое файлов.
    - Собирает все системные #include.
    - Ищет и включает соответствующий .cpp файл из src для каждого .hpp.
    """
    lines = content.split('\n')
    result_lines = []
    
    for line in lines:
        local_include_match = re.match(r'^\s*#include\s+"([^"]+)"', line)
        system_include_match = re.match(r'^\s*#include\s+<([^>]+)>', line)

        if local_include_match:
            include_file_relative = local_include_match.group(1)
            include_path_abs = None
            
            current_dir = os.path.dirname(current_file_path_str)
            
            resolved_from_current = os.path.abspath(os.path.join(current_dir, include_file_relative))
            if os.path.exists(resolved_from_current):
                 include_path_abs = resolved_from_current
            else:
                search_dirs = [
                    os.path.join(base_dir, "include"),
                    os.path.join(base_dir, "include", "impl"),
                    os.path.join(base_dir, "src")
                ]
                for search_dir_root in search_dirs:
                    path_candidate = os.path.abspath(os.path.join(search_dir_root, include_file_relative))
                    if os.path.exists(path_candidate):
                        include_path_abs = path_candidate
                        break
            
            if include_path_abs and include_path_abs not in processed_files:
                processed_files.add(include_path_abs)
                relative_path_for_log = os.path.relpath(include_path_abs, base_dir)
                print(f"Processing Header: {relative_path_for_log}")
                
                with open(include_path_abs, 'r', encoding='utf-8') as f_header:
                    include_content = f_header.read()
                
                processed_block = process_content_recursive(
                    include_content, processed_files, all_system_includes, base_dir, include_path_abs
                )
                
                result_lines.append(f"// ========== From Header {relative_path_for_log} ==========")
                result_lines.extend(processed_block)
                result_lines.append(f"// ========== End of Header {relative_path_for_log} ==========")
                result_lines.append("")

                cpp_file_path_abs = find_cpp_for_header(include_path_abs, base_dir)
                if cpp_file_path_abs and cpp_file_path_abs not in processed_files:
                    processed_files.add(cpp_file_path_abs)
                    cpp_relative_path_for_log = os.path.relpath(cpp_file_path_abs, base_dir)
                    print(f"Processing Source for Header: {cpp_relative_path_for_log}")
                    with open(cpp_file_path_abs, 'r', encoding='utf-8') as f_cpp:
                        cpp_content = f_cpp.read()
                    
                    processed_cpp_block = process_content_recursive(
                        cpp_content, processed_files, all_system_includes, base_dir, cpp_file_path_abs
                    )
                    
                    result_lines.append(f"// ========== From Source {cpp_relative_path_for_log} ==========")
                    result_lines.extend(processed_cpp_block)
                    result_lines.append(f"// ========== End of Source {cpp_relative_path_for_log} ==========")
                    result_lines.append("")

        elif system_include_match:
            all_system_includes.add(line.strip())
        
        else:
            stripped_line = line.strip()
            if not stripped_line.startswith('#pragma once'):
                if stripped_line or (result_lines and not result_lines[-1].startswith("// ========== End of")):
                    result_lines.append(line)
            elif result_lines and result_lines[-1].strip() == "":
                 result_lines.pop()
    
    return result_lines


def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    processed_files = set() 
    all_system_includes = set()
    
    main_cpp_file_relative = os.path.join("src", "external_sort_task.cpp")
    main_cpp_file_abs = os.path.abspath(os.path.join(base_dir, main_cpp_file_relative))
    
    print(f"Base directory: {base_dir}")
    print(f"Processing main file: {main_cpp_file_relative}")
    
    if not os.path.exists(main_cpp_file_abs):
        print(f"Error: Main file {main_cpp_file_abs} not found!")
        return

    processed_files.add(main_cpp_file_abs)
    with open(main_cpp_file_abs, 'r', encoding='utf-8') as f:
        main_content = f.read()
    
    merged_content_lines = process_content_recursive(
        main_content, processed_files, all_system_includes, base_dir, main_cpp_file_abs
    )
    
    header_block = [
        "// Merged external sort implementation",
        "// Generated automatically",
        ""
    ]
    header_block.extend(sorted(list(all_system_includes)))
    header_block.append("") 
    
    final_content_lines = header_block + merged_content_lines
    final_content = '\n'.join(final_content_lines)
    
    output_file_path = os.path.join(base_dir, "external_sort_merged.cpp")
    with open(output_file_path, 'w', encoding='utf-8') as f:
        f.write(final_content)
    
    print(f"\nMerged file created: {output_file_path}")
    print(f"Processed {len(processed_files)} unique files (headers and sources).")
    
    print("\nProcessed files (relative to base directory):")
    sorted_relative_paths = sorted([os.path.relpath(p, base_dir) for p in processed_files])
    for rel_path in sorted_relative_paths:
        print(f"  {rel_path}")

if __name__ == "__main__":
    main()
# Run with: python3 generate_code_dump.py

import os

OUTPUT_FILE = "code_dump.txt"
TARGET_DIRS = ["src", "include"]
EXTRA_FILES = ["ui/ui_events.cpp", "ui/ui_events.h"]

def generate_tree(base_path, prefix=""):
    tree_lines = []
    entries = sorted(os.listdir(base_path))
    for idx, entry in enumerate(entries):
        full_path = os.path.join(base_path, entry)
        connector = "└── " if idx == len(entries) - 1 else "├── "
        if os.path.isdir(full_path):
            tree_lines.append(f"{prefix}{connector}{entry}/")
            extension = "    " if idx == len(entries) - 1 else "│   "
            tree_lines.extend(generate_tree(full_path, prefix + extension))
        else:
            tree_lines.append(f"{prefix}{connector}{entry}")
    return tree_lines

def read_file_content(file_path):
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            return f.read()
    except Exception as e:
        return f"// Error reading file {file_path}: {e}"

def collect_all_files():
    file_paths = []

    for target in TARGET_DIRS:
        for root, _, files in os.walk(target):
            for file in files:
                file_paths.append(os.path.join(root, file))

    for extra in EXTRA_FILES:
        if os.path.isfile(extra):
            file_paths.append(extra)
    return file_paths

def main():
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write("Tree\n\n")
        for target in TARGET_DIRS:
            out.write(f"{target}/\n")
            tree = generate_tree(target)
            out.write("\n".join(tree))
            out.write("\n\n")

        for extra in EXTRA_FILES:
            out.write(f"{extra}\n")
        out.write("\n\n")

        for path in collect_all_files():
            out.write(f"// {path}\n")
            out.write(read_file_content(path))
            out.write("\n\n")

if __name__ == "__main__":
    main()

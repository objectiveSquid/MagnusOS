import shutil
import json
import sys
import os


def main(list_file: str, input_directory: str, output_directory: str) -> None:
    try:
        with open(list_file, "r") as fp:
            files = json.load(fp)
    except Exception as e:
        print(f"Failed to open {list_file}: {e}")
        sys.exit(1)

    for filename, paths in files.items():
        for path in paths:
            try:
                os.makedirs(
                    os.path.dirname(f"{output_directory}/{path}"), exist_ok=True
                )
                shutil.copy(
                    f"{input_directory}/{filename}", f"{output_directory}/{path}"
                )
            except Exception as e:
                print(f"Failed to copy {filename} to {path}: {e}")
                sys.exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(
            "Usage: python3 copy-shared-files.py <list_file> <input_directory> <output_directory>"
        )
        sys.exit(1)

    main(sys.argv[1], sys.argv[2], sys.argv[3])

from typing import Callable, Any
from SCons.Node.FS import Dir, File, Entry
from SCons.Environment import Environment
import re
import os


def parse_size(size: str) -> int:
    size_match = re.match(r"([0-9\.]+)([kmg]?)", size, re.IGNORECASE)
    if size_match == None:
        raise ValueError(f"Invalid size: {size}")

    result = float(size_match.group(1))
    multiplier = size_match.group(2).lower()

    match multiplier:
        case "k":
            result *= 1024
        case "m":
            result *= 1024**2
        case "g":
            result *= 1024**3

    return int(result)


# simply tests if its a valid size
def parse_size_dry(size: str) -> str:
    try:
        parse_size(size)
    except Exception:
        raise

    return size


def glob_recursive(env: Environment, pattern: str, node: str = ".") -> list[File]:
    source_directory = str(env.Dir(node).srcnode())  # type: ignore

    directory_list = [source_directory]
    for root, directories, _ in os.walk(source_directory):
        for directory in directories:
            directory_list.append(os.path.join(root, directory))

    all_sources: list[File] = []
    for directory in directory_list:
        all_sources.extend(env.Glob(os.path.join(directory, pattern)))

    return all_sources


def find_index(array: list, predicate: Callable[[Any], bool]) -> int:
    for i in range(len(array)):
        if predicate(array[i]):
            return i

    raise ValueError(f"Element not found based on predicate, array: {array}")
    return None


def is_file_name(obj: str | File | Dir | Entry, name: str) -> bool:
    if isinstance(obj, str):
        return os.path.split(obj)[1] == name
    try:
        return os.path.split(obj.name)[1] == name
    except AttributeError:
        return False

from PIL import Image
import subprocess
import bitarray
import shutil
import sys
import os


def relative_directory(path: str) -> str:
    return os.path.join(os.path.dirname(__file__), path)


def png_to_bits(filepath: str) -> bytes:
    with Image.open(filepath, "r") as img:
        img = img.convert("RGB")
        pixels = list(img.getdata())

    output = bitarray.bitarray(endian="big")
    for pixel in pixels:
        if pixel == (255, 255, 255):
            output.append(1)
        else:
            output.append(0)

    return output.tobytes()


def convert_bits(bits: bytes, font_size: tuple[int, int]) -> bytes:
    output = bitarray.bitarray(endian="big")

    for char_y in range(16):
        for char_x in range(16):
            for dy in range(font_size[1]):
                for dx in range(font_size[0]):
                    bit_x = char_x * font_size[0] + dx
                    bit_y = char_y * font_size[1] + dy
                    bit_index = bit_y * 16 * font_size[0] + bit_x
                    byte_index = bit_index // 8
                    bit_offset = bit_index % 8

                    is_set = (bits[byte_index] >> (7 - bit_offset)) & 1
                    if is_set:
                        output.append(1)
                    else:
                        output.append(0)

    return output.tobytes()


def generate_font_files(output_directory: str) -> None:
    shutil.rmtree(output_directory, ignore_errors=True)
    os.makedirs(output_directory, exist_ok=True)

    for font_filename in os.listdir(relative_directory("rasterfonts")):
        if not font_filename.endswith(".png"):
            continue

        font_size: tuple[int, int] = [int(size) for size in font_filename.split("_")[0].split("x")]  # type: ignore
        png_bits = png_to_bits(relative_directory(f"rasterfonts/{font_filename}"))
        converted_bits = convert_bits(png_bits, font_size)

        with open(
            f"{output_directory}/{'x'.join(str(x) for x in font_size)}.f", "wb"
        ) as f:
            f.write(converted_bits)


def generate_header_file(fonts_directory: str, output_header_path: str) -> None:
    with open(output_header_path, "w") as f:
        f.write("#pragma once\n\n")

        f.write('#include "font.h"\n')
        f.write("#include <stdint.h>\n\n")

        f.write("static const FONT_FontInfo FONT_RasterFontSizes[] = {\n")
        for font_filename in os.listdir(fonts_directory):
            if not font_filename.endswith(".f"):
                continue

            clean_fontsize = [
                part.lstrip("0") for part in font_filename.split(".")[0].split("x")
            ]
            f.write(
                f"    {{\"{'x'.join(clean_fontsize)}.f\", {', '.join(clean_fontsize)}}},\n"
            )
        f.write("};\n")



FONT_FILE = "./rsrc/spleen-8x16.bdf"
OUT_FILE = "./src/kernel/include/drivers/font.h"

font_file_data = ""

with open(FONT_FILE, 'r') as f:
    font_file_data = f.readlines()


char_map = {}
index = 0
last_encoding = 0;

while font_file_data[index].split(" ")[0] != "STARTCHAR":
    index += 1


def create_global_bitmap(outfile):
    global char_map
    outfile.write(f"\n\nstatic char* BITMAP_GLOBAL[{len(char_map)}] = {'{'}\n")
    for key in char_map:
        outfile.write(f"bitmap_{key},\n")
    outfile.write("};\n\n")

def write_char_bitmap(character, outfile):
    global char_map
    outfile.write(f'\nstatic unsigned char bitmap_{character}[16] = {"{"}\n')
    for b in char_map[character]:
        outfile.write(f'{b}, ')
    outfile.write("\n};\n")


def create_character_enum(outfile):
    global char_map
    outfile.write("\n\nenum FONT_BITMAP_KEY {\n")
    for key in char_map:
        outfile.write(f"FONT_BITMAP_{key},\n")
    outfile.write("};\n\n")



def parse_index(outfile):
    global index
    global last_encoding
    if font_file_data[index].split(" ")[0] != "STARTCHAR":
        exit(-1) # should never error


    font_bytes = []
    bitmap_character = font_file_data[index].split(" ")[1:]
    bitmap_character = "_".join(bitmap_character).strip().replace("-", "_").replace("<", "_").replace(">", "_")
    print(bitmap_character)

    while font_file_data[index].split(" ")[0] != "ENCODING":
        index += 1
    last_encoding = int(font_file_data[index].split(" ")[-1].strip()) # extract encoding number   
    print(f'Encoding: {last_encoding}')
    while font_file_data[index] != "BITMAP\n":
        index += 1
    index += 1 # advance to actual bitmap entries
    
    
    while font_file_data[index] != "ENDCHAR\n":
        font_byte_entry = font_file_data[index][:-1]
        font_bytes.append(f"0x{font_byte_entry}")
        index += 1
    index += 1 # advance to next entry
    char_map[bitmap_character] = font_bytes
    

    

while last_encoding <= 159:
    parse_index(OUT_FILE)
    print(last_encoding)


with open(OUT_FILE, "w") as f:
    f.write(
        "/*"
        "All of these fonts are from https://github.com/fcambus/spleen/blob/master/spleen-8x16.bdf.\n"
        "I used metaprogramming to build this header file. See scripts/build_font_bitmap.py.\n"
        "*/"
    )
    f.write("#ifndef FONT_H\n#define FONT_H\n")
    create_character_enum(f)
    for char in char_map:
        write_char_bitmap(char, f)
    create_global_bitmap(f)
    f.write("#endif")


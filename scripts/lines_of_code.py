import os
import sys

# Author: Cole Strickler
# Usage: python3 lines_of_code.py [start directory]
# Info: Gets the total lines of code in the project. Currently counts lines from the following
#       extensions: ".c", ".h", ".asm", ".py", ".h"


args = sys.argv
argc =  len(sys.argv)
def get_filelines(file):
    lines = 0
    with open(file, 'r') as f:
        lines = len(f.readlines())
    return lines


if argc != 2:
    print("Usage: python3 lines_of_code.py [start directory]")
    exit(-1)

directory = args[1]
if not os.path.isdir(directory):
    print("Directory not found:", directory)
    exit(-1)


total_loc = 0

for root, dirs, files in os.walk(directory):
    for file_name in files:
        file_path = os.path.join(root, file_name)
        file_name, file_extension = os.path.splitext(file_path)
        # Call the function on the file
        if file_extension in [".c", ".h", ".asm", ".py", ".h"]:
            loc = get_filelines(file_path)
            total_loc += loc
            print(f"{file_name + file_extension} --> {loc}")
            

print(f"Total lines of code in project {directory}: {total_loc}")

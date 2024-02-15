import sys
import subprocess
args = sys.argv

bitmask_test_prog = "./scripts/bitmask_test.c"
chmod = ["chmod", "+x", "./scripts/bitmask_test"]


gcc_args = ["gcc", bitmask_test_prog, "-o", "./scripts/bitmask_test"]

if len(args) != 2:
    print("[USAGE]: test_bitmask.py 0xfffff")
    exit(0)


lines = []
with open(bitmask_test_prog, 'r') as f:
    lines = f.readlines()


for i in range(len(lines)):
    line = lines[i]
    
    if line[0:7] == "#define":
        x = line.split(' ')
        x[-1] = sys.argv[1] + "ULL"
        lines[i] = " ".join(x)
        lines[i] += "\n"


with open(bitmask_test_prog, 'w') as f:
    f.writelines(lines)

gcc = subprocess.Popen(gcc_args)
gcc.wait()
chmod = subprocess.Popen(chmod)
chmod.wait()
test = subprocess.Popen("./scripts/bitmask_test")

for i in test.communicate():
    print(i)
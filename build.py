import os
file_path = os.path.realpath(__file__)
print("Removing old builds")
try:
    os.system('rmdir -r bin')
    os.system('rmdir -r obj')
except OSError:
    print("Nothing to remove!")

print("Building..")
os.system(f"C:/CEdev/bin/make.exe '{file_path}'")
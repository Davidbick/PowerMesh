import shutil
import os


def read_version(version_h_file: str):
    major = minor = sub = "x"

    # Get Version
    with open(version_h_file) as ver_file:
        for line in ver_file:
            if line.startswith("#define FW_VERSION_MAJOR"):
                parts = line.split("\t")
                major = parts[-1].replace("\n", "")
            elif line.startswith("#define FW_VERSION_MINOR"):
                parts = line.split("\t")
                minor = parts[-1].replace("\n", "")
            elif line.startswith("#define FW_VERSION_SUB"):
                parts = line.split("\t")
                sub = parts[-1].replace("\n", "")
            else:
                pass

    version = f"{major}.{minor}.{sub}"

    return version


def create_programming_file(new_name: str):
    programmer_file_name = new_name.replace(".hex", "_program.bat")
    programmer_code = f'@echo off\n\nset CORTEX={new_name}\n\nPATH C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin\n\nSTM32_Programmer_CLI.exe -c port=SWD -w imgs\\%CORTEX%\n\npause> nul | set /p "=Press ENTER to continue"'

    print(f"Creating {programmer_file_name}...")
    programmer_file = open(programmer_file_name, "w")
    programmer_file.write(programmer_code)
    programmer_file.close()


def create_version(target, name_prefix: str):
    coord_name = "COORD"
    device_name = "DEVICE"

    if target is None:
        target = coord_name

    if os.getcwd().endswith(target):
        version_path = "../../NUCLEO-Common/Inc/version.h"
        target_path = ""
    else:
        version_path = "../NUCLEO-Common/Inc/version.h"
        target_path = f"{target}/"

    version = read_version(f"{version_path}")

    if "x" in version:
        exit(-1)
    elif target not in [coord_name, device_name]:
        exit(-2)
    else:
        old_name = f"{name_prefix}_{target}.hex"
        new_name = f"{name_prefix}_{target}_{version}.hex"

        print(f"Renaming {target_path}{old_name} to {target_path}{new_name}...")
        os.replace(f"{target_path}{old_name}", f"{target_path}{new_name}")
        
        create_programming_file(new_name)
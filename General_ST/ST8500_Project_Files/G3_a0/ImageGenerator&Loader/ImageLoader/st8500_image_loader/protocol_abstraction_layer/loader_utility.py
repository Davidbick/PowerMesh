import os
import win32console
import pywintypes


# Used to avoid to freeze the cmd window when clicked inside (no effect in pycharm or IDLE)
def quick_edit_mode(turn_on=None, silent=True):
    """ Enable/Disable windows console Quick Edit Mode """
    ENABLE_QUICK_EDIT_MODE = 0x40
    ENABLE_EXTENDED_FLAGS = 0x80
    is_on = False

    try:
        screen_buffer = win32console.GetStdHandle(win32console.STD_INPUT_HANDLE)
        try:
            orig_mode = screen_buffer.GetConsoleMode()
            is_on = (orig_mode & ENABLE_QUICK_EDIT_MODE)
            if is_on != turn_on and turn_on is not None:
                if turn_on:
                    is_on = True
                    new_mode = orig_mode | ENABLE_QUICK_EDIT_MODE
                else:
                    new_mode = orig_mode & ~ENABLE_QUICK_EDIT_MODE
                screen_buffer.SetConsoleMode(new_mode | ENABLE_EXTENDED_FLAGS)

            if not silent:
                print('Quick edit: %s' % ('enabled' if is_on else 'disabled'))
        except pywintypes.error:
            if not silent:
                print("Output-only terminal, no need to disable quick mode")
        except AttributeError:
            if not silent:
                print("No terminal, no need to disable quick mode")
    except pywintypes.error:
        pass
    except AttributeError:
        pass

    return is_on if turn_on is None else turn_on


def get_git_info(git_path=".git/"):
    branch_name = None
    ver_sha_code = None
    head_path = git_path + "HEAD"

    if os.path.isfile(head_path):
        head_file = open(head_path, "r")
        line = head_file.readline()
        head_file.close()

        line = line.replace("\n", "")
        parts = line.split(" ")
        ver_path = git_path + parts[-1]

        parts = parts[-1].split("/")
        branch_name = parts[-1]

        try:
            git_file = open(ver_path, "r")
            ver_sha_code = git_file.readline()
            git_file.close()

            ver_sha_code = ver_sha_code.replace("\n", "")
        except FileNotFoundError:
            print("Could not open: "+ver_path)

    return branch_name, ver_sha_code

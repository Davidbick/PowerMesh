import win32console
import pywintypes
import logging


# Used to avoid freezing the cmd window when clicked inside (no effect in pycharm or IDLE)
def quick_edit_mode(turn_on=None, silent=True):
    """ Enable/Disable Windows console Quick Edit Mode """
    ENABLE_QUICK_EDIT_MODE = 0x40
    ENABLE_EXTENDED_FLAGS = 0x80
    is_on = False

    logger = logging.getLogger("SYSTEM")

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
                logger.info('Quick edit: %s' % ('enabled' if is_on else 'disabled'))
        except pywintypes.error:
            if not silent:
                logger.warning("Output-only terminal, no need to disable quick mode")
        except AttributeError:
            if not silent:
                logger.warning("No terminal, no need to disable quick mode")
    except pywintypes.error:
        pass
    except AttributeError:
        pass

    return is_on if turn_on is None else turn_on

import sys
import serial
import serial.tools.list_ports
import argparse
import logging
import configparser
import loader_utility
from image_loader_st8500_core import StImageLoader, LoaderOp


def print_available_com_ports():
    """ Prints all the available serial ports
    :returns: A list of the serial ports available on the system
    """
    ports = serial.tools.list_ports.comports()
    print("Available COM ports:")
    port_found_n = 0
    for port, description, hw_id in sorted(ports):
        if description.startswith("STMicroelectronics STLink Virtual COM Port") or \
                description.startswith("Silicon Labs Dual CP2105 USB to UART Bridge:"):
            print("\t{}".format(description))
            port_found_n += 1

    if port_found_n == 0:
        print("\tNo compatible COM port found")
    print("")


def try_com_port(com_name: str, lg: logging.Logger):
    """
    Prompts user for the COM port if the given one is not available
    :param com_name: COM port name (string) to check
    :param lg:  logger to use for debug prints
    :return: The COM port name
    """
    il = StImageLoader(com_port, 115200, lg=lg)

    if com_name is not None:
        com_name = il.test_com_port(com_name)

    if com_name is None:
        print_available_com_ports()

    # Prompts user for the COM port to use
    while com_name is None:
        print("Insert COM port number: ", end='')
        com_name = "COM" + input()
        com_name = il.test_com_port(com_name, silent=True)

    return com_name


def show_progress(progress):
    print(f"\r{float(progress)}% ", end="")
    if progress == 100:
        print("")


if __name__ == '__main__':

    version_string = StImageLoader.version()

    print("ST8500 Image Loader - Version {}\n".format(version_string))

    config = configparser.ConfigParser()

    loader_utility.quick_edit_mode(False)

    parser = argparse.ArgumentParser()

    parser.add_argument('--port', action='store', dest='com_port', default=None,
                        help='Serial port name to be used (example: COM1)')
    parser.add_argument('--baudrate', action='store', default=115200,
                        help='Sets the baudrate value in bps for the image upload and the programming process')
    parser.add_argument('--pe', action='store', dest='pe_file', default="",
                        help='Pass PE img filename')
    parser.add_argument('--rte', action='store', dest='rte_file', default="",
                        help='Pass RTE img filename')
    parser.add_argument('--get_protocol', action='store_true',
                        help='Get FW protocol currently running on target (BOOT, PRIME, G3, MM)')
    parser.add_argument('--reset', action='store_true',
                        help='Force reset')
    parser.add_argument('--erase', action='store_true',
                        help='Erases the main RTE/PE image locations on the ST8500 SPI Flash Memory (sectors 0-5)')
    parser.add_argument('--G3_erase_NVM', action='store_true',
                        help='Erases the NVM locations on the ST8500 SPI Flash Memory (sectors 28-31)')
    parser.add_argument('--G3_erase_full', action='store_true',
                        help='Erases the entire SPI FLASH memory (sectors 0-31)')
    parser.add_argument('--write_to_RAM', action='store_true',
                        help='Uploads the specified PE and RTE images to the ST8500 RAM in boot from UART mode')
    parser.add_argument('--program_SPI_Flash', action='store_true',
                        help='Program the external ST8500 SPI Flash Memory')

    args = parser.parse_args()

    if args.com_port != "COM0":
        com_port = args.com_port
    else:
        com_port = None

    if args.get_protocol:
        operation = LoaderOp.check
    elif args.write_to_RAM:
        operation = LoaderOp.upload
    elif args.program_SPI_Flash:
        operation = LoaderOp.program
    elif args.reset:
        operation = LoaderOp.reset
    elif args.erase:
        operation = LoaderOp.erase
    elif args.G3_erase_NVM:
        operation = LoaderOp.erase_nvm
    elif args.G3_erase_full:
        operation = LoaderOp.erase_full
    else:
        operation = LoaderOp.invalid
        print("Executing command: Help")
        parser.print_help()
        sys.exit()

    # COM port (for all commands except help)
    if operation != LoaderOp.invalid:
        program_baudrate = int(args.baudrate)
        pe_file = args.pe_file
        rte_file = args.rte_file

        # Define logging format
        logger_fmt = "%(asctime)s %(levelname)s: %(message)s"
        logger_datefmt = "%H:%M:%S"
        logger_formatter = logging.Formatter(fmt=logger_fmt, datefmt=logger_datefmt)

        # Console Handler
        console_handler = logging.StreamHandler(sys.stdout)

        # Logger
        log = logging.getLogger(__name__)
        log.setLevel(logging.INFO)

        # Handlers
        log.addHandler(console_handler)

        com_port = try_com_port(com_port, lg=log)

        if com_port is not None:
            image_loader = StImageLoader(com_port, program_baudrate, lg=log)

            image_loader.run(operation, pe_file, rte_file, progress_ck=show_progress)
        else:
            raise Exception("Invalid COM port")

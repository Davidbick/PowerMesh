import os
from basenode import BaseNode
from sniffer import Sniffer
from servicenodeemb import ServiceNodeEmb

PYTHON_TEST_VERSION = "0056"

# Flag telling if the test logs will be generated in .xml (true) or .txt (false) format
# IMPORTANT: For execution under Jenkins, IT MUST BE SET TO True
use_XML_runner = True

# Flag telling the sniffer if packets dump has to be done either 
# during test execution or at the end of test execution
snifferPacketDumpDeferred = True

# Flag telling if directories of skipped tests have to be created or not
# IMPORTANT: For execution under Jenkins, IT MUST BE SET TO True
dumpSkippedTestsDir = True

# Max Number Of Consecutive Fails before passing to next testcase
max_consecutive_fails = 4

class Devices:

    # serial port names, overridden by environment variables if present - DO NOT CHANGE COMPORT NUMBERS!!!!
    bn_class,   bn_port,    bn_baud   =     BaseNode,        "COM10",     0   # 0080E14B9F11
    snif_class, snif_port,  snif_baud =     Sniffer,         "COM12",  57600  # 0080E14B9D91
    sn1_class,  sn1_port,   sn1_baud  =     ServiceNodeEmb,  "COM14",  57600  # 0080E14B9D89   
    sn2_class,  sn2_port,   sn2_baud  =     ServiceNodeEmb,  "COM16",  57600  # 0080E14B9D88
    sn3_class,  sn3_port,   sn3_baud  =     ServiceNodeEmb,  "COM18",  57600  # 0080E14B9DE1
    sn4_class,  sn4_port,   sn4_baud  =     ServiceNodeEmb,  "COM20",  57600  # 0080E14B9DE3


    # linux values 
    '''
    bn_class,   bn_port,    bn_baud   =     BaseNode,        "/dev/serial/by-path/pci-0000:00:1d.0-usb-0:1:1.0-port0",      0
    snif_class, snif_port,  snif_baud =     Sniffer,         "/dev/serial/by-path/pci-0000:00:1d.3-usb-0:2:1.0",  57600
    sn1_class,  sn1_port,   sn1_baud  =     ServiceNodeEmb,  "/dev/serial/by-path/pci-0000:00:1d.3-usb-0:1:1.0-port0",  57600
    sn2_class,  sn2_port,   sn2_baud  =     ServiceNodeEmb,  "/dev/serial/by-path/pci-0000:00:1d.0-usb-0:2:1.0", 57600
    sn3_class,  sn3_port,   sn3_baud  =     ServiceNodeEmb,  "/dev/serial/by-path/pci-0000:00:1d.2-usb-0:2:1.0",  57600
    sn4_class,  sn4_port,   sn4_baud  =     ServiceNodeEmb,  "/dev/serial/by-path/pci-0000:00:1d.2-usb-0:1:1.0-port0",  57600
    '''
    
    if 'PRIMECOM_BN' in os.environ:
        fields = os.environ['PRIMECOM_BN'].split(",")
        bn_port, bn_baud = fields[0], int(fields[1])

    if 'PRIMECOM_SNIFFER' in os.environ:
        fields = os.environ['PRIMECOM_SNIFFER'].split(",")
        snif_port, snif_baud = fields[0], int(fields[1])

    if 'PRIMECOM_SN1' in os.environ:
        fields = os.environ['PRIMECOM_SN1'].split(",")
        sn1_port, sn1_baud = fields[0], int(fields[1])

    if 'PRIMECOM_SN2' in os.environ:
        fields = os.environ['PRIMECOM_SN2'].split(",")
        sn2_port, sn2_baud = fields[0], int(fields[1])

    if 'PRIMECOM_SN3' in os.environ:
        fields = os.environ['PRIMECOM_SN3'].split(",")
        sn3_port, sn3_baud = fields[0], int(fields[1])

    if 'PRIMECOM_SN4' in os.environ:
        fields = os.environ['PRIMECOM_SN4'].split(",")
        sn4_port, sn4_baud = fields[0], int(fields[1])

    #sn1_port, sn3_port = sn3_port, sn1_port
    #sn1_class, sn3_class = sn3_class, sn1_class

ResultsFolder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results")

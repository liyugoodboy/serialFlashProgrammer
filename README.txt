Texas Instruments C2000 Serial Flash Programmer Example Package


This file contains instructions for using serial_flash_programmer.exe.


Command line parameters for serial_flash_programmer.exe:

Usage: serial_flash_programmer.exe  -x <device> -k <kernel name> -f <filename> -p COM<num>
                   [-m] <kernel name> [-n] <filename> [-b] <baudrate>
                   [-q] [-w] [-v]

	-d <device>  - The name of the device to load to
	               f2802x, f2803x, f2805x, f2806x, f2837xD, f2837xS or f2807x.
	-k <file>    - The file name for flash kernel
	               This file must be in the SCI boot format
	-a <file>    - The file name for download use
	               This file must be in the SCI boot format
	-p COM<num>  - Set the COM port to be used for communications
	-m <file>    - The CPU02 file name for flash kernel in dual core operations
	             - This file must be in the SCI boot format
	-n <file>    - The CPU02 file name for download use in dual core operations
	             - This file must be in the SCI boot format
	-b <num>     - Set the baud rate for the COM port
	-? or -h     - Show this help
	-q           - Quiet mode. Disable output to stdio
	-w           - Wait for a key press before exiting
	-v           - Enable verbose output


Before running serial_flash_programmer.exe, the microcontroller must be attached
to a COM port, must be running the SCI boot loader. If these conditions are met, 
the following command will load the included example programs:

	serial_flash_programmer.exe  -d f2837xD -k F2837xD_sci_flash_kernels_cpu01.txt  -a blinky_cpu01.txt 
                                 -b 9600 -p COM7

F2802x, F2803x, F2805x, F2806x Usage:

The serial flash programmer for these devices runs strictly from the command line.  The only
option and function supported is a device firmware upgrade.  It downloads the kernel to the device which
which running the sci boot loader and then branches to the kernel.  The host PC then performs 
an autobaud lock with the kernel and then sends the flash application to the kernel which receives 
the data, erases and programs the flash.
            
F2837xS, F2807x Usage:

The program reads the input file data into memory, then attempts to open the
COM port and sends a word by word transfer containing the kernel data in ASCII boot format.
The boot loader of the device copies the incoming data into RAM, then branches to 
the specified entry point.  At this point the flash kernel is running.  The host application 
then waits for input from the user from command line and requests any necessary data to 
send to the device running the kernel.  Below are the possible options given to the user of 
the flash programming solution...
•	DFU
•	Erase 
•	Verify 
•	Unlock Zone 1
•	Unlock Zone 2
•	Run CPU1
•	Reset CPU1
•	Done

DFU stands for device firmware upgrade and erases, programs and verifies the necessary flash sectors.
The DFU options use the input flash application files which must be in the appropriate format as
described below.  The Verify function also uses these input files to send to the device so that
it can verify the flash contents.  When Erase is selected, the application requests which sectors
to erase from the user on the command line.  When Unlock is selected, the application requests
for four 32-bit passwords in hex from the user from standard input.  When Run is selected, the 
application requests the entry point address to branch to.  When Reset is selected, the command is 
sent to the device and the device performs a watchdog reset and the application exits.
            
F2837xD Dual Core Soprano Usage:                         
                                 
The program reads the input file data into memory, then attempts to open the
COM port and sends a word by word transfer containing the kernel data in ASCII boot format.
The boot loader of the device copies the incoming data into RAM, then branches to 
the specified entry point.  At this point the flash kernel is running.  The host application 
then waits for input from the user from command line and requests any necessary data to 
send to the device running the kernel.  Below are the possible options given to the user of 
the flash programming solution...
•	DFU CPU1
•	DFU CPU2
•	Erase CPU1
•	Erase CPU2
•	Verify CPU1
•	Verify CPU2
•	Unlock CPU1 Zone 1
•	Unlock CPU1 Zone 2
•	Unlock CPU2 Zone 1
•	Unlock CPU2 Zone 2
•	Run CPU1
•	Reset CPU1
•	Run CPU1 Boot CPU2
•	Reset CPU1 Boot CPU2
•	Run CPU2
•	Reset CPU2
•	Done

DFU stands for device firmware upgrade and erases, programs and verifies the necessary flash sectors.
The DFU options use the input flash application files which must be in the appropriate format as
described below.  The Verify function also uses these input files to send to the device so that
it can verify the flash contents.  When Erase is selected, the application requests which sectors
to erase from the user on the command line.  When Unlock is selected, the application requests
for four 32-bit passwords in hex from the user from standard input.  When Run is selected, the 
application requests the entry point address to branch to.  When Reset is selected, the command is 
sent to the device and the device performs a watchdog reset and the application exits.

When the application starts, the serial connection is with the SCI module which is owned by CPU1.
Only commands designated for CPU1 are acceptable at this point before booting CPU2.  If a CPU2 command
is selected, the application asks the user for a different option.  After booting to CPU2, either with
"Run CPU1 Boot CPU2" or "Reset CPU1 Boot CPU2," CPU1 relinquishes the SCI module and Flash pump to CPU2.
Therefore, the host application then prohibits any CPU1 commands to be selected and only allows 
communication with CPU2 from that point forward.

The Flash Kernel:

The flash kernel communicates with the host via the SCI module.  It waits for commands from the 
host and sends an ACK or NAK depending whether or not the packet was received properly.  The 
kernel has the ability to unlock zones 1 and 2, program a flash application, verify the contents
of the flash, erase the flash (all or specific sectors), and finally either run the core by
branching to a specific entry point address or reset the core with a watchdog timer.

Input File Format:

The input files (kernel and flash application) must be a ASCII file containing only data in 
the standard boot loader format. This format is described in the ROM Code and Peripheral Booting
chapter of the device TRM and is reproduced below for convenience. To convert
a .out file compiled by Code Composer Studio into the required format, use the
hex2000 tool, which is part of the C2000 Code Generation Tools package:

	hex2000 -boot -a -sci8 <.out filename> -o <output filename>

Boot loader data format:
The Serial boot loader data follows the same format as the other peripheral boot loaders.

Byte   Contents
1      LSB: AA (KeyValue for memory width = 8-bits)
2      MSB: 08h (KeyValue for memory width = 8-bits)
3      LSB: bytes 3-18 reserved for future use
4      MSB: bytes 3-18 reserved for future use
...    ...
17     LSB: bytes 3-18 reserved for future use
18     MSB: bytes 3-18 reserved for future use
19     LSB: Upper half (MSW) of Entry point PC[23:16]
20     MSB: Upper half (MSW) of Entry point PC[31:24] (Note: Always 0x00)
21     LSB: Lower half (LSW) of Entry point PC[7:0]
22     MSB: Lower half (LSW) of Entry point PC[15:8]
23     LSB: Block size[7:0] (number of 16-bit words) of the first block of data to load
24     MSB: Block size[15:8]
25     LSB: Block load starting address [23:16]
26     LSB: Block load starting address [31:24]
27     LSB: Block load starting address [7:0]
28     LSB: Block load starting address [15:8]
29     LSB: First data word in the block
30     MSB: First data word in the block
...    ...
n      MSB: Last data word in block
n+1    LSB: Block size[7:0] of the next block of data
[Same structure as the first block]
x      LSB: Block size[7:0] of 0x0000 indicates the end of the load
x+1    MSB: Block size[15:8] of 0x0000 indicates the end of the load

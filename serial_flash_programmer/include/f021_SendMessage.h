//###########################################################################
// FILE:   f021_DownloadIimage.h
// TITLE:  Download Image function for f021 devices.
//
// This function is used to communicate and download with the device.  For 
// F021 devices, the serial flash programmer sends the application the same
// way it does the kernel.  In both instances, the serial flash programmer
// send one byte and the device echos back that same byte.
//###########################################################################
// $TI Release: F28X7X Support Library$
// $Release Date: Octobe 23, 2014 $
//###########################################################################

#ifndef __F021_SENDMESSAGE__
#define __F021_SENDMESSAGE__

extern void clearBuffer(void);
extern void autobaudLock(void);
extern void loadProgram(FILE *fh);
uint32_t constructPacket(uint8_t* packet, uint16_t command, uint16_t length, uint8_t * data);
int f021_SendPacket(uint8_t* packet, uint32_t length);

#endif
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

#ifndef __F021_DOWNLOADKERNEL__
#define __F021_DOWNLOADKERNEL__

void clearBuffer(void);
void autobaudLock(void);
void loadProgram(FILE *fh);
int f021_DownloadKernel(wchar_t* kernel);

#endif
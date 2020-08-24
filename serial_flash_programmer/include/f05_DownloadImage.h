//###########################################################################
// FILE:   f05_DownloadImage.h
// TITLE:  Download Image function required for serial flash programmer.
//
// This function is used to communicate and download with the device.  For 
// F05 devices, the serial flash programmer loads the kernel with a byte by
// byte echo back.  When communicating the application to the kernel, it
// works differently.  It sends chunks of the application and waits for 
// a checksum of that data.
//###########################################################################
// $TI Release: F28X7X Support Library$
// $Release Date: Octobe 23, 2014 $
//###########################################################################

#ifndef __F05_DOWNLOADIMAGE__
#define __F05_DOWNLOADIMAGE__

int f05_DownloadImage(void);

#endif
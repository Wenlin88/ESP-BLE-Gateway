#ifndef _RUUVITAG_H    // Put these two lines at the top of your file.
#define _RUUVITAG_H    // (Use a suitable name, usually based on the file name.)


// 16 bit
short getShort(unsigned char* data, int index);
short getShortone(unsigned char* data, int index);
// 16 bit
unsigned short getUShort(unsigned char* data, int index);
unsigned short getUShortone(unsigned char* data, int index);

#endif // _RUUVITAG_H    // Put this line at the end of your file.
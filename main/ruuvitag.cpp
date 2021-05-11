// 16 bit
short getShort(unsigned char* data, int index)
{
  return (short)((data[index] << 8) + (data[index + 1]));
}

short getShortone(unsigned char* data, int index)
{
  return (short)((data[index]));
}

// 16 bit
unsigned short getUShort(unsigned char* data, int index)
{
  return (unsigned short)((data[index] << 8) + (data[index + 1]));
}

unsigned short getUShortone(unsigned char* data, int index)
{
  return (unsigned short)((data[index]));
}

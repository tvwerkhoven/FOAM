#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>
#include <unistd.h>

// Logging levels
#define IO_ERR          0x00000001 | 0x00000400
#define IO_WARN         0x00000002
#define IO_INFO         0x00000003
#define IO_XNFO         0x00000004
#define IO_DEB1         0x00000005
#define IO_DEB2         0x00000006
#define IO_LEVEL_MASK   0x000000FF

// Logging flags
#define IO_NOID         0x00000100      //!< Do not add loglevel string
#define IO_FATAL        0x00000200      //!< Fatal, quit immediately

#define max(x,y) (x) > (y) ? (x) : (y)

class Io{
  FILE *f;
  unsigned char iocmd,swap_endian;
  int verb,level_mask;
public:
  Io(void);
  Io::Io(int l);
  ~Io(void);
  void reconf(int);
  int msg(int,const char*,...);
  int vlevel(void){ return verb; }
};

#endif

#ifndef SYS_INFO
#define SYS_INFO
struct sysinfo {
  uint64 freemem;   // amount of free memory (bytes)
  uint64 nproc;     // number of process
};
#endif

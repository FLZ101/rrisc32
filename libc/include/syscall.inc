
void exit(int status);
int open(char *filename, char *mode);
void close(int fd);
long read(int fd, void *buf, unsigned long count);
long write(int fd, void *buf, unsigned long count);
long lseek(int fd, long offset, int whence);
void *sbrk(long inc);
void sleep(unsigned int ms);

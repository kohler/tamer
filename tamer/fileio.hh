#ifndef TAMER_FILEIO_HH
#define TAMER_FILEIO_HH
#include <tamer.hh>
namespace tamer {
namespace fileio {

int make_nonblocking(int fd);
void open(const char *filename, int flags, event<int> done);
void open(const char *filename, int flags, mode_t mode, event<int> done);
void read(int fd, void *buf, size_t size, ssize_t &size_out, event<int> done);
void write(int fd, void *buf, size_t size, ssize_t &size_out, event<int> done);
void close(int fd);

inline void open(const char *filename, int flags, event<int> done) {
    open(filename, flags, 0, done);
}

}}
#endif /* TAMER_TAME_FILEIO_HH */

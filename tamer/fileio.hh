#ifndef TAMER_FILEIO_HH
#define TAMER_FILEIO_HH
#include <tamer.hh>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
namespace tamer {
namespace fileio {

int make_nonblocking(int fd);
void open(const char *filename, int flags, event<int> done);
void open(const char *filename, int flags, mode_t mode, event<int> done);
void fstat(int fd, struct stat &stat_out, event<int> done);
void read(int fd, void *buf, size_t size, ssize_t &size_out, event<int> done);
void write(int fd, const void *buf, size_t size, ssize_t &size_out, event<int> done);
void close(int fd);
void pipe(int fd_out[2], event<int> done);
void socket(int domain, int type, int protocol, event<int> done);
void accept(int listenfd, struct sockaddr *name_out, socklen_t *namelen_out, event<int> done);
void connect(int fd, const struct sockaddr *name, socklen_t namelen, event<int> done);

inline void open(const char *filename, int flags, event<int> done) {
    open(filename, flags, 0, done);
}

}}
#endif /* TAMER_TAME_FILEIO_HH */

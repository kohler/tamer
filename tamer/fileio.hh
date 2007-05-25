#ifndef TAMER_FILEIO_HH
#define TAMER_FILEIO_HH
#include <tamer/tamer.hh>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
namespace tamer {
namespace fileio {

/** @namespace tamer::fileio
 *  @brief  Namespace containing public Tamer classes and functions for
 *  file descriptor I/O.
 */

/** @file <tamer/fileio.hh>
 *  @brief  Functions for asynchronous I/O.
 */

/** @brief  Make file descriptor nonblocking.
 *  @param  fd  File descriptor.
 *  @return  0 on success, or a negative error code.
 */
int make_nonblocking(int fd);

/** @brief  Open file.
 *  @param  filename  Filename.
 *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, etc.).
 *  @param  done      Event triggered on completion.
 *
 *  @a done is triggered with the file descriptor on success, or a negative
 *  error code.
 */
void open(const char *filename, int flags, event<int> done);

/** @brief  Open file.
 *  @param  filename  Filename.
 *  @param  flags     Open flags (@c O_RDONLY, @c O_EXCL, etc.).
 *  @param  mode      Permissions mode (used when creating a file).
 *  @param  done      Event triggered on completion.
 *
 *  @a done is triggered with the file descriptor on success, or a negative
 *  error code.
 */
void open(const char *filename, int flags, mode_t mode, event<int> done);

/** @brief  Fetch file descriptor status.
 *  @param       fd    File descriptor.
 *  @param[out]  stat  Status structure.
 *  @param       done  Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 */
void fstat(int fd, struct stat &stat, event<int> done);

/** @brief  Read from file descriptor.
 *  @param       fd     File descriptor.
 *  @param[out]  buf    Buffer.
 *  @param       size   Buffer size.
 *  @param[out]  nread  Number of characters read.
 *  @param       done   Event triggered on completion.
 *
 *  @a done is triggered with 0 on success or end-of-file, or a negative error
 *  code.  @a nread is kept up to date as the read progresses.
 */
void read(int fd, void *buf, size_t size, ssize_t &nread, event<int> done);

/** @brief  Write to file descriptor.
 *  @param       fd        File descriptor.
 *  @param       buf       Buffer.
 *  @param       size      Buffer size.
 *  @param[out]  nwritten  Number of characters written.
 *  @param       done      Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.  @a
 *  nwritten is kept up to date as the write progresses.
 */
void write(int fd, const void *buf, size_t size, ssize_t &nwritten, event<int> done);

/** @brief  Close file descriptor.
 *  @param  fd    File descriptor.
 *  @return  0 on success, or a negative error code.
 *
 *  @note Use @c tamer::fileio::close in preference to the @c close system call
 *  because the @c tamer version triggers @c at_fd_close events.
 */
int close(int fd);

/** @brief  Close file descriptor.
 *  @param  fd    File descriptor.
 *  @param  done  Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 *
 *  @note Use @c tamer::fileio::close in preference to the @c close system call
 *  because the @c tamer version triggers @c at_fd_close events.
 */
void close(int fd, event<int> done);

/** @brief  Create a nonblocking pipe.
 *  @param[out]  fd    Pipe file descriptors.
 *  @return  0 on success, or a negative error code.
 *
 *  Both file descriptors in @a fd are made nonblocking.
 */
int pipe(int fd[2]);

/** @brief  Create a nonblocking pipe.
 *  @param[out]  fd    Pipe file descriptors.
 *  @param       done  Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.  Both
 *  file descriptors in @a fd are made nonblocking.
 */
void pipe(int fd[2], event<int> done);

/** @brief  Open a nonblocking socket.
 *  @param  domain    Protocol domain.
 *  @param  type      Transport type.
 *  @param  protocol  Protocol.
 *  @return  Socket file descriptor on success, or a negative error code.
 *
 *  The returned file descriptor is made nonblocking.
 */
int socket(int domain, int type, int protocol);

/** @brief  Open a nonblocking socket.
 *  @param  domain    Protocol domain.
 *  @param  type      Transport type.
 *  @param  protocol  Protocol.
 *  @param  done      Event triggered on completion.
 *
 *  @a done is triggered with the socket file descriptor on success, or a
 *  negative error code.  The returned file descriptor is made nonblocking.
 */
void socket(int domain, int type, int protocol, event<int> done);

/** @brief  Accept a nonblocking connection on a listening socket.
 *  @param       listenfd  Listening file descriptor.
 *  @param[out]  name      Socket address of accepted connection.
 *  @param[out]  namelen   Length of socket address of accepted connection.
 *  @param       done      Event triggered on completion.
 *
 *  @a done is triggered with the accepted file descriptor on success, or a
 *  negative error code.  The returned file descriptor is made nonblocking.
 */
void accept(int listenfd, struct sockaddr *name, socklen_t *namelen, event<int> done);

/** @brief  Connect a socket to an address.
 *  @param  fd       Socket file descriptor.
 *  @param  name     Socket address.
 *  @param  namelen  Length of socket address.
 *  @param  done     Event triggered on completion.
 *
 *  @a done is triggered with 0 on success, or a negative error code.
 */
void connect(int fd, const struct sockaddr *name, socklen_t namelen, event<int> done);

inline void open(const char *filename, int flags, event<int> done) {
    open(filename, flags, 0, done);
}

}}
#endif /* TAMER_FILEIO_HH */

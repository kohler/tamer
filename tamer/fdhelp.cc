#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

#define FDH_CLONE 100
#define FDH_KILL  101
#define FDH_OPEN  102
#define FDH_STAT  103
#define FDH_READ  104
#define FDH_WRITE 105

/* clone
 * > msg: req
 * < msg: errno | pid; [anc: fd]
 *
 * kill
 * > msg: req
 *
 * open 
 *  > msg: req | mode | filename
 *  < msg: errno; [anc: fd]
 *
 * stat
 *  > msg: req, anc: fd
 *  < msg: errno | [stat]
 *
 * read
 *  > msg: req | size; anc: fd
 *  < sendfile: ifd, ofd, NULL, size
 *
 * write
 *  > msg: req; anc: fd
 *  > write: pipe 
 *  < read: pipe -> write: fd
 */

int send(int fd, int fd_to_send, char * buf, size_t len) {
  struct iovec iov[1];
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  char anc[CMSG_SPACE(sizeof(int))];

  iov[0].iov_base = buf;
  iov[0].iov_len = len;
  
  msgh.msg_iov = iov;
  msgh.msg_iovlen = 1;
  
  msgh.msg_name = NULL;
  msgh.msg_namelen = 0;

  if (fd < 0) {
    msgh.msg_control = NULL;
    msgh.msg_controllen = 0;
  } else {
    msgh.msg_control = anc;
    msgh.msg_controllen = sizeof(anc);

    cmsg = CMSG_FIRSTHDR(&msgh);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    *(int *)CMSG_DATA(cmsg) = fd_to_send;

    msgh.msg_controllen = cmsg->cmsg_len;
  }

  msgh.msg_flags = 0;

  return sendmsg(fd, &msgh, 0);
}

int recv(int fd, int * fd_to_recv, char * buf, size_t len) {
  int r;
  int fdt;
  struct iovec iov[1];
  struct msghdr msgh;
  struct cmsghdr *cmsg;
  char anc[CMSG_SPACE(sizeof(int))];

  iov[0].iov_base = buf;
  iov[0].iov_len = len;
  
  msgh.msg_iov = iov;
  msgh.msg_iovlen = 1;
  
  msgh.msg_name = NULL;
  msgh.msg_namelen = 0;

  msgh.msg_control = anc;
  msgh.msg_controllen = sizeof(anc);

  if ((r = recvmsg(fd, &msgh, 0)) < 0)
    return r;
 
  if (msgh.msg_controllen > 0) {
    cmsg = CMSG_FIRSTHDR(&msgh);
    if (cmsg->cmsg_len == sizeof(anc) && cmsg->cmsg_type == SCM_RIGHTS)
      fdt = *(int *)CMSG_DATA(cmsg);
    if (fd_to_recv != NULL)
      *fd_to_recv = fdt;
    else
      close(fdt);
  } else if (fd_to_recv != NULL)
    *fd_to_recv = -1;
  
  return r;
}

int main (void) {
  char buf[8192];
  int len;
  int fd;
  
  pid_t pid;
  int socks[2], chfd;
  
  mode_t mode;
  char * fname;
  
  struct stat * stat;
  
  size_t size;
  ssize_t ssize;

  for (;;) {
    if ((len = recv(0, &fd, buf, sizeof(buf))) < 0) {
      perror("recvmsg");
      goto exit_;
    } else if (len == 0)
      goto exit_;
    
    switch (buf[0]) {
      case FDH_CLONE:
        printf("clone %d\n", getpid());
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) < 0) {
          chfd = -1;
          *(int *)buf = errno;
          len = sizeof(int);
          goto forkerr;
        }
        
        pid = fork();
        if (pid < 0) {
          chfd = -1;
          *(int *)buf = errno;
          len = sizeof(int);
          goto forkerr;
        } else if (pid == 0) {
          // child
          printf("!child %d\n", getpid()); 
          close(0);
          close(socks[0]);
          if (dup2(socks[1], 0) < 0) {
            close(socks[1]);
            goto exit_;
          }
          close(socks[1]);
          break;
        }
        
        chfd = socks[0];
        *(int *)buf = 0;
        *(pid_t *)&buf[sizeof(int)] = pid;
        len = sizeof(int) + sizeof(pid_t);
    forkerr:
        close(socks[1]);
        if (send(0, chfd, buf, len) < 0) {
          perror("sendmsg");
          close(socks[0]);
          goto exit_;
        }
        close(socks[0]);
        break;
      case FDH_KILL:
        printf("kill %d\n", getpid());
        goto exit_;
        break;
      case FDH_OPEN:
        printf("open %d\n", getpid());
        mode = *(int *)&buf[1];
        fname = &buf[1 + sizeof(mode_t)];
        *(int *)buf = ((fd = open(fname, mode)) < 0) ? errno : 0;
        if (send(0, fd, buf, sizeof(int)) < 0) {
          perror("sendmsg");
          goto exit;
        }
        close(fd);
        break;
      case FDH_STAT:
        printf("stat %d\n", getpid());
        stat = (struct stat *)&buf[sizeof(int)];
        *(int *)buf = (fstat(fd, stat) < 0) ? errno : 0;
        if (send(0, fd, buf, sizeof(int) + sizeof(struct stat)) < 0) {
          perror("sendmsg");
          goto exit;
        }
        close(fd);
        break;
      case FDH_READ:
        printf("read %d\n", getpid());
        size = *(size_t *)&buf[1];
        if (sendfile(0, fd, NULL, size) < 0) {
          /* TODO handle error gracefully */
          perror("sendfile");
          goto exit;
        }
        close(fd);
        break;
      case FDH_WRITE:
        printf("write %d %d\n", fd, getpid());
        do {
          if ((ssize = read(0, buf, sizeof(buf))) < 0) {
          /* TODO handle error gracefully */
            perror("read");
            goto exit;
          }
          if (ssize)
            if (ssize != write(fd, buf, (size_t)ssize)) {
          /* TODO handle error gracefully */
              perror("\nwritex");
              goto exit;
            }
        } while (ssize > 0);
        close(fd);
        break;
      default:
        fprintf(stderr, "Unknown request\n");
        goto exit;
        break;
    }
  }

exit:
  close(fd);
exit_:
  printf("exit %d\n", getpid());
  close(0);
  exit(0);
}


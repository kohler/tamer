#include <tamer/fdhmsg.hh>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#ifdef __linux__
#include <sys/sendfile.h>
#endif
#include <sys/stat.h>
#include <signal.h>

void terminate(int);
int query_count = 0;

int main (void) {
  char buf[8192];
  int len;
  int fd;
  
  pid_t pid;
  int socks[2], chfd;
  
  fdh_msg * msg;

  char * fname;
  struct stat * stat;
  
  size_t size;
  ssize_t ssize;

  if (signal(SIGTERM, terminate) == SIG_ERR) {
    perror("unable to set signal");
    exit(0); 
  }

  for (;;) {
    if ((len = fdh_recv(0, &fd, buf, sizeof(buf))) < 0) {
      perror("recvmsg");
      goto exit_;
    } else if (len == 0)
      goto exit_;
    
    query_count ++; 
    msg = (fdh_msg *)buf;

    switch (msg->query.req) {
      case FDH_CLONE:
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) < 0) {
          chfd = -1;
          msg->reply.err = errno;
          goto forkerr;
        }
        
        msg->reply.pid = pid = fork();
        if (pid < 0) {
          chfd = -1;
          msg->reply.err = errno;
          goto forkerr;
        } else if (pid == 0) {
          // child
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
        msg->reply.err = 0;
      forkerr:
        close(socks[1]);
        if (fdh_send(0, chfd, (char *)msg, FDH_MSG_SIZE) < 0) {
          perror("sendmsg");
          close(socks[0]);
          goto exit_;
        }
        close(socks[0]);
        break;
      case FDH_OPEN:
        fname = (char *)&buf[FDH_MSG_SIZE];
        msg->reply.err = ((fd =
              open(fname, msg->query.flags, msg->query.mode)) < 0) ? errno : 0;
        if (fdh_send(0, fd, (char *)msg, FDH_MSG_SIZE) < 0) {
          perror("sendmsg");
          goto exit;
        }
        close(fd);
        break;
      case FDH_STAT:
        stat = (struct stat *)&buf[FDH_MSG_SIZE];
        msg->reply.err = (fstat(fd, stat) < 0) ? errno : 0;
        if (fdh_send(0, -1, (char *)msg, FDH_MSG_SIZE + sizeof(struct stat)) < 0) {
          perror("sendmsg");
          goto exit;
        }
        close(fd);
        break;
      case FDH_READ:
#if __linux__
        if (sendfile(0, fd, NULL, msg->query.size) < 0) {
          /* TODO handle error gracefully ?*/
          perror("sendfile");
          goto exit;
        }
        close(fd);
        break;
#else
	perror("no sendfile");
	goto exit;
#endif
      case FDH_WRITE:
        size = msg->query.size; 
        do {
          if ((ssize = read(0, buf, sizeof(buf))) < 0) {
            /* TODO handle error gracefully ? signal to parent?*/
            perror("helper: read");
            goto exit;
          } else if (ssize == 0)
            break;
          if (ssize)
            if (ssize != write(fd, buf, (size_t)ssize)) {
              /* TODO handle error gracefully ? signal to parent?*/
              perror("helper: write");
              goto exit;
            }
          size -= ssize;
        } while (size);
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
  //TODO send signal to parent and restart loop instead of exiting (maybe)
  terminate(0);
}

void terminate(int) {
  //printf("exit %d query_count %d\n", getpid(), query_count);
  close(0);
  exit(0);  
}


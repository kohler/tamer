#include <tamer/fdhmsg.hh>

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

  //del
  struct stat s;

  for (;;) {
    if ((len = fdh_recv(0, &fd, buf, sizeof(buf))) < 0) {
      perror("recvmsg");
      goto exit_;
    } else if (len == 0)
      goto exit_;
    
    msg = (fdh_msg *)buf;

    switch (msg->query.req) {
      case FDH_CLONE:
        printf("clone %d\n", getpid());
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
      case FDH_KILL:
        printf("kill %d\n", getpid());
        goto exit_;
        break;
      case FDH_OPEN:
        printf("open %d\n", getpid());
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
        printf("stat %d\n", getpid());
        stat = (struct stat *)&buf[FDH_MSG_SIZE];
        msg->reply.err = (fstat(fd, stat) < 0) ? errno : 0;
        if (fdh_send(0, fd, buf, sizeof(int) + sizeof(struct stat)) < 0) {
          perror("sendmsg");
          goto exit;
        }
        close(fd);
        break;
      case FDH_READ:
        printf("read %d\n", getpid());
        if (sendfile(0, fd, NULL, msg->query.size) < 0) {
          /* TODO handle error gracefully ?*/
          perror("sendfile");
          goto exit;
        }
        close(fd);
        break;
      case FDH_WRITE:
        printf("write %d %d\n", fd, getpid());
        size = msg->query.size; 
        do {
          if ((ssize = read(0, buf, sizeof(buf))) < 0) {
          /* TODO handle error gracefully ?*/
            perror("help: read");
            goto exit;
          } else if (ssize == 0)
            break;
          if (ssize)
            if (ssize != write(fd, buf, (size_t)ssize)) {
              /* TODO handle error gracefully ?*/
              perror("\nhelp: write");
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
  printf("exit %d\n", getpid());
  close(0);
  exit(0);
}

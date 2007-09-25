#include <tamer/fdhmsg.hh>

int fdh_send(int fd, int fd_to_send, char * buf, size_t len) {
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

  if (fd_to_send < 0) {
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

int fdh_recv(int fd, int * fd_to_recv, char * buf, size_t len) {
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
    if (cmsg->cmsg_len == sizeof(anc) && cmsg->cmsg_type == SCM_RIGHTS) {
      fdt = *(int *)CMSG_DATA(cmsg);
      if (fd_to_recv != NULL)
        *fd_to_recv = fdt;
      else
        close(fdt);
    }
  } else if (fd_to_recv != NULL)
    *fd_to_recv = -1;
  
  return r;
}



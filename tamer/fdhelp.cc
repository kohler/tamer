#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

static const char * const succ = "succ";
static const char * const fail = "fail";

int main (int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  char request[255];
  char temp;
  int ret;
  int fd;
  fd_set rset;
  
  FD_ZERO(&rset);

  for (;;) {
    FD_SET(0, &rset); 
    select(1, &rset, NULL, NULL, NULL); 

    ret = read(0, request, sizeof(request));

    if (ret <= 0)
      return 0;

    fd = open(request, O_RDONLY);
    
    if (fd < 0)
      ret = write(1, fail, sizeof(fail));
    else
      ret = write(1, succ, sizeof(succ));
    
    if (ret <= 0) {
      close(fd);
      return 0;
    }
    
    ret = read(fd, &temp, sizeof(temp));

    close(fd);

    if (ret < 0)
      return 0;
  }

}

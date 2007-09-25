#ifndef TAMER_FDH_HH
#define TAMER_FDH_HH 1

#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <tamer/ref.hh>
#include <tamer/fdhmsg.hh>


#define FDH_MAX 5
#define FDH_MIN 2

namespace tamer {

class fdh {

  struct fdh_state 
    : public enable_ref_ptr_with_full_release<fdh_state> {

    union {
      fdh_msg _msg;
      char _buf[255 + FDH_MSG_SIZE];//TODO find max path len const
    };

    int   _fd;  
    pid_t _pid;
    mutex _lock;
    //event<> _kill;

    fdh_state();
    fdh_state(int fd, pid_t pid);

    operator bool() {
      return _pid > 0 && _fd > 0;
    }

    void full_release() {
      //TODO _kill.trigger(); 
    }
    
    int make_nonblocking();
    void send(int fd, char * buf, size_t size, event<int> done);
    void recv(int * fd, char * buf, size_t size, event<int> done);
    
    void kill(event<> done);
    void clone(event<fdh> done);
    void open(std::string fname, int flags, mode_t mode, event<int> fd);
    void fstat(int fd, struct stat &stat_out, event<int> done); 
    void read(int fd, size_t size, event<int> fd, event<> release);
    void write(int fd, size_t size, event<int> fd, event<> release);

    class closure__send__iPckQi_;
    void send(closure__send__iPckQi_ &, unsigned);

    class closure__recv__PiPckQi_;
    void recv(closure__recv__PiPckQi_ &, unsigned); 

    class closure__kill__Q_;
    void kill(closure__kill__Q_ &, unsigned);
    
    class closure__clone__Q3fdh_; 
    void clone(closure__clone__Q3fdh_ &, unsigned);

    class closure__open__Ssi6mode_tQi_;
    void open(closure__open__Ssi6mode_tQi_ &, unsigned); 
    
    class closure__fstat__iR4statQi_;
    void fstat(closure__fstat__iR4statQi_ &, unsigned); 
    
    class closure__read__ikQi_Q_;
    void read(closure__read__ikQi_Q_ &, unsigned); 
    
    class closure__write__ikQi_Q_;
    void write(closure__write__ikQi_Q_ &, unsigned); 
  };

  ref_ptr<fdh_state> _p;

public:
  inline fdh();
  inline fdh(int fd, pid_t pid);
  inline fdh(const fdh &other);
  inline void clone(event<fdh> done);
  inline void open(std::string fname, int flags, mode_t mode, event<int> fd);
  inline void fstat(int fd, struct stat &stat_out, event<int> done); 
  inline void read(int fd, size_t size, event<int> fdo, event<> release);
  inline void write(int fd, size_t size, event<int> fdi, event<> release);
};

inline fdh::fdh()
  : _p(new fdh_state()) {
}

inline fdh::fdh(int fd, pid_t pid)
  : _p((fd > 0 && pid > 0) ? new fdh_state(fd, pid) : 0) {
}

inline fdh::fdh(const fdh &other) 
  : _p(other._p) {
}

inline void fdh::clone(event<fdh> done) {
  if (_p)
    _p->clone(done);
  else
    done.trigger(*this);
}

inline void fdh::open(std::string fname, int flags, mode_t mode, event<int> fd) {
  if (_p)
    _p->open(fname, flags, mode, fd);
  else
    fd.trigger(-1);
}

inline void fdh::fstat(int fd, struct stat &stat_out, event<int> done) {
  if (_p)
    _p->fstat(fd, stat_out, done);
  else
    done.trigger(-1);//TODO better error?
}

inline void fdh::read(int fd, size_t size, event<int> fdo, event<> release) {
  if (_p)
    _p->read(fd, size, fdo, release);
  else
    fdo.trigger(-1); 
}

inline void fdh::write(int fd, size_t size, event<int> fdi, event<> release) {
  if (_p)
    _p->write(fd, size, fdi, release);
  else
    fdi.trigger(-1);
}

}
#endif /* TAMER_FDH_HH */

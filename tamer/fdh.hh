#ifndef TAMER_FDH_HH
#define TAMER_FDH_HH 1

#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <tamer/ref.hh>
#include <tamer/fdhmsg.hh>
#include <list>

#define FDH_MAX 6 //TODO relocate, maybe tamer.hh
#define FDH_MIN 4 //TODO relocate

namespace tamer {

class fdh {

  struct fdh_state 
    : public enable_ref_ptr_with_full_release<fdh_state> {

    union {
      fdh_msg _msg;
      char _buf[255 + FDH_MSG_SIZE];//TODO find max path len const
    };

    pid_t _pid;
    int   _fd;  
    mutex _lock;

    fdh_state();
    fdh_state(pid_t pid, int fd);
    ~fdh_state() {
      _pid = -1;
      _fd = -1; 
    }
    
    operator bool() {
      return _pid > 0 && _fd > 0;
    }

    void full_release() {
      if (kill(_pid, SIGTERM) < 0)
        perror("unable to SIGTERM helper");
    }
    
    int make_nonblocking();
    void send(int fd, char * buf, size_t size, event<int> done);
    void recv(int * fd, char * buf, size_t size, event<int> done);
   
    void clone(event<fdh> done);
    void open(std::string fname, int flags, mode_t mode, event<int> fd);
    void fstat(int fd, struct stat &stat_out, event<int> done); 
    void read(int fd, size_t size, event<int> fd, event<> release);
    void write(int fd, size_t size, event<int> fd, event<> release);

    class closure__send__iPckQi_;
    void send(closure__send__iPckQi_ &, unsigned);

    class closure__recv__PiPckQi_;
    void recv(closure__recv__PiPckQi_ &, unsigned); 
    
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
  typedef ref_ptr<fdh_state> fdh::*unspecified_bool_type;

  inline fdh();
  inline fdh(pid_t pid, int fd);
  inline fdh(const fdh &other);
  inline operator unspecified_bool_type();
  inline void clone(event<fdh> done);
  inline void open(std::string fname, int flags, mode_t mode, event<int> fd);
  inline void fstat(int fd, struct stat &stat_out, event<int> done); 
  inline void read(int fd, size_t size, event<int> fdo, event<> release);
  inline void write(int fd, size_t size, event<int> fdi, event<> release);
};

inline fdh::fdh()
  : _p(new fdh_state()) {
}

inline fdh::fdh(pid_t pid, int fd)
  : _p((fd > 0 && pid > 0) ? new fdh_state(pid, fd) : 0) {
}

inline fdh::fdh(const fdh &other) 
  : _p(other._p) {
}

inline fdh::operator unspecified_bool_type() {
  return (_p && *_p) ? &fdh::_p : 0;
}

inline void fdh::clone(event<fdh> done) {
  if (*this)
    _p->clone(done);
  else
    done.trigger(*this);
}

inline void fdh::open(std::string fname, int flags, mode_t mode, event<int> fd) {
  if (*this)
    _p->open(fname, flags, mode, fd);
  else
    fd.trigger(-1);
}

inline void fdh::fstat(int fd, struct stat &stat_out, event<int> done) {
  if (*this)
    _p->fstat(fd, stat_out, done);
  else
    done.trigger(-EBADF);
}

inline void fdh::read(int fd, size_t size, event<int> fdo, event<> release) {
  if (*this)
    _p->read(fd, size, fdo, release);
  else
    fdo.trigger(-1); 
}

inline void fdh::write(int fd, size_t size, event<int> fdi, event<> release) {
  if (*this)
    _p->write(fd, size, fdi, release);
  else
    fdi.trigger(-1);
}

//TODO create seperate locked and free list?
//TODO add dynamic helper de/allocation?
class fdhlist {
  
  int _min;
  int _max;

  std::list<fdh> _fdhl;  
  std::list<fdh>::iterator _it;

  //not being used ...
  void increase();
  void decrease();
  //... yet

  class closure__increase;
  void increase(closure__increase &, unsigned);

  class closure__decrease;
  void decrease(closure__decrease &, unsigned);

public:
  fdhlist();
  fdh get();
};

inline fdhlist::fdhlist() 
: _min(FDH_MIN), _max(FDH_MAX) {
  for (int i = 0; i < _min; i++)
    _fdhl.push_back(fdh());
  _it = _fdhl.begin();
}

inline fdh fdhlist::get() {
  if (!_fdhl.size())
    return fdh(-1,-1);

  if (_it == _fdhl.end())
    _it = _fdhl.begin();
  
  return *(_it++);
}
}
#endif /* TAMER_FDH_HH */

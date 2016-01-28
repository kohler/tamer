#ifndef TAMER_FDH_HH
#define TAMER_FDH_HH 1
#include <tamer/tamer.hh>
#include <tamer/lock.hh>
#include <tamer/ref.hh>
#include <tamer/fd.hh>
#include <tamer/fdhmsg.hh>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <list>
namespace tamer {

class fdhelper { public:

    fdhelper()
        : _p(new fdhimp) {
    }

    void open(const std::string &fname, int flags, mode_t mode,
              const event<int> &fd) {
        _p->open(fname, flags, mode, fd);
    }
    void fstat(int fd, struct stat &stat_out, const event<int> &done) {
        _p->fstat(fd, stat_out, done);
    }
    void read(int fd, void *buf, size_t size, size_t &nread, const event<int> &done) {
        _p->read(fd, buf, size, nread, done);
    }
    void write(int fd, const void *buf, size_t size, size_t &nwritten, const event<int> &done) {
        _p->write(fd, buf, size, nwritten, done);
    }

  private:

    struct fdh {
        union {
            fdh_msg msg;
            char buf[PATH_MAX + FDH_MSG_SIZE];
        } _u;
        pid_t _pid;
        fd    _fd;

        fdh();
        bool ok() const {
            return _pid > 0 && _fd;
        }

        inline void send(int fd, size_t size, const event<int> &done) {
            _fd.sendmsg(_u.buf, size, fd, done);
        }
        void recv(int *fd, size_t size, event<int> done);

        class closure__recv__PikQi_; void recv(closure__recv__PikQi_ &);
    };

    struct fdhimp : public enable_ref_ptr {
        int _min;
        int _count;
        int _max;

        std::list<fdh *> _helpers;
        std::list<fdh *> _ready;
        std::list<event<> > _waiting;

        fdhimp();
        ~fdhimp();

        void get(event<fdh *> done);
        void put(fdh *h) {
            _ready.push_back(h);
            if (_waiting.size()) {
                _waiting.front().trigger();
                _waiting.pop_front();
            }
        }

        void open(std::string fname, int flags, mode_t mode, event<int> fd);
        void fstat(int fd, struct stat &stat_out, event<int> done);
        void read(int fd, void *buf, size_t size, size_t &nread, event<int> done);
        void write(int fd, const void *buf, size_t size, size_t &nwritten, event<int> done);

        class closure__get__QP3fdh_; void get(closure__get__QP3fdh_ &);
        class closure__open__Ssi6mode_tQi_; void open(closure__open__Ssi6mode_tQi_ &);
        class closure__fstat__iR4statQi_; void fstat(closure__fstat__iR4statQi_ &);
        class closure__read__iPvkRkQi_; void read(closure__read__iPvkRkQi_ &);
        class closure__write__iPKvkRkQi_; void write(closure__write__iPKvkRkQi_ &);
    };

    ref_ptr<fdhimp> _p;

};

}
#endif /* TAMER_FDH_HH */

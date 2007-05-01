#ifndef TAMER__DRIVER_HH
#define TAMER__DRIVER_HH 1
#include <tamer/event.hh>
#include <sys/time.h>
namespace tamer {

class driver { public:

    driver();
    ~driver();

    void initialize();
    
    inline void at_fd_read(int fd, const event<> &e);
    inline void at_fd_write(int fd, const event<> &e);
    
    inline void at_time(const timeval &expiry, const event<> &e);
    inline void at_asap(const event<> &e);
    inline void at_delay(timeval delay, const event<> &e);
    inline void at_delay_sec(int delay, const event<> &e);
    inline void at_delay_msec(int delay, const event<> &e);

    static void at_signal(int signal, const event<> &e);

    void once();
    void loop();

    timeval now;
    inline void set_now();

    static driver main;

  private:
    
    struct ttimer {
	union {
	    int schedpos;
	    ttimer *next;
	} u;
	timeval expiry;
	event<> trigger;
	ttimer(int schedpos_, const timeval &expiry_, const event<> &trigger_) : expiry(expiry_), trigger(trigger_) { u.schedpos = schedpos_; }
    };

    struct ttimer_group {
	ttimer_group *next;
	ttimer t[1];
    };
    
    ttimer **_t;
    int _nt;

    event<> *_fd;
    int _fdcap;
    int _nfds;

    event<> *_asap;
    unsigned _asap_head;
    unsigned _asap_tail;
    unsigned _asapcap;

    fd_set _readfds;
    fd_set _writefds;
    
    int _tcap;
    ttimer_group *_tgroup;
    ttimer *_tfree;

    void expand_timers();
    void timer_reheapify_from(int pos, ttimer *t, bool will_delete);
    void expand_fds(int fd);
    void expand_asap();
    void at_fd(int fd, bool write, const event<> &e);
    
};

inline void driver::set_now()
{
    gettimeofday(&now, 0);
}

inline void driver::at_time(const timeval &expiry, const event<> &e)
{
    if (!_tfree)
	expand_timers();
    ttimer *t = _tfree;
    _tfree = t->u.next;
    (void) new(static_cast<void *>(t)) ttimer(_nt, expiry, e);
    _t[_nt++] = 0;
    timer_reheapify_from(_nt - 1, t, false);
}

inline void driver::at_fd_read(int fd, const event<> &e)
{
    at_fd(fd, false, e);
}

inline void driver::at_fd_write(int fd, const event<> &e)
{
    at_fd(fd, true, e);
}

inline void driver::at_asap(const event<> &e)
{
    if (_asap_tail - _asap_head == _asapcap)
	expand_asap();
    (void) new((void *) &_asap[_asap_tail & (_asapcap - 1)]) event<>(e);
    _asap_tail++;
}

inline void driver::at_delay(timeval tv, const event<> &e)
{
    timeradd(&tv, &now, &tv);
    at_time(tv, e);
}

inline void driver::at_delay_sec(int delay, const event<> &e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay;
	tv.tv_usec = 0;
	at_delay(tv, e);
    }
}

inline void driver::at_delay_msec(int delay, const event<> &e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv;
	tv.tv_sec = delay / 1000;
	tv.tv_usec = (delay % 1000) * 1000;
	at_delay(tv, e);
    }
}

inline void initialize()
{
    driver::main.initialize();
}

inline struct timeval &now()
{
    return driver::main.now;
}

inline void set_now()
{
    driver::main.set_now();
}

inline void once()
{
    driver::main.once();
}

inline void loop()
{
    driver::main.loop();
}

inline void at_time(const timeval &expiry, const event<> &e)
{
    driver::main.at_time(expiry, e);
}

inline void at_asap(const event<> &e)
{
    driver::main.at_asap(e);
}

inline void at_delay(const timeval &delay, const event<> &e)
{
    driver::main.at_delay(delay, e);
}

inline void at_delay_sec(int delay, const event<> &e)
{
    driver::main.at_delay_sec(delay, e);
}

inline void at_delay_msec(int delay, const event<> &e)
{
    driver::main.at_delay_msec(delay, e);
}

inline void at_fd_read(int fd, const event<> &e)
{
    driver::main.at_fd_read(fd, e);
}

inline void at_fd_write(int fd, const event<> &e)
{
    driver::main.at_fd_write(fd, e);
}

inline void at_signal(int sig, const event<> &e)
{
    driver::at_signal(sig, e);
}

}
#endif /* TAMER__DRIVER_HH */

#ifndef TAMER_TAME_DRIVER_HH
#define TAMER_TAME_DRIVER_HH 1
#include "tame_event.hh"
#include <sys/time.h>
namespace tame {

class driver { public:

    driver();
    ~driver();

    inline void at_fd_read(int fd, const event<> &trigger);
    inline void at_fd_write(int fd, const event<> &trigger);
    
    inline void at_time(const timeval &expiry, const event<> &trigger);
    inline void at_asap(const event<> &trigger);
    inline void at_delay(timeval delay, const event<> &trigger);
    inline void at_delay(int delay_msec, const event<> &trigger);

    static void at_signal(int signal, const event<> &trigger);

    void once();

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
    void at_fd(int fd, bool write, const event<> &trigger);
    
};

inline void driver::set_now()
{
    gettimeofday(&now, 0);
}

inline void driver::at_time(const timeval &expiry, const event<> &trigger)
{
    if (!_tfree)
	expand_timers();
    ttimer *t = _tfree;
    _tfree = t->u.next;
    (void) new(static_cast<void *>(t)) ttimer(_nt, expiry, trigger);
    _t[_nt++] = 0;
    timer_reheapify_from(_nt - 1, t, false);
}

inline void driver::at_fd_read(int fd, const event<> &trigger)
{
    at_fd(fd, false, trigger);
}

inline void driver::at_fd_write(int fd, const event<> &trigger)
{
    at_fd(fd, true, trigger);
}

inline void driver::at_asap(const event<> &trigger)
{
    if (_asap_tail - _asap_head == _asapcap)
	expand_asap();
    _asap[_asap_tail & (_asapcap - 1)] = trigger;
    _asap_tail++;
}

inline void driver::at_delay(timeval tv, const event<> &trigger)
{
    timeradd(&tv, &now, &tv);
    at_time(tv, trigger);
}

inline void driver::at_delay(int delay_msec, const event<> &trigger)
{
    if (delay_msec <= 0)
	at_asap(trigger);
    else {
	timeval tv;
	tv.tv_sec = delay_msec / 1000;
	tv.tv_usec = (delay_msec % 1000) * 1000;
	at_delay(tv, trigger);
    }
}

inline void at_time(const timeval &expiry, const event<> &trigger)
{
    driver::main.at_time(expiry, trigger);
}

inline void at_asap(const event<> &trigger)
{
    driver::main.at_asap(trigger);
}

inline void at_delay(const timeval &delay, const event<> &trigger)
{
    driver::main.at_delay(delay, trigger);
}

inline void at_delay(int delay_msec, const event<> &trigger)
{
    driver::main.at_delay(delay_msec, trigger);
}

inline void at_fd_read(int fd, const event<> &trigger)
{
    driver::main.at_fd_read(fd, trigger);
}

inline void at_fd_write(int fd, const event<> &trigger)
{
    driver::main.at_fd_write(fd, trigger);
}

inline void at_signal(int sig, const event<> &trigger)
{
    driver::at_signal(sig, trigger);
}

}
#endif /* TAMER_TAME_DRIVER_HH */

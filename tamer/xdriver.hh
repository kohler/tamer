#ifndef TAMER_XDRIVER_HH
#define TAMER_XDRIVER_HH 1
#include <tamer/event.hh>
#include <sys/time.h>
#include <signal.h>
namespace tamer {

class driver { public:

    inline driver();
    virtual inline ~driver();

    // basic functions
    enum { fdread = 0, fdwrite = 1 }; // the order is important
    virtual void at_fd(int fd, int action, const event<> &e) = 0;
    virtual void at_time(const timeval &expiry, const event<> &e) = 0;
    virtual void at_asap(const event<> &e);
    virtual void kill_fd(int fd);

    inline void at_fd_read(int fd, const event<> &e);
    inline void at_fd_write(int fd, const event<> &e);

    inline void at_delay(timeval delay, const event<> &e);
    void at_delay(double delay, const event<> &e);
    inline void at_delay_sec(int delay, const event<> &e);
    inline void at_delay_msec(int delay, const event<> &e);
    
    static void at_signal(int signal, const event<> &e);

    timeval now;
    inline void set_now();

    virtual bool empty() = 0;
    virtual void once() = 0;
    virtual void loop() = 0;

    static driver *make_tamer();
    static driver *make_libevent();
    
    static driver *main;

    static volatile sig_atomic_t sig_any_active;
    static int sig_pipe[2];
    static void dispatch_signals();
    
};

inline driver::driver()
{
}

inline driver::~driver()
{
}

inline void driver::at_asap(const event<> &e)
{
    at_time(now, e);
}

inline void driver::kill_fd(int)
{
}

inline void driver::set_now()
{
    gettimeofday(&now, 0);
}

inline void driver::at_fd_read(int fd, const event<> &e)
{
    at_fd(fd, fdread, e);
}

inline void driver::at_fd_write(int fd, const event<> &e)
{
    at_fd(fd, fdwrite, e);
}

inline void driver::at_delay(timeval delay, const event<> &e)
{
    timeradd(&delay, &now, &delay);
    at_time(delay, e);
}

inline void driver::at_delay_sec(int delay, const event<> &e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now;
	tv.tv_sec += delay;
	at_time(tv, e);
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

}
#endif /* TAMER_XDRIVER_HH */

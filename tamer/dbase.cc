#include <tamer/tamer.hh>
#include <sys/select.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

namespace tamer {

driver *driver::main;

void initialize()
{
    if (!driver::main && !getenv("TAMER_NOLIBEVENT"))
	driver::main = driver::make_libevent();
    if (!driver::main)
	driver::main = driver::make_tamer();
}

void driver::at_delay(double delay, const event<> &e)
{
    if (delay <= 0)
	at_asap(e);
    else {
	timeval tv = now;
	long ldelay = (long) delay;
	tv.tv_sec += ldelay;
	tv.tv_usec += (long) ((delay - ldelay) * 1000000 + 0.5);
	if (tv.tv_usec >= 1000000) {
	    tv.tv_sec++;
	    tv.tv_usec -= 1000000;
	}
	at_time(tv, e);
    }
}

}

#ifndef TAMER_DRIVER_HH
#define TAMER_DRIVER_HH 1
#include <tamer/xdriver.hh>
namespace tamer {

/** @file <tamer/driver.hh>
 *  @brief  Functions for registering primitive events and managing the event
 *  loop.
 */

/** @brief  Initializes the Tamer event loop.
 *
 *  Must be called at least once before any primitive Tamer events are
 *  registered.
 */
void initialize();

/** @brief  Fetches Tamer's current time.
 *  @return  Current timestamp.
 */
inline timeval &now()
{
    return driver::main->now;
}

/** @brief  Sets Tamer's current time to the current timestamp.
 */
inline void set_now()
{
    driver::main->set_now();
}

/** @brief  Test whether driver events are pending.
 *  @return  True if no driver events are pending, otherwise false.
 *
 *  If @c driver_empty() is true, then @c once() will hang forever.
 */
inline bool driver_empty()
{
    return driver::main->empty();
}

/** @brief  Run driver loop once. */
inline void once()
{
    driver::main->once();
}

/** @brief  Run driver loop indefinitely. */
inline void loop()
{
    driver::main->loop();
}

/** @brief  Register event for file descriptor readability.
 *  @param  fd  File descriptor.
 *  @param  e   Event.
 *
 *  Triggers @a e when @a fd becomes readable.  Cancels @a e when @a fd is
 *  closed.
 */
inline void at_fd_read(int fd, const event<> &e)
{
    driver::main->at_fd_read(fd, e);
}

/** @brief  Register event for file descriptor writability.
 *  @param  fd  File descriptor.
 *  @param  e   Event.
 *
 *  Triggers @a e when @a fd becomes writable.  Cancels @a e when @a fd is
 *  closed.
 */
inline void at_fd_write(int fd, const event<> &e)
{
    driver::main->at_fd_write(fd, e);
}

/** @brief  Register event for a given time.
 *  @param  expiry  Time.
 *  @param  e       Event.
 *
 *  Triggers @a e at timestamp @a expiry, or soon afterwards.
 */
inline void at_time(const timeval &expiry, const event<> &e)
{
    driver::main->at_time(expiry, e);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c now(), or soon
 *  afterwards.
 */
inline void at_delay(const timeval &delay, const event<> &e)
{
    driver::main->at_delay(delay, e);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c now(), or soon
 *  afterwards.
 */
inline void at_delay(double delay, const event<> &e)
{
    driver::main->at_delay(delay, e);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay seconds have elapsed since @c now(), or soon
 *  afterwards.
 */
inline void at_delay_sec(int delay, const event<> &e)
{
    driver::main->at_delay_sec(delay, e);
}

/** @brief  Register event for a given delay.
 *  @param  delay  Delay time in milliseconds.
 *  @param  e      Event.
 *
 *  Triggers @a e when @a delay milliseconds have elapsed since @c now(), or
 *  soon afterwards.
 */
inline void at_delay_msec(int delay, const event<> &e)
{
    driver::main->at_delay_msec(delay, e);
}

/** @brief  Register event for signal occurrence.
 *  @param  sig  Signal number.
 *  @param  e    Event.
 *
 *  Triggers @a e soon after @a sig is received.  The signal @a sig remains
 *  blocked until any code waiting on @a e has run.
 */
inline void at_signal(int sig, const event<> &e)
{
    driver::at_signal(sig, e);
}

/** @brief  Register event to trigger soon.
 *  @param  e  Event.
 *
 *  Triggers @a e the next time through the driver loop.
 */
inline void at_asap(const event<> &e)
{
    driver::main->at_asap(e);
}

}
#endif /* TAMER_DRIVER_HH */

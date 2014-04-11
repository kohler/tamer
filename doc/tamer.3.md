tamer(3) - event-driven programming support for C++

## SYNOPSIS

    #include <tamer/tamer.hh>
    using namespace tamer;
    
    class event<> { public:
        event();
        operator bool() const;
        bool empty() const;
        void operator()();
        void trigger();   // synonym for operator()()
        void at_trigger(const event<>& e);
        event<>& operator+=(event<> e);
    }
    
    template <typename T>
    class event<T> { public:
        event();
        operator bool() const;
        bool empty() const;
        void operator()(T v);
        void trigger(T v);
        void at_trigger(const event<>& e);
        void unblock();
        event<> unblocker() const;
        event<T>& operator+=(event<T> e);
    }
    
    tamed void example(event<> done) {
        tvars { ... closure variable declarations; ... }
        twait { ... at_delay(5, make_event()); ... }
        done();
    }

## DESCRIPTION

`Tamer` provides language extensions and libraries that simplify
C++ event-driven programming. This page describes the Tamer
abstractions and some of the user-accessible methods and functions in
the Tamer library. Most Tamer programs will also use the tamer(1)
preprocessor, which converts programs with `tamed` functions into
conventional C++.

## OVERVIEW

Tamer programming uses two main concepts, **events** and **tamed
functions**. An event is an object representing a future occurrence. A
tamed function is a function that can block until one or more events
complete.

The following tamed function blocks for 5 seconds (during which other
computation can proceed), then prints "Done" to standard output:

    tamed void done_after_5sec(event<> done) {
        twait { tamer::at_delay(5, make_event()); }
        std::cout << "Done\n";
        done();
    }

How does this work?

*   The `tamed` keyword marks a function as tamed.

*   The `twait` keyword marks a code region that can block. `twait` can
    only appear in tamed functions.

*   The call to `make_event()` creates an `event<>` object bound to
    the `twait` region.

*   The call to `tamer::at_delay()` tells the Tamer library to register
    a timer that'll go off in 5 seconds. The event is triggered when
    the timer expires. Tamer understands several kinds of primitive
    occurrence, including file descriptor events, signals, and timers;
    users build up more complex occurrences from these primitives.
    
*   The `twait` region executes as normal until the final right brace.
    Then it blocks until all events bound to the region have
    triggered. Here, there's one bound event, which will trigger in 5
    seconds.

*   When `done_after_5sec` blocks, it appears to return to its caller.
    However, its state, arguments, and blocking point are preserved in
    a heap **closure**. When the relevant events trigger, the function
    will pick up where it left off.

*   After 5 seconds, the event is triggered. This resumes the tamed
    function, which prints to standard output.

*   Finally, the tamed function signals its actual completion by
    triggering the `done` event, which was passed as an argument. An
    event is used to indicate completion because tamed functions
    typically return to their callers before they complete.

## EVENT VALUES

Events can pass values when they are triggered. Triggering an
`event<>` simply indicates that the event has happened, but triggering
an `event<T>` will pass a value of type `T` back to the event's
creator. For instance, the following code blocks for 5 seconds, then
triggers the `done` event, passing it the current time as a double:

    tamed void done_after_5sec(event<double> done) {
        twait { tamer::at_delay(5, make_event()); }
        done(tamer::dnow());
    }

That might be used in the following context, which tests how long it
takes for a function to unblock:

    tamed unblock_test(event<> done) {
        tamed { double unblock_time; }
        twait { done_after_5sec(make_event(unblock_time)); }
        std::cout << "Unblocked at time " << unblock_time << "\n"
                  << "It is now " << tamer::dnow() << "\n";
        done();
    }

How does this work?

*   The `done_after_5sec` tamed function now takes an `event<double>`
    argument. To trigger such an event, you must supply a `double`.

*   The `unblock_test` tamed function uses the `tamed` keyword to
    mark declarations of **closure variables**, which
    are local variables that survive across `twait` regions. Here
    there is one such variable, `unblock_time`.

    (Normal local variables are stored on the stack, rather than in
    the closure. Their values are reset whenever a tamed function
    blocks.)

*   The call to `make_event(unblock_time)` creates an `event<double>`
    rather than an `event<>`. This event contains a reference to the
    `unblock_time` closure variable. When the event is triggered, the
    supplied double value will be stored there.

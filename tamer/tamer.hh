#ifndef TAMER_TAMER_HH
#define TAMER_TAMER_HH 1

/** @mainpage Tamer
 *  @section  Introduction
 *
 *  Tamer is a C++ language extension and library that simplifies event-driven
 *  programming.  Functions in Tamer can block to wait for an event, then
 *  resume computation later.  While one function is blocked, other functions
 *  can continue their computation.  This is as easy as threaded programming,
 *  but avoids most synchronization and locking issues and generally requires
 *  less memory.
 *
 *  Most documented Tamer functions and classes can be found under the
 *  "Namespaces" and "Classes" tabs.
 *
 *  <a href='http://www.read.cs.ucla.edu/tamer/'>Main Tamer page</a>
 */

/** @file <tamer/tamer.hh>
 *  @brief  The main Tamer header file.
 */

/** @namespace tamer
 *  @brief  Namespace containing public Tamer classes and functions for the
 *  Tamer core.
 */

#include <tamer/rendezvous.hh>
#include <tamer/event.hh>
#include <tamer/driver.hh>
#include <tamer/adapter.hh>

#endif /* TAMER_TAMER_HH */

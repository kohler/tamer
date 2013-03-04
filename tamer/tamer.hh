#ifndef TAMER_TAMER_HH
#define TAMER_TAMER_HH 1
/* Copyright (c) 2007-2013, Eddie Kohler
 * Copyright (c) 2007, Regents of the University of California
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Tamer LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Tamer LICENSE file; the license in that file is
 * legally binding.
 */

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

/** @brief Major version number for this Tamer release. */
#define TAMER_MAJOR_VERSION   1
/** @brief Minor version number for this Tamer release. */
#define TAMER_MINOR_VERSION   5
/** @brief Release level for this Tamer release. */
#define TAMER_RELEASE_LEVEL   0

#include <tamer/rendezvous.hh>
#include <tamer/event.hh>
#include <tamer/driver.hh>
#include <tamer/adapter.hh>

#endif /* TAMER_TAMER_HH */

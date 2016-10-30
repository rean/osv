/*
 * Copyright (C) 2015 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#ifndef PTHREAD_HH_
#define PTHREAD_HH_

#ifdef __cplusplus
extern "C" {
#endif

// Linux's <time.h> defines 9 types of clocks. We reserve space for 16 slots
// and use the clock ids afterwards for per-thread clocks. This is OSv-
// specific, and an application doesn't need to know about it - only
// pthread_getcpuclockid() and clock_gettime() need to know about this.
#define _OSV_CLOCK_SLOTS 16

#ifdef __cplusplus
}

namespace pthread_private {
void run_tsd_dtors();
}
#endif

int pthread_barrier_init(pthread_barrier_t *__restrict,
                         const pthread_barrierattr_t *__restrict,
                         unsigned);
int pthread_barrier_destroy(pthread_barrier_t *);
int pthread_barrier_wait(pthread_barrier_t *);

#endif /* PTHREAD_HH_ */

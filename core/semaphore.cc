/*
 * Copyright (C) 2013 Cloudius Systems, Ltd.
 *
 * This work is open source software, licensed under the terms of the
 * BSD license as described in the LICENSE file in the top-level directory.
 */

#include <osv/semaphore.hh>
#include <osv/trace.hh>

TRACEPOINT(trace_sem_waiter_save, "sem: %p, curr-name: %s (thread: %p) wr addr: %p, #waiters: %d (val: %d)", semaphore *, const char *, sched::thread *, semaphore::wait_record *, int, unsigned);
TRACEPOINT(trace_sem_waiter_cleanup, "sem: %p, curr-name: %s (thread: %p) wr addr: %p, #waiters: %d (val: %d)", semaphore *, const char *, sched::thread *, semaphore::wait_record *, int, unsigned);
TRACEPOINT(trace_sem_waiter_scan, "sem: %p, curr-name: %s (thread: %p) wr addr: %p, #waiters: %d (val: %d)", semaphore *, const char *, sched::thread *, semaphore::wait_record *, int, unsigned);
TRACEPOINT(trace_sem_waiter_leave, "sem: %p, curr-name: %s (thread: %p) wr addr: %p, #waiters: %d (val: %d)", semaphore *, const char *, sched::thread *, semaphore::wait_record *, int, unsigned);

semaphore::semaphore(unsigned val)
    : _val(val)
{
}

void semaphore::post_unlocked(unsigned units)
{
    _val += units;
    auto i = _waiters.begin();
    while (_val > 0 && i != _waiters.end()) {
        auto wr = i++;
        if (wr->units <= _val) {
            _val -= wr->units;
            wr->owner->wake();
            wr->owner = nullptr;
            _waiters.erase(wr);
        }
    }
}

bool semaphore::wait(unsigned units, sched::timer* tmr)
{
    wait_record wr;
    wr.owner = nullptr;
    sched::thread* current = sched::thread::current();
    std::string current_name = "unknown(0)";
    if (current) {
       current_name = current->name();
    }

    std::lock_guard<mutex> guard(_mtx);

    if (_val >= units) {
        _val -= units;
        return true;
    } else {
        wr.owner = sched::thread::current();
        wr.units = units;
        trace_sem_waiter_save(this, current_name.c_str(),
                              sched::thread::current(), &wr,
                              (int) _waiters.size(), _val);
        _waiters.push_back(wr);
    }

    sched::thread::wait_until(_mtx,
            [&] { return (tmr && tmr->expired()) || !wr.owner; });

    // if wr.owner, it's a timeout - post() didn't wake us and didn't decrease
    // the semaphore's value for us. Note we are holding the mutex, so there
    // can be no race with post(). To clean up we should remove the
    // wait record (local variable) that we just pushed onto _waiters
    // (via push_back)
    if (wr.owner) {
       _waiters.erase(_waiters.iterator_to(wr));
    }

    return !wr.owner;
 }

bool semaphore::trywait(unsigned units)
{
    bool ok = false;
    WITH_LOCK(_mtx) {
        if (_val > units) {
            _val -= units;
            ok = true;
        }
    }

    return ok;
}





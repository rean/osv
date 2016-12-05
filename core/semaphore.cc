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

    // If wr.owner is not nullptr then there was a timeout (post() did not
    // wake us). In that case, just remove the wait_record (local variable)
    // that we just pushed onto _waiters (via push_back)
    if (wr.owner) {
       auto i = _waiters.begin();
       // _val can be 0 right now, so we can't use the same condition that
       // post_unlocked uses "_val > 0 && i != _waiters.end()"
       while (i != _waiters.end()) {
          trace_sem_waiter_scan(this, current_name.c_str(),
                                sched::thread::current(), &wr,
                                (int) _waiters.size(), _val);

          auto wait_rec = i++;
          if (wait_rec->owner == sched::thread::current()) {
             trace_sem_waiter_cleanup(this, current_name.c_str(),
                                      sched::thread::current(), &wr,
                                      (int) _waiters.size(), _val);
             // We found our wait record so remove it from the list.
             // There is no need to wake ourselves or set wr.owner = nullptr
             // we want to remember that we timed out (if post happened then
             // wr.owner would have been set to nullptr)
             if (wait_rec->units <= _val) {
                _val -= wait_rec->units;
             }
             // Remove our wait_record
             _waiters.erase(wait_rec);
          }
       }
    }
    // Check whether the _waiters list is empty
    trace_sem_waiter_leave(this, current_name.c_str(),
                           sched::thread::current(), &wr,
                           (int) _waiters.size(), _val);

    // if wr.owner, it's a timeout - post() didn't wake us and didn't decrease
    // the semaphore's value for us. Note we are holding the mutex, so there
    // can be no race with post().
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





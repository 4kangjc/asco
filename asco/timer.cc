#include "timer.h"
#include "log.h"
#include "async_io.h"

namespace asco {

static auto g_logger = ASCO_LOG_NAME("system");

bool Timer::Comparactor::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    long long l = lhs->ts_.tv_sec * 1000 + lhs->ts_.tv_nsec;
    long long r = rhs->ts_.tv_sec * 1000 + rhs->ts_.tv_nsec;
    if (l != r) {
        return l < r;
    }
    return lhs.get() < rhs.get();
}

Timer::Timer(long long ms, __task&& cb, bool recurring, IOManager* manager)
    : ts_{.tv_sec = ms / 1000, .tv_nsec = ms % 1000}, cb_(std::move(cb)),
    manager_(manager), recurring_(recurring) {
}

bool Timer::isRecurring() const {
    LOCK_GUARD(manager_->timer_mutex_);
    return recurring_;
}

void Timer::setRecurring(bool v) {
    LOCK_GUARD(manager_->timer_mutex_);
    recurring_ = v;
}

const __task& Timer::getCb() const { 
    LOCK_GUARD(manager_->timer_mutex_);
    return cb_; 
}

__kernel_timespec* Timer::getTs() { 
    LOCK_GUARD(manager_->timer_mutex_);
    return &ts_; 
}

void Timer::cacel() {
    LOCK_GUARD(manager_->timer_mutex_);
    ts_.tv_nsec = ts_.tv_sec = 0;
    cb_ = nullptr;
    recurring_ = false;
}

void Timer::reset(long long ms, __task&& cb) {
    LOCK_GUARD(manager_->timer_mutex_);
    ts_ = {.tv_sec = ms / 1000, .tv_nsec = ms % 1000 };
    if (cb) {
        cb_ = std::move(cb);
    }
}



}
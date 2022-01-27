#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace asco {

static auto g_logger = ASCO_LOG_NAME("system");
static thread_local Scheduler* t_scheduler = nullptr;           // 线程所属的协程调度器

Scheduler::Scheduler(size_t threads, bool use_caller, std::string_view name) : name_(name), idle_([this]() -> Coroutine {
    ASCO_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        co_await Coroutine::yield{};
    }
}) {
    ASCO_ASSERT(threads > 0);

    if (use_caller) {
        --threads;

        ASCO_ASSERT(t_scheduler == nullptr);
        t_scheduler = this;

        Thread::SetName(name);

        rootThreadId_ = GetThreadId();
        threadIds_.push_back(rootThreadId_);
    } else {
        rootThreadId_ = -1;
    }
    threadCount_ = threads;

    tickle_ = []() {
        ASCO_LOG_INFO(g_logger) << "tickle";
    };
}

void Scheduler::schedule(Coroutine&& co) {
    bool need_tickle = false;
    {
        LOCK_GUARD(mutex_);
        need_tickle = tasks_.empty();
        tasks_.emplace_back(std::make_shared<Coroutine>(std::move(co)));
    }
    if (need_tickle) {
        tickle_();
    }
}

void Scheduler::schedule(const Coroutine::ptr& co) {
    bool need_tickle = false;
    {
        LOCK_GUARD(mutex_);
        need_tickle = tasks_.empty();
        tasks_.push_back(co);
    }
    if (need_tickle) {
        tickle_();
    }
}

void Scheduler::schedule(Coroutine::ptr&& co) {
    bool need_tickle = false;
    {
        LOCK_GUARD(mutex_);
        need_tickle = tasks_.empty();
        tasks_.push_back(std::move(co));
    }
    if (need_tickle) {
        tickle_();
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::start() {
    LOCK_GUARD(mutex_);
    if (!stopping_) {
        return;
    }
    stopping_ = false;
    ASCO_ASSERT(threads_.empty());
    threads_.reserve(threadCount_);
    for (size_t i = 0; i < threadCount_; ++i) {
        threads_.emplace_back(std::make_shared<Thread>(name_ + "_" + std::to_string(i), 
            &Scheduler::run, this));
        threadIds_.push_back(threads_[i]->getId());
    }
}

void Scheduler::stop() {
    stopping_ = true;

    if (threadCount_ == 0) {
        if (stopping()) {
            return;
        }
    }

    if (rootThreadId_ != -1) {
        ASCO_ASSERT(GetThis() == this);
    } else {
        ASCO_ASSERT(GetThis() != this);     
    }

    for (size_t i = 0; i < threadCount_; ++i) {
        tickle_();
    }

    if (rootThreadId_ != -1) {
        if (!stopping()) {
            run();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        LOCK_GUARD(mutex_);
        thrs.swap(threads_);
    }
    for (auto& thr : thrs) {
        thr->join();
    }
}

void Scheduler::run() {
    ASCO_LOG_DEBUG(g_logger) << name_ << " run";
    setThis();
    Coroutine idle_coro(idle_());

    Coroutine::ptr co;
    while (true) {
        bool tickle_me = false;
        {
            LOCK_GUARD(mutex_);
            if (!tasks_.empty()) {
                co = std::move(tasks_.front());
                tasks_.pop_front();
                ++activeThreadCount_;
                tickle_me = true;
            }
        }


        if (tickle_me) {
            tickle_();
        }

        if (co) {
            ASCO_ASSERT(co->getState() == Coroutine::READY || co->getState() == Coroutine::YIELD);
            co->resume();
            --activeThreadCount_;
            co = nullptr;
        } else {
            if (idle_coro.getState() == Coroutine::FINISH) {
                ASCO_LOG_INFO(g_logger) << "idle coroutine finish!";
                break;
            }
            ++idleThreadCount_;
            idle_coro.resume();
            --idleThreadCount_;
        }

    }
}

Scheduler::~Scheduler() {
    ASCO_ASSERT(stopping_);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

bool Scheduler::stopping() {
    LOCK_GUARD(mutex_);
    return stopping_ && tasks_.empty() && activeThreadCount_ == 0;
}

}
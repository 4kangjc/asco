#include "coroutine.h"
#include "log.h"
#include "macro.h"
#include <stack>

namespace asco {

thread_local std::stack<Coroutine*> s_stk;

static std::atomic<int> s_count;

static auto g_logger = ASCO_LOG_NAME("system");

void Coroutine::yield::await_suspend(std::coroutine_handle<> ) {
    s_stk.top()->state_ = Coroutine::YIELD;
    s_stk.pop();
}

std::suspend_always Coroutine::Promise::initial_suspend() {
    ++s_count;
    return {};
}

std::suspend_always Coroutine::Promise::final_suspend() noexcept {
    s_stk.top()->state_ = Coroutine::FINISH;
    s_stk.pop();
    return {};
}

void Coroutine::Promise::unhandled_exception() {
    s_stk.top()->state_ = Coroutine::EXCEPT;
    ASCO_LOG_ERROR(g_logger) << "exception";
}

Coroutine::Coroutine(Coroutine&& rhs) : state_(rhs.state_), handle_(std::move(rhs.handle_)),
                data_(std::move(rhs.data_)) {
    rhs.handle_ = nullptr;
    rhs.data_ = nullptr;
}

Coroutine& Coroutine::operator=(Coroutine&& rhs) {
    state_ = rhs.state_;
    handle_ = std::move(rhs.handle_);
    data_ = std::move(rhs.data_);
    rhs.handle_ = nullptr;
    rhs.data_ = nullptr;
    return *this;
}

Coroutine::ptr Coroutine::GetThis() {
    if (s_stk.empty()) {
        return nullptr;
    }
    return s_stk.top()->shared_from_this();
}

Coroutine* Coroutine::__Get__This() {
    if (s_stk.empty()) {
        return nullptr;
    }
    return s_stk.top();
}

void Coroutine::resume() {
    state_ = Coroutine::EXEC;
    s_stk.push(this);
    handle_.resume();
}

void Coroutine::reset() {
    ASCO_ASSERT2(state_ == Coroutine::FINISH, "state is not finish");
    ASCO_ASSERT2(handle_.done(), "hansle is not done");
    handle_.destroy();
    handle_ = nullptr;
    state_ = Coroutine::READY;
}

Coroutine::~Coroutine() {
    if (!handle_) {
        return;
    }
    ASCO_ASSERT2(handle_.done(), "handle is not done");
    handle_.destroy();
    --s_count;
}

int Coroutine::TotalCoroutines() {
    return s_count;
}


} // namespace asco
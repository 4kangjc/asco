#pragma once

#include <coroutine>
#include <any>
#include <memory>
#include "noncopyable.h"

namespace asco {

class Coroutine : noncopyable, public std::enable_shared_from_this<Coroutine> {
public:
    using ptr = std::shared_ptr<Coroutine>;
    enum State {
        READY,
        EXEC,
        YIELD,
        FINISH,
        EXCEPT
    };

    struct yield {
        bool await_ready() { return false; }
        void await_suspend(std::coroutine_handle<> h);
        void await_resume() { }   
    };

    struct Promise {
        auto get_return_object() { return std::coroutine_handle<Promise>::from_promise(*this); }
        std::suspend_always initial_suspend();
        std::suspend_always final_suspend() noexcept;

        template <class T>
        void return_value(T&& value) {
            auto self = __Get__This();
            if (self) {
                self->data_ = std::forward<T>(value);
            }
        }

        void unhandled_exception();
    };

    using promise_type = Promise;
    using Handle = std::coroutine_handle<promise_type>;

    template <class T>
    T get_result() {
        try {
            return std::any_cast<T>(data_);
        } catch (...) {

        }
        return T();
    }

    Coroutine(Handle h) : handle_(h) {}
    Coroutine() = default;
    Coroutine(Coroutine&& rhs);
    Coroutine& operator=(Coroutine&& rhs);

    static Coroutine::ptr GetThis();
    void resume();
    State getState() { return state_; }
    operator bool() { return handle_ != nullptr; }
    void reset();
    ~Coroutine();
    static int TotalCoroutines();
private:
    static Coroutine* __Get__This();
private:
    State state_ = Coroutine::READY;
    Handle handle_ = nullptr;
    std::any data_;
};

}
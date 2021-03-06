#pragma once

#include <functional>
#include <memory>

namespace asco {

class __task {
public:
    template <typename F, typename... A>
    __task(F&& func, A&&... args) : cb_(std::bind(std::forward<F>(func), std::forward<A>(args)...)) {}
    template <typename F>
    __task(F&& func) : cb_(std::forward<F>(func)) { }
    __task() = default;
    __task(__task&& other) noexcept : cb_(std::move(other.cb_)) { }
    __task(__task& other) : cb_(other.cb_) {}
    __task(const __task& other) : cb_(other.cb_) {}
    ~__task() = default;
    __task& operator=(const __task& cb) { cb_ = cb.cb_; return *this; }
    __task& operator=(__task&& cb) { cb_ = std::move(cb.cb_); return *this; }
    void operator()() { return cb_(); }
    operator bool() { return cb_ != nullptr; }
    operator std::function<void()>() const { return cb_; }
    auto& get() { return cb_; }
    void swap(__task& cb) noexcept { cb_.swap(cb.cb_); }
private:
    std::function<void()> cb_;
};

}
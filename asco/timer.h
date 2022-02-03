#pragma once

#include <linux/time_types.h>
#include <memory>
#include "task.h"
#include "mutex.h"

namespace asco {

class IOManager;

class Timer {
friend class IOManager;
public:
    using ptr = std::shared_ptr<Timer>;
    // 取消定时器
    void cancel();
    // 
    template <class... Args>
    void reset(long long ms, Args&&... args) {
        return reset(ms, __task(std::forward<Args>(args)...));
    }
    // 重置定时器时间和函数
    void reset(long long ms, __task&& cb = nullptr);
    // 是否是循环定时器
    bool isRecurring() const;
    // 设置为循环定时器
    void setRecurring(bool v = true);
    // 获得定时器执行函数
    const __task& getCb() const;
    // 获得定时器时间
    __kernel_timespec* getTs();
private:
    Timer(long long ms, __task&& cb, bool recurring, IOManager* manager);
private:
    __kernel_timespec ts_;
    __task cb_;
    IOManager* manager_ = nullptr;
    bool recurring_;
private:
    struct Comparactor {
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

}
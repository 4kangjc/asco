#pragma once

#include "coroutine.h"
#include "scheduler.h"
#include "timer.h"
#include <liburing.h>
// #include <stdint.h>

namespace asco {

struct resume_resolver {
    friend struct Awaiter_io;

    void resolve(int result) noexcept {
        this->result = result;
        // coro->resume();
        Scheduler::GetThis()->schedule(coro);
    }
private:
    Coroutine::ptr coro;
    // std::coroutine_handle<> handle;
    int result = 0;
};


class Awaiter_io : public Coroutine::yield {
public:
    int await_resume() { return resolver.result; }
    void await_suspend(std::coroutine_handle<> handle) {
        resolver.coro = Coroutine::GetThis();
        Coroutine::yield::await_suspend(handle);
        // resolver.handle = handle;
        io_uring_sqe_set_data(sqe, &resolver);
    }
    Awaiter_io(io_uring_sqe* rhs) : sqe(rhs) {}
private:
    io_uring_sqe* sqe;
    resume_resolver resolver;
};

Awaiter_io async_read(int fd, void* buf, unsigned long nbyte, unsigned long long offset = 0);
Awaiter_io async_write(int fd, const void* buf, unsigned long nbyte, unsigned long long offset = 0);
Awaiter_io async_readv(int fd, struct iovec* iov, unsigned int nr_vecs, unsigned long long offset = 0);
Awaiter_io async_writev(int fd, const struct iovec* iov, unsigned int nr_vecs, unsigned long long offset = 0);
Awaiter_io async_read_fixed(int fd, void* buf, unsigned long nbytes, unsigned long long offset = 0, int buf_index = 0);
Awaiter_io async_write_fixed(int fd, const void* buf, unsigned long nbytes, unsigned long long offset = 0, int buf_index = 0);
Awaiter_io async_recvmsg(int sockfd, msghdr* msg, uint32_t flags);
Awaiter_io async_sendmsg(int sockfd, const msghdr* msg, uint32_t flags);
Awaiter_io async_recv(int sockfd, void* buf, size_t length, int flags);
Awaiter_io async_send(int sockfd, const void* buf, size_t length, int flags);
Awaiter_io async_connect(int fd, sockaddr* addr, socklen_t addrlen);
Awaiter_io async_accept(int fd, sockaddr* addr, socklen_t* addrlen, int flags = 0);
Awaiter_io async_close(int fd);
Awaiter_io async_timeout(__kernel_timespec* ts);
Awaiter_io async_sleep(long long sec);
Awaiter_io async_usleep(long long usec);

class IOManager : public Scheduler {
friend class Timer;
public:
    IOManager(size_t threads = 1, bool use_caller = true, std::string_view name = "");
    ~IOManager();
    bool stopping() override;

    static IOManager* GetThis();

    template <typename... Args>
    Timer::ptr addTimer(uint64_t ms, bool recurring, Args&&... args) {
        Timer::ptr timer(new Timer(ms, __task(std::forward<Args>(args)...), recurring, this));
        schedule([](Timer::ptr timer) -> Coroutine {
            do {
               co_await async_timeout(timer->getTs());
                auto func = timer->getCb();     // copy function
                if (func) {
                    func();
                } 
            } while (timer->isRecurring());
        }(timer));
        return timer;
    }

public:
    std::atomic<size_t> pendingEventCount_ = {0};                          // 当前等待执行的事件数量
private:
    mutable mutex timer_mutex_;                                           // 定时器锁
private:
    int tickleFds_[2];
};

void register_buffers(const struct iovec *iovecs, unsigned nr_iovecs);

}
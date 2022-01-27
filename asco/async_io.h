#pragma once

#include "coroutine.h"
#include "scheduler.h"
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

class IOManager : public Scheduler {
public:
    IOManager(size_t threads = 1, bool use_caller = true, std::string_view name = "");
    ~IOManager();
    bool stopping() override;

    static IOManager* GetThis();
public:
    std::atomic<size_t> pendingEventCount_ = {0};               // 当前等待执行的事件数量
private:
    int tickleFds_[2];
};

Awaiter_io async_read(int fd, void* buf, unsigned long nbyte, unsigned long long offset = 0);
Awaiter_io async_write(int fd, const void* buf, unsigned long nbyte, unsigned long long offset = 0);
Awaiter_io async_readv(int fd, struct iovec* iov, unsigned int nr_vecs, unsigned long long offset = 0);
Awaiter_io async_writev(int fd, const struct iovec* iov, unsigned int nr_vecs, unsigned long long offset = 0);
Awaiter_io async_recvmsg(int sockfd, msghdr* msg, uint32_t flags);
Awaiter_io async_sendmsg(int sockfd, const msghdr* msg, uint32_t flags);
Awaiter_io async_recv(int sockfd, void* buf, size_t length, int flags);
Awaiter_io async_send(int sockfd, const void* buf, size_t length, int flags);
Awaiter_io async_connect(int fd, sockaddr* addr, socklen_t addrlen);
Awaiter_io async_accept(int fd, sockaddr* addr, socklen_t* addrlen, int flags = 0);
Awaiter_io async_close(int fd);
Awaiter_io async_timeout(__kernel_timespec* ts);

}
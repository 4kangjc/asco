#include "async_io.h"
#include "log.h"
#include "macro.h"
#include <liburing.h>

namespace asco {

static auto g_logger = ASCO_LOG_NAME("system");

struct __io__uring__ {
    io_uring ring;

    __io__uring__(int entries = 64, uint32_t flags = 0, uint32_t wq_fd = 0) {
        io_uring_params p = {
            .flags = flags,
            .wq_fd = wq_fd
        };

        io_uring_queue_init_params(entries, &ring, &p);
    }
    ~__io__uring__() {
        io_uring_queue_exit(&ring);
    }
};

thread_local __io__uring__ s_uring;

io_uring_sqe* io_uring_get_sqe_safe() noexcept {
    auto sqe = io_uring_get_sqe(&s_uring.ring);
    if (ASCO_LIKELY(sqe)) {
        return sqe;
    } else {
        ASCO_LOG_INFO(g_logger) << "SQ is full, flushing ";
        io_uring_submit(&s_uring.ring);
        sqe = io_uring_get_sqe(&s_uring.ring);
        if (ASCO_LIKELY(sqe)) {
            return sqe;
        }
        ASCO_LOG_ERROR(g_logger) << "is null";
    }
    return nullptr;
}

IOManager::IOManager(size_t threads, bool use_caller, std::string_view name) : Scheduler(threads, use_caller, name) {
    int rt = pipe(tickleFds_);
    ASCO_ASSERT(!rt);
    tickle_ = [this]() {
        if (idleThreadCount_ == 0) {    
            return;
        }
        ASCO_LOG_DEBUG(g_logger) << "tickle!";
        auto sqe = io_uring_get_sqe_safe();
        io_uring_prep_write(sqe, tickleFds_[1], "", 1, 0);
        // io_uring_sqe_set_data(sqe, nullptr);
    };

    idle_ = [this]() -> Coroutine {
        ASCO_LOG_DEBUG(g_logger) << "idle";
        while (true) {
            if (ASCO_UNLIKELY(stopping())) {
                ASCO_LOG_FMT_INFO(g_logger, "IOManager name = %s idle stopping exit", Scheduler::getName().c_str());
                break;
            }

            io_uring_submit_and_wait(&s_uring.ring, 1);
            // io_uring_submit(&s_uring.ring);

            io_uring_cqe* cqe = nullptr;
            unsigned head;
            int cqe_count = 0;
            
            io_uring_for_each_cqe(&s_uring.ring, head, cqe) {
                auto coro = static_cast<resume_resolver*>(io_uring_cqe_get_data(cqe));
                if (coro) {
                    coro->resolve(cqe->res);
                    pendingEventCount_--;
                } else {
                    // uint8_t dummy[256];
                    // while (read(tickleFds_[0], &dummy, sizeof(dummy)) > 0);
                    // continue;
                }
                cqe_count++;
                // io_uring_cqe_seen(&s_uring.ring, cqe);
            }

            io_uring_cq_advance(&s_uring.ring, cqe_count);
            ASCO_LOG_DEBUG(g_logger) << "handle " << cqe_count << " cqe";

            co_await Coroutine::yield();
        }
    };

    start();
}

bool IOManager::stopping() {
    return pendingEventCount_ == 0 && Scheduler::stopping();
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

IOManager::~IOManager() {
    stop();
    close(tickleFds_[0]);
    close(tickleFds_[1]);

}

Awaiter_io async_read(int fd, void* buf, unsigned long nbytes, unsigned long long offset) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_read(sqe, fd, buf, nbytes, offset);
    return Awaiter_io(sqe);
}

Awaiter_io async_write(int fd, const void* buf, unsigned long nbytes, unsigned long long offset) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_write(sqe, fd, buf, nbytes, offset);
    return Awaiter_io(sqe);
}

Awaiter_io async_readv(int fd, struct iovec* iov, unsigned int nr_vecs, unsigned long long offset) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_readv(sqe, fd, iov, nr_vecs, offset);
    return Awaiter_io(sqe);
}

Awaiter_io async_writev(int fd, const struct iovec* iov, unsigned int nr_vecs, unsigned long long offset) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_writev(sqe, fd, iov, nr_vecs, offset);
    return Awaiter_io(sqe);
}

Awaiter_io async_read_fixed(int fd, void* buf, unsigned long nbytes, unsigned long long offset, int buf_index) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_read_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    return Awaiter_io(sqe);
}

Awaiter_io async_write_fixed(int fd, const void* buf, unsigned long nbytes, unsigned long long offset, int buf_index) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_write_fixed(sqe, fd, buf, nbytes, offset, buf_index);
    return Awaiter_io(sqe);
}

Awaiter_io async_recvmsg(int sockfd, msghdr* msg, uint32_t flags) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_recvmsg(sqe, sockfd, msg, flags);
    return Awaiter_io(sqe);
}

Awaiter_io async_sendmsg(int sockfd, const msghdr* msg, uint32_t flags) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_sendmsg(sqe, sockfd, msg, flags);
    return Awaiter_io(sqe);
}

Awaiter_io async_recv(int sockfd, void* buf, size_t length, int flags) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_recv(sqe, sockfd, buf, length, flags);
    return Awaiter_io(sqe);
}

Awaiter_io async_send(int sockfd, const void* buf, size_t length, int flags) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_send(sqe, sockfd, buf, length, flags);
    return Awaiter_io(sqe);
}

Awaiter_io async_connect(int fd, sockaddr* addr, socklen_t addrlen) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_connect(sqe, fd, addr, addrlen);
    return Awaiter_io(sqe);
}

Awaiter_io async_accept(int fd, sockaddr* addr, socklen_t* addrlen, int flags) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
    return Awaiter_io(sqe);
}

Awaiter_io async_close(int fd) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_close(sqe, fd);
    return Awaiter_io(sqe);
}

Awaiter_io async_timeout(__kernel_timespec* ts) {
    IOManager::GetThis()->pendingEventCount_++;
    auto sqe = io_uring_get_sqe_safe();
    io_uring_prep_timeout(sqe, ts, 0, 0);
    return Awaiter_io(sqe);
}

thread_local std::vector<std::unique_ptr<__kernel_timespec>> ts;

Awaiter_io async_sleep(long long sec) {
    ts.push_back(std::make_unique<__kernel_timespec>(sec, 0));
    return async_timeout(ts.back().get());
}

Awaiter_io async_usleep(long long usec) {
    ts.push_back(std::make_unique<__kernel_timespec>(0, usec));
    return async_timeout(ts.back().get());
}

void register_buffers(const struct iovec *iovecs, unsigned nr_iovecs) {
    io_uring_register_buffers(&s_uring.ring, iovecs, nr_iovecs);
}

} // namespace asco
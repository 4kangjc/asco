#include "asco/async_io.h"
#include "asco/log.h"
#include "asco/timer.h"

static auto& g_logger = ASCO_LOG_ROOT(); 

__kernel_timespec ts{2, 0};

asco::Coroutine test_sleep2() {
    ASCO_LOG_INFO(g_logger) << "sleep 2 second...";
    // co_await asco::async_sleep(2);
    co_await asco::async_timeout(&ts);
    ASCO_LOG_INFO(g_logger) << "sleep 2 finish!";
}

asco::Coroutine test_sleep3() {
    ts.tv_sec = 4;
    ASCO_LOG_INFO(g_logger) << "sleep 3 second...";
    co_await asco::async_sleep(3);
    ASCO_LOG_INFO(g_logger) << "sleep 3 finish!";
}

asco::Coroutine test_read_fixed() {
    // ASCO_LOG_INFO(g_logger) << ""
    std::string buf;
    buf.resize(4096);
    iovec iov;
    iov.iov_base = buf.data();
    iov.iov_len = buf.size();
    asco::register_buffers(&iov, 1);
    // io_uring_register_buffers()
    int n = co_await asco::async_read_fixed(0, buf.data(), 14);
    ASCO_LOG_FMT_INFO(g_logger, "read %d bytes from buf : %s", n, buf.c_str());
    // co_await asco::async_read(0, buf.data(), 14);
}


int main() {
    asco::IOManager iom;
    // iom.schedule(test_sleep2());
    // iom.schedule(test_sleep3());
    // iom.schedule(test_read_fixed());
    auto timer = iom.addTimer(1000, true, [](){
        ASCO_LOG_INFO(g_logger) << "Hello World";
    });
    // timer->cacel();
    timer->reset(1000, [](){
        ASCO_LOG_INFO(g_logger) << "Hello reset!";
    });
    timer->setRecurring(false);
}   
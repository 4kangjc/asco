#include "asco/scheduler.h"
#include "asco/log.h"

static auto& g_logger = ASCO_LOG_ROOT();

asco::Coroutine test() {
    co_return "test";
}

struct Add {
    asco::Coroutine operator()(int a, int b) {
        ASCO_LOG_FMT_INFO(g_logger, "%d + %d = %d", a, b, a + b);
        co_return a + b;
    }
};

int main() {
    asco::Scheduler sc(2, true, "TEST");
    sc.start();

    Add a;
    auto ac = std::make_shared<asco::Coroutine>(a(1, 2));
    sc.schedule(ac);

    sc.schedule([&]() -> asco::Coroutine {
        ASCO_LOG_INFO(g_logger) << "Hello Coroutine!";
        // sc.schedule(asco::Coroutine::GetThis());
        // co_await asco::Coroutine::yield();
        // ASCO_LOG_INFO(g_logger) << "call back";
        co_return "this is co!";
    }());

    sc.schedule(test());

    if (ac->getState() == asco::Coroutine::FINISH) {
        ASCO_LOG_INFO(g_logger) << ac->get_result<int>();
    } else {
        ASCO_LOG_INFO(g_logger) << "ac is not finish, its state = " << ac->getState();
    }

    sc.stop();

    // ASCO_LOG_INFO(g_logger) << ac->get_result<int>();
}
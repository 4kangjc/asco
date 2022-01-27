#include "asco/coroutine.h"
#include "asco/log.h"

static auto& g_logger = ASCO_LOG_ROOT();

int main() {
    // auto co = std::make_shared<asco::Coroutine>([]() -> asco::Coroutine {
    //     ASCO_LOG_INFO(g_logger) << "Hello Coroutine!";
    //     co_await asco::Coroutine::yield();
    //     ASCO_LOG_INFO(g_logger) << "call back";
    //     co_return "this is co!";
    // }());
    // co->resume();
    // ASCO_LOG_INFO(g_logger) << "from co";
    // co->resume();

    asco::Coroutine co([]() -> asco::Coroutine {
        ASCO_LOG_INFO(g_logger) << "Hello Coroutine!";
        co_await asco::Coroutine::yield();
        ASCO_LOG_INFO(g_logger) << "call back";
        co_return "this is co!";
    }());

    co.resume();
    ASCO_LOG_INFO(g_logger) << "from co";
    co.resume();

    ASCO_LOG_INFO(g_logger) << co.get_result<const char*>();
}
#include "asco/config.h"

static auto& g_logger = ASCO_LOG_ROOT();
static auto&& s_logger = ASCO_LOG_NAME("system");

void test_config() {
    ASCO_LOG_INFO(g_logger) << "Hello config";
    ASCO_LOG_INFO(s_logger) << "Hello config";
    asco::Config::LoadFromConDir("conf/");
    ASCO_LOG_INFO(g_logger) << "Hello config";
    ASCO_LOG_INFO(s_logger) << "Hello config";
}

int main() {
    test_config();
}
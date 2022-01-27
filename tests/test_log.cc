#include "asco/log.h"

static auto& g_logger = ASCO_LOG_ROOT();

int main() {
    ASCO_LOG_INFO(g_logger) << "Hello World!";
    ASCO_LOG_FMT_INFO(g_logger, "Hello %s", "world!");
    asco::LogAppender::ptr file_appender(new asco::FileLogAppender("conf/test.log"));
    g_logger->addAppender(file_appender);
    ASCO_LOG_INFO(g_logger) << "Hello file!";
}
#include "asco/async_io.h"
#include "asco/log.h"
#include "asco/macro.h"
#include <cstring>
#include <arpa/inet.h>

static auto& g_logger = ASCO_LOG_ROOT();


int main(int argc, char** argv) {
    const char* ip;
    int port;

    if (argc < 3) {
        ASCO_LOG_INFO(g_logger) << "argc < 3...";
        return 1;
    }

    ip = argv[1];
    port = atoi(argv[2]);

    asco::IOManager iom;
    iom.schedule([]() -> asco::Coroutine {
        // int n = co_await asco::async_write(1, "Hello World!\n", 14);
        // ASCO_LOG_INFO(g_logger) << "write " << n << " bytes to stdout";

        std::string buf;
        buf.resize(1024);
        int m = co_await asco::async_read(0, buf.data(), buf.size());
        ASCO_LOG_INFO(g_logger) << "read " << m << " bytes from stdin";

        int n = co_await asco::async_write(1, "Hello World!\n", 14);
        ASCO_LOG_INFO(g_logger) << "write " << n << " bytes to stdout";
    }());
    iom.schedule([]() -> asco::Coroutine {
        co_return "H";
    }());

    iom.schedule([](const char* ip, int port) -> asco::Coroutine {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        ASCO_ASSERT(sockfd >= 0);
        sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
        do {
            int rt = co_await asco::async_connect(sockfd, (sockaddr*)&addr, sizeof(addr));
            if (rt) {
                ASCO_LOG_INFO(g_logger) << "connect fail!, ip = " << ip << ", port = " << port;
                break;
            }

            const char data[] = "GET / HTTP/1.1\r\n\r\n";
            rt = co_await asco::async_send(sockfd, data, sizeof(data), 0);
            ASCO_LOG_INFO(g_logger) << "send rt = " << rt << " errno = " << errno;

            if (rt <= 0) {
                break;
            }

            std::string buff;
            buff.resize(4096);
            rt = co_await asco::async_recv(sockfd, buff.data(), buff.size(), 0);
            ASCO_LOG_INFO(g_logger) << "recv rt = " << rt << " errno = " << errno;
            if (rt <= 0) {
                break;
            }
            buff.resize(rt);
            ASCO_LOG_INFO(g_logger) << buff;
        } while (false);
    
        co_await asco::async_close(sockfd);

    }(ip, port));
}
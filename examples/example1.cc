#include <asco/asco.h>
#include <iostream>
using namespace asco;

int main () {
    IOManager iom;
    iom.schedule([]() -> Coroutine {
        const char hello[] = "Hello World!\n";
        int n = co_await async_write(1, hello, sizeof(hello));
        std::cout << "n = " << n << std::endl;
    }());
}

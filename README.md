# asco

## 介绍
asco 是一个基于liburing和cpp 20 coroutine的异步协程库  


## 编译
* 编译环境  
`gcc version 11.1.0` (支持c++2a)  
`Linux 5.15.13-arch1-1 x86_64` (支持io_uring)  

* 编译
```
git clone https://github.com/4kangjc/asco.git
cd asco
git submodule update --init --recursive
make
```

## 事例
```cpp
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
```

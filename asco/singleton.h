#pragma once

#include <memory>

namespace asco {

template <typename T>
class Singleton {
public:
    static T& GetInstance() {
        static T v;
        return v;
    }
};

template <typename T>
class SingletonPtr {
public: 
    static std::shared_ptr<T> GetInstance() {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

} // namespace asco
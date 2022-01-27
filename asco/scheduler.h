#pragma once

#include "coroutine.h"
#include "task.h"
#include "thread.h"
#include "mutex.h"
#include <string_view>
#include <deque>
#include <vector>
#include <atomic>

namespace asco {

class Scheduler {
public:
    /**
     * @brief 构造函数
     * @param[in] threads 线程数量
     * @param[in] use_caller 是否使用当前调用线程
     * @param[in] name 协程调度器名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true, std::string_view name = "");
    ~Scheduler();
    // 返回协程调第器的名称
    auto& getName() const { return name_; }
    static Scheduler* GetThis();
    // 开始协程调度器
    void start();
    // 停止协程调度器
    void stop();
    
    void schedule(Coroutine&& co);
    void schedule(const Coroutine::ptr& co);
    void schedule(Coroutine::ptr&& co);

protected:
    // 线程执行实体
    void run();
    // 返回是否可以停止
    virtual bool stopping();
    // 设置当前协程调度器
    void setThis();
    // 是否有空闲线程
    bool hasIdleThreads() const { return idleThreadCount_ > 0; }
private:    
    mutable mutex mutex_;                                                      // Mutex       
    std::vector<Thread::ptr> threads_;                                         // 线程池                                    
    std::deque<Coroutine::ptr> tasks_;                                         // 待执行的协程
    std::string name_;                                                         // 调度器名称        
protected:
    std::vector<int> threadIds_;                                               // 线程id数组
    size_t threadCount_                    = 0;                                // 线程数量
    std::atomic<size_t> activeThreadCount_ = {0};                              // 工作线程数量
    std::atomic<size_t> idleThreadCount_   = {0};                              // 空闲线程数量
    bool stopping_ = true;                                                     // 是否正在停止
    int rootThreadId_ = 0;                                                     // 主线程id(use_caller)
    std::function<Coroutine()> idle_;                                          // 协程无任务调度时执行idle协程
    __task tickle_;                                                            // 通知调度器有任务了

};

}
#ifndef _COROUTINE_H_
#define _COROUTINE_H_

#include <iostream>     
#include <memory>       
#include <atomic>       
#include <functional>   
#include <cassert>      
#include <ucontext.h>   
#include <unistd.h>
#include <mutex>

namespace jozon {

class Fiber: public std::enable_shared_from_this<Fiber> {
public:
    enum State 
    {
        READY,     // 准备就绪
        RUNNING,   // 正在运行
        TERM,      // 挂起
    };
private:
    Fiber();
public:
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);
    ~Fiber();
    //重启一个协程
    void reset(std::function<void()> cb);

    //任务协程恢复执行
    void resume();

    //协程挂起
    void yield();

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

    //设置当前运行的协程
    static void SetThis(Fiber* f) ;

    //获取当前运行的协程
    static std::shared_ptr<Fiber> GetThis();

    //设置调度协程，即主协程
    static void SetSchedulerFiber(Fiber* f);
	

    //得到当前协程的id
    static uint64_t GetFiberId() ;

    //协程函数
    static void MainFunc();

private:
    uint64_t m_id;                //协程id
    
    uint64_t m_stacksize;        //协程栈大小
    ucontext_t m_ctx;            //协程上下文
    std::function<void()> m_cb;  //协程函数
    
	void* m_stack = nullptr;    // 协程栈指针
    State m_state = READY;               //协程状态
    bool m_runInScheduler = false; //是否在调度器中运行

public:
    std::mutex m_mutex; //协程锁
};


}//end namespace jozon

#endif // _COROUTINE_H_
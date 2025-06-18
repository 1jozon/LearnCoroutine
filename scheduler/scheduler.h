#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include "../thread/thread.h"
#include "../fiber/fiber.h"

#include <mutex>
#include <vector>

namespace jozon{

class scheduler{
public:
    scheduler(size_t threads = 1, bool use_caller = true, const std::string& name="Scheduler");
    virtual ~scheduler();

    const std::string& getName() const {return m_name;}

public:    
    // 获取正在运行的调度器
    static scheduler* GetThis();

protected:
    // 设置正在运行的调度器
    void SetThis(); 

public:
    // 添加任务到任务队列
    template <class FiberOrCb>
    void scheduleLock(FiberOrCb fc, int thread = -1)
    {
        bool need_tickle;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            // empty ->  all thread is idle -> need to be waken up
            need_tickle = m_tasks.empty();

            ScheduleTask task(fc, thread);
            if (task.fiber || task.cb)
            {
                m_tasks.push_back(task);
            }
        }
        if (need_tickle)
        {
            tickle();
        }
    }
    // 启动线程池
    virtual void start();
    // 关闭线程池
    virtual void stop();        

protected:
    virtual void tickle();
    // 运行线程池
    virtual void run(); 
    // 线程空闲时的处理
    virtual void idle(); 
    // 是否可以关闭     
    virtual bool stopping();           
    bool hasIdleThreads() { return m_idleThreadCount > 0; }
private:
    struct SchedulerTask
    {
        std::shared_ptr<Fiber> fiber;
        std::function<void()> cb;
        int thread; // 指定任务需要运行的线程id
        SchedulerTask()
            : fiber(nullptr), cb(nullptr), thread(-1) {}
        SchedulerTask(std::shared_ptr<Fiber> f, int thr)
            : fiber(f), thread(thr) {}
        SchedulerTask(std::shared_ptr<Fiber>* f, int thr)
            : thread(thr)
        {
            fiber.swap(*f);
        }
        SchedulerTask(std::function<void()> f, int thr)
            : cb(f), thread(thr) {}
        SchedulerTask(std::function<void()>* f, int thr)
            : thread(thr)
        {
            cb.swap(*f);
        }       
        void reset()
        {
            fiber = nullptr;
            cb = nullptr;
            thread = -1;
        }
    };
private:
    std::string m_name; // 调度器名称
    size_t m_threadCount; // 线程数量
    
    bool m_stopping = false; // 是否正在停止

    // std::vector<std::shared_ptr<Thread>> m_threads; // 线程池
    std::vector<int> m_threadIds; // 线程id列表

    std::mutex m_mutex; // 互斥锁
    std::vector<SchedulerTask> m_tasks; // 任务队列

    std::atomic<size_t> m_activeThreadCount = {0}; // 活跃线程数
    std::atomic<size_t> m_idleThreadCount = {0}; // 空闲线程数
    bool m_stopping = false; // 是否正在停止

    bool m_useCaller; // 是否使用主线程用于工作线程
    std::shared_ptr<Fiber> m_schedulerFiber; // 调度器协程
    int m_rootThread; // 主线程id
}



#endif // _SCHEDULER_H_
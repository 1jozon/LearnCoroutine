#include "scheduler.h"
static bool debug = true;
namespace jozon {

static thread_local scheduler* t_scheduler = nullptr;

scheduler* scheduler::GetThis() {
    return t_scheduler;
}

void scheduler::SetThis() {
    t_scheduler = this;
}

scheduler::scheduler(size_t threads, bool use_caller, const std::string &name):
m_useCaller(use_caller),m_name(name)
{
    assert(threads >0 && scheduler::GetThis()==nullptr);
    SetThis();
    Thread::SetName(m_name);
    if (use_caller)
    {   
        threads--;
        // 创建主协程
        Fiber::GetThis();
        // 创建调度协程
        m_schedulerFiber.reset(new Fiber(std::bind(&scheduler::run, this), 0, false)); // false -> 该调度协程退出后将返回主协程
        Fiber::SetSchedulerFiber(m_schedulerFiber.get());
        
        m_rootThread = Thread::GetThreadId();
        m_threadIds.push_back(m_rootThread);

    }
    m_threadCount = threads;
    if(debug) std::cout << "scheduler::scheduler() success\n";
}

scheduler::~scheduler() {
    assert(stopping() == true);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
    if(debug) std::cout << "scheduler::~scheduler() success\n";
}

void scheduler::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_stopping){
        std::cerr << "scheduler::start() error: scheduler is stopping" << std::endl;
        return;
    }
    assert(m_threads.empty());
    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; i++) {
        m_threads[i].reset(new Thread(std::bind(&scheduler::run, this), m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    if (debug) std::cout << "scheduler::start() success" << std::endl;
}

void scheduler::run()
{
    int thread_id = Thread::GetThreadId();
    if (debug) std::cout << "scheduler::run() starts in thread: " << thread_id << std::endl;
    SetThis();
    if (thread_id != m_rootThread){
        Fiber::GetThis();
    }
    std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&scheduler::idle, this));
    SchedulerTask task;
    while(true){
        task.reset();
        bool tickle_me = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_tasks.begin();
            while (it != m_tasks.end()) {
                if (it->thread == -1 || it->thread == thread_id){
                    it++;
                    tickle_me = true;
                    continue;
                }
                assert( it->fiber || it->cb);
                task = *it;
                m_tasks.erase(it);
                m_activeThreadCount++;
                break;
            }
             tickle_me = tickle_me || !m_tasks.empty();
        }
        if (tickle_me) {
            tickle();
        }
        if (task.fiber)
        {
            {
                std::lock_guard<std::mutex> lock(task.fiber->m_mutex);
                if (task.fiber->getState() != Fiber::TERM) {
                    task.fiber->resume();
                }
            }
            m_activeThreadCount--;
            task.reset();
        }
        else if (task.cb)
        {
            std::shared_ptr<Fiber> cb_fiber = std::make_shared<Fiber>(task.cb);
            {
                std::lock_guard<std::mutex> lock(cb_fiber->m_mutex);
                cb_fiber->resume();
            }
            m_activeThreadCount--;
            task.reset();
        }
        else
        {
            if (idle_fiber->getState() == Fiber::TERM) {
                if (debug) std::cout << "scheduler::run() ends in thread: " << thread_id << std::endl;
                break;
            }
            m_idleThreadCount++;
            idle_fiber->resume();
            m_idleThreadCount--;
        }
    }

}

void scheduler::stop(){
    if (debug) std::cout<< "scheduler::stop() starts in thread: " << Thread::GetThreadId() << std::endl;
    if (stopping()){
        return ;
    }
    m_stopping = true;
    if (m_useCaller) {
        assert(GetThis() == this);
    } else {
        assert(GetThis() != this);
    }  
    for (size_t i=0; i<m_threadCount; i++){
        tickle();
    }
    if (m_schedulerFiber) {
        tickle();   
    }
    if (m_schedulerFiber)
    {
        m_schedulerFiber->resume();
        if (debug) std::cout << "scheduler::stop() ends in thread: " << Thread::GetThreadId() << std::endl;
    }
    std::vector<std::shared_ptr<Thread>> thrs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        thrs.swap(m_threads);
    }
    for (auto& thr : thrs) {
        if (thr) {
            thr->join();
        }
    }
    if (debug) std::cout << "scheduler::stop() success" << std::endl;
}

void scheduler::tickle(){

}

void scheduler::idle(){
    while(stopping() == false){
        if (debug) std::cout << "scheduler::idle() in thread: " << Thread::GetThreadId() << std::endl;
        Fiber::GetThis()->yield();
    }
}

bool scheduler::stopping(){
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stopping && m_activeThreadCount == 0 && m_tasks.empty();
}

} // end of namespace jozon
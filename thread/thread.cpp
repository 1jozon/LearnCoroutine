#include "thread.h"

#include <sys/syscall.h> 
#include <iostream>
#include <unistd.h> 

namespace jozon {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "unknown";

pid_t Thread::GetThreadId() {
    return static_cast<pid_t>(syscall(SYS_gettid));
}

Thread* Thread::GetThis() {
    return t_thread;
}

const std::string& Thread::GetName() {
    return t_thread_name;
}

void Thread::SetName(const std::string& name) {
    if (t_thread) {
        t_thread_name = name;
        pthread_setname_np(pthread_self(), name.c_str());
    } else {
        t_thread_name = name;
    }
}

Thread::Thread(std::function<void()> cb, const std::string& name)
    : m_cb(cb), m_name(name) {
    
    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        std::cerr << "Failed to create thread: " << rt<<" name="<<name << std::endl;
        throw std::runtime_error("Thread creation failed");
    }
    m_semaphore.wait();
}

Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
        m_thread = 0;
    }
}

void Thread::join() {
    if (m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if(rt){
            std::cerr << "Failed to join thread: " << rt << " name=" << m_name << std::endl;
            throw std::logic_error("Thread join failed");
        }
        m_thread = 0;
    }
}
void* Thread::run(void* arg) {
    Thread* thread = (Thread*)(arg);
    t_thread = thread;
    t_thread_name = thread->m_name;

    thread->m_id = GetThreadId();
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    std::function<void()> cb;
    cb.swap(thread->m_cb); // swap -> 可以减少m_cb中只能指针的引用计数

    thread->m_semaphore.signal();
    cb(); // 执行线程函数
    return nullptr;
} 
}
#ifndef _THREAD_H
#define _THREAD_H

#include <mutex>
#include <condition_variable>
#include <functional>

namespace jozon{
    
class Semaphore
{
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
public:
    explicit Semaphore(int initial_count = 0) : count(initial_count) {}
    // p操作
    void wait(){
        std::unique_lock<std::mutex> lock(mtx);
        while (count <= 0){
            cv.wait(lock);
        }
        --count;
    }
    // v操作
    void signal(){
        std::unique_lock<std::mutex> lock(mtx);
        ++count;
        cv.notify_one();
    }
};

class Thread
{
public:
    Thread(std::function<void()> cb, const std::string& name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();

public:
    // 获取系统分配的线程id
	static pid_t GetThreadId();
    // 获取当前所在线程
    static Thread* GetThis();

    // 获取当前线程的名字
    static const std::string& GetName();
    // 设置当前线程的名字
    static void SetName(const std::string& name);

private:
	// 线程函数
    static void* run(void* arg);

private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;

    // 线程需要运行的函数
    std::function<void()> m_cb;
    std::string m_name;
    
    Semaphore m_semaphore;

};
}  
#endif
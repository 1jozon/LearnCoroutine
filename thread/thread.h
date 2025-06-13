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
}

}

#endif
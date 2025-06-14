#include "thread.h"
#include <iostream>
#include <memory>
#include <vector>
#include <unistd.h>  

using namespace jozon;

void func()
{
    std::cout << "第一次输出..." << ", this id: " << Thread::GetThis()->getId() << ", this name: " << Thread::GetThis()->getName() <<std::endl;

    sleep(3);  // 沉睡三分钟

    std::cout << "第二次输出..." << ", this id: " << Thread::GetThis()->getId() << ", this name: " << Thread::GetThis()->getName() <<std::endl;
    // std::cout << ", this id: " << Thread::GetThis()->getId() << ", this name: " << Thread::GetThis()->getName() << std::endl;
}

int main() {
    std::vector<std::shared_ptr<Thread>> thrs;

    for(int i=0;i<5;i++)
    {
        std::shared_ptr<Thread> thr = std::make_shared<Thread>(&func, "thread_"+std::to_string(i));
        thrs.push_back(thr);
        
    }

    for(int i=0;i<5;i++)
    {
        thrs[i]->join();
    }

    return 0;
}
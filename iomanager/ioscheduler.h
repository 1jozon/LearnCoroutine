#ifndef __IOMANAGER_H__
#define __IOMANAGER_H__
#include "../scheduler/scheduler.h"
#include "../timer/timer.h"
namespace jozon {

class IOManager : public scheduler, public TimerManager 
{
public:
    enum Event 
    {
        NONE = 0x0,
        READ = 0x1,   // EPOLLIN
        WRITE = 0x4   // EPOLLOUT
    };
private:
      struct  FdContext 
      {
          struct EventContext 
          {
              scheduler* scheduler = nullptr; // 调度器
              std::shared_ptr<Fiber> fiber; // 协程
              std::function<void()> cb; // 回调函数
          };

          EventContext read; // 读事件上下文
          EventContext write; // 写事件上下文
          int fd = 0; // 文件描述符
          Event events = NONE; // 注册的事件类型
          std::mutex mutex; // 互斥锁

          EventContext& getEventContext(Event event); // 获取事件上下文
          void resetEventContext(EventContext &ctx); // 重置事件上下文
          void triggerEvent(Event event); // 触发事件
      };
public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "IOManager");
    ~IOManager();

    // 添加事件
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    // 删除事件                 
    bool delEvent(int fd, Event event);
    // 取消事件
    bool cancelEvent(int fd, Event event);
    // 取消所有事件并触发回调
    bool cancelAll(int fd); 
    
    static IOManager* GetThis() ;
protected:
    void tickle() override; // 唤醒调度器
    bool stopping() override; // 是否停止
    void idle() override; // 空闲时处理
    void contextResize(size_t size); // 调整上下文大小
    void onTimerInsertedAtFront() override; // 当一个最早的timer加入到堆中时调用
private:
      int m_epfd = 0; // epoll文件描述符
      int m_tickleFds[2]; // 用于唤醒调度器的文件描述符
      std::vector<FdContext*> m_fdContexts; // 文件描述符上下文列表
      std::atomic<size_t> m_pendingEventCount = {0}; // 待处理事件数量
      std::mutex m_mutex; // 互斥锁
};


} // end of namespace jozon


#endif __IOMANAGER_H__
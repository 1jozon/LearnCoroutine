#include <iostream>
#include <ucontext.h>
#include <cstdlib>

#define STACK_SIZE 1024 * 128  // 协程栈大小

char stack[STACK_SIZE];   // 栈空间
ucontext_t ctx_main;      // 主上下文
ucontext_t ctx_child;     // 子上下文

void child_func()
{
    std::cout << "Child: In coroutine\n";
    
    std::cout << "Child: Switch back to main context\n";
    swapcontext(&ctx_child, &ctx_main);  // 切回主上下文
    
    std::cout << "Child: Back again (should not happen)\n"; // 不会执行
}

int main()
{
    // 1. 获取主上下文
    getcontext(&ctx_child);

    // 2. 设置子上下文栈空间
    ctx_child.uc_stack.ss_sp = stack;
    ctx_child.uc_stack.ss_size = STACK_SIZE;
    ctx_child.uc_stack.ss_flags = 0;
    ctx_child.uc_link = &ctx_main;  // 执行完后跳转到主上下文

    // 3. 绑定函数
    makecontext(&ctx_child, (void(*)(void))child_func, 0);

    // 4. 获取主上下文
    getcontext(&ctx_main);

    std::cout << "Main: Switching to coroutine\n";
    swapcontext(&ctx_main, &ctx_child);  // 切换到子上下文

    std::cout << "Main: Back from coroutine\n";

    return 0;
}
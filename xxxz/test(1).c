/* gcc -E hello.c -o hello.i      # 预处理
gcc -S hello.i -o hello.s      # 编译为汇编
gcc -c hello.s -o hello.o      # 汇编为目标文件
gcc hello.o -o hello           # 链接
man gcc*/

/* gcc -g -o myprog myprog.c
gdb myprog          # 直接调试可执行文件
gdb myprog core     # 同时加载 core dump 文件（程序崩溃后生成）
gdb -p PID          # 附加到一个已经运行的进程
设置断点
(gdb) break main          # 在 main 函数入口暂停
(gdb) break 7             # 在第 7 行暂停
(gdb) break add           # 在 add 函数开头暂停
(gdb) info breakpoints    # 查看所有断点
(gdb) delete 1            # 删除编号为 1 的断点
(gdb) clear add           # 清除 add 函数处的断点
运行与控制程序
(gdb) run                 # 开始运行程序（可加参数，如 run arg1 arg2）
(gdb) next                # 单步执行，不进入函数内部（缩写 n）
(gdb) step                # 单步执行，遇到函数会进入内部（缩写 s）
(gdb) continue            # 继续运行直到下一个断点（缩写 c）
(gdb) finish              # 运行直到当前函数返回
(gdb) until 9             # 继续运行到第 9 行
查看状态
(gdb) print x             # 打印变量 x 的值（缩写 p）
(gdb) p result            # 打印 result
(gdb) p &x                # 打印变量 x 的地址
(gdb) p *ptr              # 打印指针指向的内容
(gdb) display x           # 每次暂停时自动显示 x 的值
(gdb) info locals         # 查看当前函数所有局部变量
(gdb) backtrace           # 查看函数调用栈（缩写 bt）
(gdb) frame 2             # 切换到栈帧 2（查看调用者的上下文）
修改变量或内存
(gdb) set var x = 100     # 将 x 改为 100
(gdb) set {int}0x7fff1234 = 42  # 向内存地址写值
退出 GDB
(gdb) quit                # 或按 Ctrl+D */

#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int main()
{
    int x = 10, y = 20;
    int result = add(x, y);
    printf("Result = %d\n", result);
    return 0;
}
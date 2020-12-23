# NEMU
This is a simplified simulator is implemented to simulate the computer system.
It's a program that runs other programs. That is, NEMU's job is to simulate a set of computer hardware on which programs can run.


## architecture
x86

## dentry
```bash
NEMU
├── abstract-machine   # 抽象计算机
├── fceux-am           # 红白机模拟器
├── init.sh            # init script
├── Makefile           
├── nemu               # simulator
└── README.md

nemu
├── include                    # 存放全局使用的头文件
│   ├── common.h               # 公用的头文件
│   ├── cpu
│   │   ├── decode.h           # 译码相关
│   │   └── exec.h             # 执行相关
│   ├── debug.h                # 一些方便调试用的宏
│   ├── device                 # 设备相关
│   ├── isa                    # ISA相关
│   ├── isa.h                  # ISA相关
│   ├── macro.h                # 一些方便的宏定义
│   ├── memory                 # 访问内存相关
│   ├── monitor
│   │   ├── log.h              # 日志文件相关
│   │   └── monitor.h
│   └── rtl
│       ├── pesudo.h           # RTL伪指令
│       └── rtl.h              # RTL指令相关定义
├── Makefile                   # 指示NEMU的编译和链接
├── Makefile.git               # git版本控制相关
├── runall.sh                  # 一键测试脚本
└── src                        # 源文件
    ├── device                 # 设备相关
    ├── engine
    │   └── interpreter        # 解释器的实现
    ├── isa                    # ISA相关的实现
    │   ├── mips32
    │   ├── riscv32
    │   └── x86
    ├── main.c                 # 你知道的...
    ├── memory
    │   └── paddr.c            # 物理内存访问
    └── monitor
        ├── cpu-exec.c         # 指令执行的主循环
        ├── debug              # 简易调试器相关
        │   ├── expr.c         # 表达式求值的实现
        │   ├── log.c          # 日志文件相关
        │   ├── ui.c           # 用户界面相关
        │   └── watchpoint.c   # 监视点的实现
        └── monitor.c


```
## 开发历程
首先开发`monitor`模块
使用匿名`struct` + 匿名`union` re-organize CPU_state cpu 结构体
用户界面主循环(`ui_mainloop`) 是`monitor`的核心功能，可以在命令行中输入命令，对客户计算机进行监控和调试，类似于gdb
`monitor/cpu-exec.c/cpu_exec()`模拟CPU执行一个程序的过程（取址、译码、执行）
### 编写简单调试器（简易gdb）
主要完善 `monitor/debug/ui.c`
完成表达式求值 `expr.c`
实现 monitor的调试功能，相当于实现了简易的 gdb，功能如下：
```bash
help - Display informations about all supported commands
c - Continue the execution of the program
q - Exit NEMU
si - execute the program step by step
info - print registers status
p - calculate the value of indicate expression
x - x N EXPR: calculate the val of EXPR, using the result as the start mem addresss and output     N consecutive 4 bytes in hexademical form
w - w EXPR: Pause the program when the value of EXPR changes
d - d N : delete the watch point which serial number is N
```

### 开启PA2



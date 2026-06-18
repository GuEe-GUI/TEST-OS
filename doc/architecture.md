# 架构说明

## 总体结构

```
boot/boot.asm          MBR，加载 Loader
boot/loader.asm        实模式 → 保护模式 → 分页，加载 ELF 内核
kernel/                内核 C 与汇编
  start.c              入口与初始化
  memory.c             页表扩展、页故障
  thread.c             内核线程调度
  interrupt.c          IDT / PIC / ISR
  timer.c              PIT 定时器
  console.c / vbe.c    图形控制台
  ide.c / disk.c       块设备抽象
  fat32.c              FAT32 文件系统
  eval.c / cmds.c      交互式 Shell
libraries/             字符串等基础库
include/               公共头文件
```

## 启动流程

1. **boot.asm**（0x7C00）：从软盘读取 Loader 到 0x7000
2. **loader.asm**：
   - INT 15h E820 探测内存，结果写入 `0x800007010` / `0x800007050`
   - VBE 设置图形模式（默认 800×600×24）
   - 建立 GDT，开启 A20，进入保护模式
   - 建立页表：低 4MB 恒等映射、高 0x80000000 内核映射、显存映射
   - 解析 ELF Program Header，拷贝各段到虚拟地址
   - 跳转到 `0x80100000` 的 `_start`
3. **start.c `entry()`**：初始化各子系统后调用 `start_thread()`，启动 `sleeper_polling` 与 `eval` 两个内核线程

## 内存布局

| 虚拟地址 | 用途 |
|----------|------|
| `0x00000000` | 低 4MB 恒等映射（NULL 页不可访问） |
| `0x80000000` | 内核线性映射基址 |
| `0x80100000` | 内核链接入口 |
| `0x800007010` | ARDS 数量（Loader 写入） |
| `0x800007050` | ARDS 数组 |
| `0x80002000` | 内核栈顶 |
| 堆起始 | `KERNEL_HEAP_BOTTOM` 之后，位图分配器管理 |

物理页表在 Loader 阶段预分配：`0x3000` 页目录，`0x4000`/`0x5000`/`0x6000` 页表。

## 线程模型

- 最多 **256** 个内核线程（tid 位图分配）
- 状态：`RUNNING` / `READY` / `SUSPEND` / `DIED`
- **tid 0**：`thread_cleaner`，负责回收已退出线程
- 定时器 IRQ0 每 `KERNEL_THREAD_STEP` tick 触发抢占调度
- `sleep()` 通过 `sleeper_polling` 线程轮询唤醒

## 磁盘与路径

- 盘符编码：`a:` = `0x3a61`，存于路径前 2 字节
- 路径示例：`a:/dir/file.txt`
- 第一块盘：软盘 `TEST-OS.img`（仅启动）
- 第二块盘：IDE `FAT-FS.img`（数据，需 `disk -f fat32 a:` 格式化）

## 调试

编译时 `DEBUG_MODE=y`（默认）启用 `LOG()` 宏，在控制台输出带 tick 前缀的调试信息。

GDB 调试：

```bash
make run CROSS_COMPILE= GDB_MODE=y
# 另一终端
make gdb CROSS_COMPILE= GDB_MODE=y
```

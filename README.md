# TEST OS

本项目为i386架构下简单内核的实现，仅作为学习目的编写，非常小，不会提供复杂的功能

## 1、环境
- Windows环境下建议使用WSL，MSYS2或Cygwin等POSIX环境，并安装`NASM` 可生成elf的`GCC` `QEMU`
- Linux环境下，安装对应缺少的环境即可
- 环境安装好之后，选择交叉编译器前缀，`make run`后即可编译并运行

## 2、案例
[线程创建](doc/thread.md)

## 3、运行效果
![展示](https://raw.githubusercontent.com/GuEe-GUI/TEST-OS/master/doc/show.png "展示")
![帮助](https://raw.githubusercontent.com/GuEe-GUI/TEST-OS/master/doc/help.png "帮助")
![磁盘](https://raw.githubusercontent.com/GuEe-GUI/TEST-OS/master/doc/disk.png "磁盘")

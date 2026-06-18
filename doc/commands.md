# eval Shell 命令参考

启动后进入 `eval` 交互界面，提示符格式：

```
<admin>@eval <当前路径>>
```

输入 `help` 查看全部命令。

## 系统

| 命令 | 说明 |
|------|------|
| `help` | 列出所有命令 |
| `version` | 显示 OS 版本信息 |
| `cls` | 清屏 |
| `powerdown` | 关闭虚拟机（QEMU） |
| `rtc` | 读取 RTC 时间 |
| `mem` | 显示内存信息 |
| `graphic` | 图形模式测试 |

## 线程

| 命令 | 说明 |
|------|------|
| `thread -l` | 列出所有线程 |
| `thread -w <tid>` | 唤醒线程 |
| `thread -s <tid>` | 挂起线程 |
| `thread -e <tid>` | 结束线程 |

## 中断

| 命令 | 说明 |
|------|------|
| `interrupt -l` | 列出中断向量 |
| `interrupt -e <vector>` | 启用中断 |
| `interrupt -d <vector>` | 禁用中断 |

## 磁盘与文件

路径使用前导盘符，例如 `a:/` 表示 A 盘根目录。

| 命令 | 说明 |
|------|------|
| `disk -l` | 列出磁盘 |
| `disk -d <id>` | 磁盘详情（如 `a:`） |
| `disk -f <id> fat32` | 格式化为 FAT32 |
| `cd <path>` | 切换目录或盘符 |
| `dir` | 列出当前目录 |
| `md <name>` | 创建目录 |
| `del <name>` | 删除文件或目录 |
| `rename <old> <new>` | 重命名 |
| `copy <src> <dst>` | 复制文件 |
| `pushf <data> > <file>` | 覆盖写入文件 |
| `pushf <data> >> <file>` | 追加写入文件 |
| `popf -a <file>` | 以 ASCII 读取文件 |
| `popf -h <file>` | 以十六进制读取文件 |

### 典型流程

```
disk -f fat32 a:
cd a:
pushf hello > test.txt
popf -a test.txt
dir
```

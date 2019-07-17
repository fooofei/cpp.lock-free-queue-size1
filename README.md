# simple_queue
A cross platform simple lock-free queue (queue item=1)

探讨如果做队列通知。

A 线程在某个位置放置了结果， 且通知 B 线程去取用。

优势 1 通知作为 IO 事件，很容易融入事件循环

优势 2 不会漏掉事件，没有 pthread_cond_wait 那么麻烦

优势 3 无锁编程，通知利用了 SOCKET 是无锁的，利用了 atomic 是无锁的


PIPE 是单向的，而且在 Windows 上没有实现，不如直接实现一个跨平台的 socketpair.


## 差异

在 Windows 中，使用 `_pipe()` 模拟 Linux 的 `pipe`

```c
#define pipe(fds)  _pipe(fds, 4096, _O_BINARY)
```

Linux 中，pipe 的 FD 视为文件句柄，能够使用 `read()/write()`，

但是 Windows 中，不认这个，Windows 认 `ReadFile/WriteFile`

才是操作文件的, `send()/recv()/WSASend()/WSARecv()` 是操作

SOCKET的。

参考 https://my.oschina.net/moooofly/blog/184986

socketpair 只实现了 AF_UNIX ?
```
socketpair(AF_UNIX, SOCK_STREAM, 0, xx) 
```


## NON-BLOCK

Linux 上的 PIPE 设置 non-block 与 SOCKET 方式与一致，

但是在 Windows 则不一样了。

_pipe() 出来的 不能设置。

要 `CreatePipe()` -> `SetNamedPipeHandleState(PIPE_NOWAIT)` 

-> `_open_osfhandle()`

参考 [on blocking read on os.pipe on Windows] https://stackoverflow.com/questions/34504970/non-blocking-read-on-os-pipe-on-windows/34504971

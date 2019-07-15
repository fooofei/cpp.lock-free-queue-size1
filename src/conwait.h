
#ifndef CON_WAIT_H
#define CON_WAIT_H

#ifdef WIN32
#include <ws2tcpip.h>
#endif

#ifdef WIN32
typedef SOCKET sock_t;
#else
typedef int sock_t;
#define INVALID_SOCKET -1
#define closesocket close
#endif

struct conwait {
    void* value01; // use atomic write
    void* value10;
    sock_t pair[2];
};

#ifdef __cplusplus
extern "C" {
#endif

    // 暂时不清楚 iOS 锁屏 会不会使句柄无效

    int conwait_init(struct conwait* cw);
    void conwait_term(struct conwait* cw);

    sock_t conwait_up_read_sock(struct conwait* cw);
    sock_t conwait_down_read_sock(struct conwait* cw);

    void conwait_up_clear(struct conwait* cw);
    void conwait_down_clear(struct conwait* cw);

    void conwait_up_write(struct conwait* cw, void* value);
    void conwait_down_write(struct conwait* cw, void* value);

    void* conwait_up_read(struct conwait* cw);
    void* conwait_down_read(struct conwait* cw);
#ifdef __cplusplus
}
#endif

#endif

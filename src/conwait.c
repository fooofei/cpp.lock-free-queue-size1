
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "conwait.h"


// *nix have socketpair() but not on Windows.
// ref mongoose, implement socketpair() by TCP socket.

// socketpair is better than pipe()

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#else
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

// TODO implement it
static void* atomic_write(void** ptr, void* nval)
{
    void* oval = NULL;
    oval = *ptr;
    *ptr = nval;
    return oval;
}

static int set_nonblock(sock_t fd)
{
#ifdef WIN32
    unsigned long on = 1;
    return ioctlsocket(fd, FIONBIO, &on);
#else
    int f = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, f | O_NONBLOCK);
#endif
    
}

int conwait_init(struct conwait* cw)
{
    sock_t listen_sock;
    struct sockaddr_in sin;
    struct sockaddr_in sa;
    struct sockaddr sa2;
    int rc;

    cw->pair[0] = INVALID_SOCKET;
    cw->pair[1] = INVALID_SOCKET;
    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock == INVALID_SOCKET) {
        return -1;
    }

    for (;;) {
        sin.sin_addr.s_addr = htonl(0x7f000001);
        sin.sin_port = 0;
        sin.sin_family = AF_INET;
        rc = bind(listen_sock, (struct sockaddr*) & sin, sizeof(sin));
        if (rc != 0) {
            break;
        }
        rc = listen(listen_sock, 1);
        if (rc != 0) {
            break;
        }
        socklen_t slen = sizeof(sa);
        rc = getsockname(listen_sock, (struct sockaddr*) & sa, &slen);
        if (rc != 0) {
            break;
        }

        cw->pair[0] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (cw->pair[0] == INVALID_SOCKET) {
            break;
        }
        rc = connect(cw->pair[0], (struct sockaddr*) & sa, slen);
        if (rc != 0) {
            break;
        }
        slen = sizeof(sa2);
        cw->pair[1] = accept(listen_sock, (struct sockaddr*) & sa2, &slen);
        if (cw->pair[1] == INVALID_SOCKET) {
            break;
        }
        set_nonblock(cw->pair[0]);
        set_nonblock(cw->pair[1]);
        closesocket(listen_sock);
        return 0;
    }
    conwait_term(cw);
    closesocket(listen_sock);
    listen_sock = INVALID_SOCKET;
    return -1;
}

void conwait_term(struct conwait* cw)
{
    if (cw->pair[0] != INVALID_SOCKET) {
        closesocket(cw->pair[0]);
        cw->pair[0] = INVALID_SOCKET;
    }
    if (cw->pair[1] != INVALID_SOCKET) {
        closesocket(cw->pair[1]);
        cw->pair[1] = INVALID_SOCKET;
    }
    void* p;
    p = atomic_write(&cw->value01, NULL);
    if (p) {
        free(p);
    }
    p = atomic_write(&cw->value10, NULL);
    if (p) {
        free(p);
    }
}

sock_t conwait_up_read_sock(struct conwait* cw)
{
    return cw->pair[0];
}

sock_t conwait_down_read_sock(struct conwait* cw)
{
    return cw->pair[1];
}

void conwait_up_clear(struct conwait* cw)
{
    void* r;
    r = conwait_up_read(cw);
    if (r) {
        free(r);
    }
}

void conwait_down_clear(struct conwait* cw)
{
    void* r;
    r = conwait_down_read(cw);
    if (r) {
        free(r);
    }
}

void conwait_up_write(struct conwait* cw, void* value)
{
    void* oval = NULL;
    uint8_t v = 1;
    int rsize;

    oval = atomic_write(&cw->value01, value);
    if (oval) {
        free(oval);
    }
    errno = 0;
    // Windows cannot use MSG_DONTWAIT instead use nonblock flag
    rsize = (int)send(cw->pair[0], &v, sizeof(v), 0);
    if (rsize <= 0 && errno == EPIPE) {
        // handle pipe error here
    }

}

void* conwait_down_read(struct conwait* cw) 
{
    int rsize;
    char buf[8];
    uint8_t v = 1;
    void* value = NULL;

    for (;;) {
        errno = 0;
        rsize = recv(cw->pair[1], buf, sizeof(buf), 0);
        if (rsize <= 0 ) {
            break;
        }
    }
    return atomic_write(&cw->value01, NULL);
}

void conwait_down_write(struct conwait* cw, void* value)
{
    void* oval = NULL;
    uint8_t v = 1;
    int rsize;

    oval = atomic_write(&cw->value10, value);
    if (oval) {
        free(oval);
    }
    errno = 0;
    // Windows cannot use MSG_DONTWAIT instead use nonblock flag
    // Windows cannot use read() on SOCKET
    rsize = (int)send(cw->pair[1], &v, sizeof(v), 0);
    if (rsize <= 0 && errno == EPIPE) {
        // handle pipe error here
    }
}

void* conwait_up_read(struct conwait* cw)
{
    int rsize;
    char buf[8];
    uint8_t v = 1;
    void* value = NULL;

    for (;;) {
        errno = 0;
        rsize = recv(cw->pair[0], buf, sizeof(buf), 0);
        if (rsize <= 0) {
            break;
        }
    }
    return atomic_write(&cw->value10, NULL);
}

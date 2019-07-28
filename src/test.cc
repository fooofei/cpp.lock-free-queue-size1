
#include <stdio.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <functional>

#ifdef WIN32
#else
#include <sys/select.h>
#endif
extern "C" {
#include "conwait.h"
};


void thread_func1(void* arg)
{

	printf("%s up->down 0->1\n", __func__);
	int cnt;

	struct conwait* cw = (struct conwait*)arg;
	cnt = 0;
	for (;;) {
		cnt++;
		std::this_thread::sleep_for(std::chrono::seconds(3));

		int* value = (int*)malloc(sizeof(int));
		*value = cnt;
		printf("0->1 cnt=%d write\n", cnt);
		conwait_up_write(cw, value);


		fflush(stdout);

	}


}

void thread_func2(void* arg)
{
	printf("%s down->up 1->0\n", __func__);
	int cnt;
	cnt = 0;
	struct conwait* cw = (struct conwait*)arg;

	for (;;) {
		++cnt;
		std::this_thread::sleep_for(std::chrono::seconds(2));

		int* value = (int*)malloc(sizeof(int));
		*value = cnt;
		printf("  1->0 cnt=%d write\n", cnt);
		conwait_down_write(cw, value);

		fflush(stdout);
	}
}

void thread_up_reader(void* arg)
{
	struct conwait* cw = (struct conwait*)arg;
	fd_set readfds;
	int r;
	int nfds;

	for (;;) {

		FD_ZERO(&readfds);

		FD_SET(cw->pair[0], &readfds);
		nfds = (int)cw->pair[0] + 1;

		r = select(nfds, &readfds, NULL, NULL, NULL);
		if (r > 0 && FD_ISSET(cw->pair[0], &readfds)) {
			int* value = (int*)conwait_up_read(cw);
			printf("%s select return %d =%d\n", __func__, r, *value);
			free(value);
		} else {
			printf("%s select return %d \n", __func__, r);
		}


	}


}

void thread_down_reader(void* arg)
{
	struct conwait* cw = (struct conwait*)arg;
	fd_set readfds;
	int r;
	int nfds;

	for (;;) {

		FD_ZERO(&readfds);

		FD_SET(cw->pair[1], &readfds);
		nfds = (int)cw->pair[1] + 1;

		r = select(nfds, &readfds, NULL, NULL, NULL);
		if (r > 0 && FD_ISSET(cw->pair[1], &readfds)) {
			int* value = (int*)conwait_down_read(cw);
			printf("%s select return %d =%d\n", __func__, r, *value);
			free(value);
		} else {
			printf("%s select return %d \n", __func__, r);
		}


	}
}


static void wsa_init()
{
#ifdef WIN32
	WSADATA wsaData;
	WORD socketVersion = MAKEWORD(2, 0);
	WSAStartup(socketVersion, &wsaData);
#endif
}

static void wsa_term()
{
#ifdef WIN32
	WSACleanup();
#endif
}

int main()
{

	struct conwait cw;
	int rc;
	memset(&cw, 0, sizeof(cw));
	wsa_init();
	rc = conwait_init(&cw);
	printf("conwait_init return %d\n", rc);
	if (rc < 0) {
		return 0;
	}

	std::thread th1(thread_func1, &cw);
	std::thread th2(thread_func2, &cw);
	std::thread th3(thread_up_reader, &cw);
	std::thread th4(thread_down_reader, &cw);
	th1.join();
	th2.join();
	th3.join();
	th4.join();

	conwait_term(&cw);
	wsa_term();
	printf("main exit\n");
	return 0;
}

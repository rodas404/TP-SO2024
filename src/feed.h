#ifndef FEED_H
#define FEED_H

#include "utils.h"

void *listen_manager(void *arg);
void handler_sigalrm(int s, siginfo_t *i, void *v);
void handler_sigclose();
#endif
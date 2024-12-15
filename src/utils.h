#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#define TAM_USR_TOP 20
#define TAM_MSG 300
#define MAX_TAM_BUFFER 350

#define SPACE " "
#define NPSERVER "server"
#define NPCLIENT "client_%d"

typedef struct{
    char username[TAM_USR_TOP];
    int pid;
} loginRequest;

typedef struct{
    int type;
    int pid;
    char content[350];
    int exit_reason; // 0: comando "exit", 1: Ctrl+C
} userRequest;


void organizaComando(char *str);

#endif
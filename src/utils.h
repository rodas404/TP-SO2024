#ifndef UTILS_H
#define UTILS_H

#define TAM_USR_TOP 20
#define TAM_MSG 300
#define TAM_BUFFER 350

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

#endif
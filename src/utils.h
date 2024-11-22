#ifndef UTILS_H
#define UTILS_H

#define TAM_USR_TOP 20
#define TAM_MSG 300

#define SPACE " "
#define NPSERVER "server"
#define NPCLIENT "client_%d"

//deve ser só preciso duas estruturas para respostas do servidor, uma com array de topicos e outra apenas com a mensagem
typedef struct{
    char username[TAM_USR_TOP];
    int pid;
} loginRequest;
typedef struct{
    int type;
    int pid;
    char content[350];
} userRequest;

#endif
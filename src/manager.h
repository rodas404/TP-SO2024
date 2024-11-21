#ifndef MANAGER_H
#define MANAGER_H

#include "utils.h"

#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_MSG_PST 5


typedef struct Topico{
    char nome[TAM_TOP];
    int status;
    pMensagens mensagens;
} topico;


typedef struct Mensagem{
    char conteudo[TAM_MSG];
    char autor[TAM_USR];
    int tempExp;
} mensagem, *pMensagens;

typedef struct{
    char username[TAM_USR];
    int pid;
    char np_cliente[50];
} User;

#endif
#ifndef MANAGER_H
#define MANAGER_H

#include "utils.h"

#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_MSG_PST 5

typedef struct {
    char conteudo[TAM_MSG];
    char autor[TAM_USR_TOP];
    int tempExp;
} Mensagem;

typedef struct {
    char topic_name[TAM_USR_TOP];
    int locked; // 0: desbloqueado, 1: bloqueado
    int n_msgs;
    int n_subsTopic; // Número de subscritores ativos
    Mensagem mensagens[MAX_MSG_PST];
} Topic;

typedef struct{
    char username[TAM_USR_TOP];
    int pid;
    char np_cliente[50];
    Topic *subs[MAX_TOPICS];
    int n_subs;
} User;

typedef struct{
    int stop;
    int fs;
    pthread_mutex_t *mutex;
} tdados;



void *listen(void *dados);

int validaComando(char *command);

void acorda(int s, siginfo_t *info, void *c);

#endif
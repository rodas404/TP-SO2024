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




void handle_exit(int pid);
void *listen(void *dados);
void show_messages(char *topico);
void handle_message(userRequest request);
int add_topic(const char *topic);
int armazena_messagem(const char *topic, int duracao, const char *msg, int pid);
char* get_username_by_pid(int pid);
int send_message(const char *fifo_name, const char *message);
void handle_login(userRequest request);
int add_user(char *username, pid_t pid);
void organizaComando(char *str);
void handler_sigalrm(int s, siginfo_t *i, void *v);
void list_users();
void remove_user(const char *username);
void list_topics();
void toggle_topic_lock(const char *topic_name, int lock);
void close_program();
int validaComando(char *command);
void handle_subscribe(userRequest request);
void handle_unsubscribe(userRequest request);
Topic* find_topic(const char *topic_name);
void send_topic_messages(Topic *topic_ptr, int pid);
void acorda(int s, siginfo_t *info, void *c);
void handle_topics(int pid);

#endif
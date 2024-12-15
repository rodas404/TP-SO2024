#ifndef MANAGER_MESSAGES_H
#define MANAGER_MESSAGES_H

#include "manager.h"

// Envia mensagem para um feed específico
int send_message(const char *fifo_name, const char *message);

// Obtém o nome de utilizador através do PID
char* get_username_by_pid(int pid);

// Armazena uma mensagem persistente para um tópico
int armazena_mensagem(const char *topic, int duracao, const char *msg, int pid);

// Recupera e armazena mensagens em memória
int recupera_mensagem(const char *topic, int duracao, const char *msg, char* username);

// Remove uma mensagem de um tópico
void remove_message(Topic *topic, int index);

// Mostra todas as mensagens persistentes de um tópico
void show_messages(char *topico);

// Trata comando das mensagens
void handle_message(userRequest request); 

#endif // MANAGER_MESSAGES_H

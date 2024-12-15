#ifndef MANAGER_TOPICS_H
#define MANAGER_TOPICS_H

#include "manager.h"

/* Função responsável por encontrar um tópico pelo nome */
Topic* find_topic(const char *topic_name);

/* Função responsável por adicionar um novo tópico */
int add_topic(const char *topic);

/* Função responsável por listar todos os tópicos disponíveis */
void list_topics(void);

/* Função para bloquear ou desbloquear um tópico */
void toggle_topic_lock(const char *topic_name, int lock);

/*Função para lidar com o comando "Topics" do feed*/
void handle_topics(int pid); 

/* Função para enviar a lista de tópicos para um feed */
void send_topics(const char *fifo_name);

/* Função para enviar todas as mensagens persistentes de um tópico para um feed */
void send_topic_messages(Topic *topic_ptr, int pid);

/* Função para tratar o pedido de subscrição de um tópico por um utilizador*/
void handle_subscribe(userRequest request);

/* Função para tratar o pedido de cancelamento de subscrição de um tópico */
void handle_unsubscribe(userRequest request);

#endif // MANAGER_TOPICS_H

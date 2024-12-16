#include "manager.h"
#include "manager_messages.h"
#include "manager_topics.h"

extern User users[MAX_USERS];
extern int num_users; 
extern Topic topics[MAX_TOPICS];
extern int topic_count;
extern pthread_mutex_t topic_mutex;
extern char message[350]; 

/* Função responsável por encontrar um tópico pelo nome.*/
Topic* find_topic(const char *topic_name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic_name) == 0) {
            return &topics[i];
        }
    }
    return NULL;
}

/* Função para adicionar um novo tópico.*/
int add_topic(const char *topic) {
    int flag = 1;

    if (topic_count >= MAX_TOPICS) 
        return 0;

    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic) == 0) {
            flag = 0; 
        }
    }

    if (flag) {
        strcpy(topics[topic_count].topic_name, topic);
        topics[topic_count].locked = 0; 
        topics[topic_count].n_msgs = 0;
        topic_count++; 
    }
    return 1;
}

/* Função responsável por listar todos os tópicos disponíveis (manager) */
void list_topics() {
    if(topic_count > 0){
       printf("Tópicos disponíveis:\n");
       for (int i = 0; i < topic_count; i++) {
            printf("- %s: (mensagens: %d)(bloqueado: %s)\n", topics[i].topic_name, topics[i].n_msgs, topics[i].locked ? "sim" : "não");
        }
    }else{
        printf("De momento, não existem tópicos disponíveis.\n");
    }
    
}

/* Função para bloquear ou desbloquear um tópico */
void toggle_topic_lock(const char *topic_name, int lock) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic_name) == 0) {
            topics[i].locked = lock;

            // Notifica os utilizadores sobre o bloqueio/desbloqueio
            sprintf(message, "<MANAGER> O tópico '%s' foi %s.\n", 
                    topic_name, lock ? "bloqueado" : "desbloqueado");

            // Verifica utilizadores subscritos
            for (int j = 0; j < num_users; j++) {
                for (int k = 0; k < users[j].n_subs; k++) {
                    if (users[j].subs[k] == &topics[i]) { 
                        send_message(users[j].np_cliente, message);
                        break;
                    }
                }
            }

            printf("Tópico '%s' foi %s.\n", topic_name, lock ? "bloqueado" : "desbloqueado");
            return;
        }
    }
    printf("Tópico '%s' não encontrado.\n", topic_name);
}

/*Função para lidar com o comando "Topics" do feed*/
void handle_topics(int pid) {
    char npfifo[50];
    sprintf(npfifo, NPCLIENT, pid);
    char message[MAX_TAM_BUFFER];

    //Manda os tópicos
    send_topics(npfifo);
}

/* Função para enviar a lista de tópicos para um feed */
void send_topics(const char *fifo_name) {
    char message[MAX_TAM_BUFFER];
    size_t message_size ; 

    int fd = open(fifo_name, O_WRONLY); 
    if (fd == -1) {
        perror("Erro ao abrir FIFO para envio de tópicos.");
        return;
    }

    if (topic_count <= 0) {
        printf("De momento, não existem tópicos ativos.\n");
        snprintf(message, sizeof(message), "De momento, não existem tópicos ativos.\n");
        if (write(fd, message, strlen(message) + 1) == -1) {
            perror("Erro ao enviar mensagem de tópicos.");
        }
        close(fd);  
        return;
    }

    message_size = snprintf(message, sizeof(message), "Tópicos disponíveis:\n");

    for (int i = 0; i < topic_count; i++) { //A concatenar para mandar a lista de tópicos de uma vez
        message_size += snprintf(message + message_size, sizeof(message) - message_size,
                                  "- %s: (mensagens: %d)(bloqueado: %s)\n",
                                  topics[i].topic_name, topics[i].n_msgs,
                                  topics[i].locked ? "sim" : "não");

        if (message_size >= sizeof(message)) {
            printf("Erro: Tamanho total da mensagem ultrapassou o limite de buffer.\n");
            break;
        }
    }

    if (write(fd, message, message_size + 1) == -1) {
        perror("Erro ao enviar mensagem completa com tópicos.");
        close(fd);
        return;
    }

    if (close(fd) == -1) {
        perror("Erro ao fechar FIFO.");
    }
}

/* Função para enviar todas as mensagens persistentes de um tópico para um feed */
/*void send_topic_messages(Topic *topic_ptr, int pid) { //mesmo problema da função send_topics
    char npfifo[50];
    sprintf(npfifo, NPCLIENT, pid);
    for (int j = 0; j < topic_ptr->n_msgs; j++) {
        sprintf(message, "<%s> %s - %s", topic_ptr->topic_name, topic_ptr->mensagens[j].autor, topic_ptr->mensagens[j].conteudo);
        if (send_message(npfifo, message) == 0) {
            printf("Erro ao enviar mensagem ao user nº%d", pid);
            return;
        }
    }
}*/

/* Função para enviar todas as mensagens persistentes de um tópico para um feed */
void send_topic_messages(Topic *topic_ptr, int pid) { 
    char message[MAX_TAM_BUFFER];
    size_t message_size;

    char npfifo[50];
    sprintf(npfifo, NPCLIENT, pid);

    // Se o tópico não tiver mensagens, informa ao feed
    if (topic_ptr->n_msgs == 0) {
        sprintf(message, "<MANAGER> Subscreveu ao tópico %s com sucesso.\n",topic_ptr->topic_name);

        if (send_message(npfifo, message) == 0) {
            printf("Erro ao enviar mensagem ao user nº%d\n", pid);
            return;
        }
        return;
    }

    message_size = snprintf(message, sizeof(message), "Mensagens do tópico '%s':\n", topic_ptr->topic_name);

    for (int j = 0; j < topic_ptr->n_msgs; j++) {
        message_size += snprintf(message + message_size, sizeof(message) - message_size,
                                  "<%s> %s - %s\n", 
                                  topic_ptr->topic_name, 
                                  topic_ptr->mensagens[j].autor, 
                                  topic_ptr->mensagens[j].conteudo);

        if (message_size >= sizeof(message)) {
            printf("Erro: Tamanho total da mensagem ultrapassou o limite de buffer.\n");
            break;
        }
    }

    // Envia todas as mensagens concatenadas de uma vez
    if (send_message(npfifo, message) == 0) {
        printf("Erro ao enviar mensagens ao user nº%d\n", pid);
        return;
    }
}


/* Função para tratar o pedido de subscrição de um tópico por um utilizador*/
void handle_subscribe(userRequest request) {
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE);

    char npfifo[50];
    sprintf(npfifo, NPCLIENT, request.pid);

    int res = add_topic(topic);
    if (!res) {
        send_message(npfifo, "<MANAGER> Não foi possível adicionar o tópico, dado que foi atingido o número máximo de tópicos.");
        return;
    }

    Topic *topic_ptr = find_topic(topic);
    if (topic_ptr == NULL) {
        printf("Tópico não encontrado.\n");
        send_message(npfifo, "<MANAGER> O Tópico não foi encontrado.");
        return;
    }

    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == request.pid) {
            if (users[i].n_subs >= MAX_TOPICS) {
                printf("Utilizador '%s' atingiu o limite de subscrições.\n", users[i].username);
                send_message(npfifo, "<MANAGER> Não foi possível adicionar o tópico, dado que foi atingido o número máximo de subscrições.");
                return;
            }

            users[i].subs[users[i].n_subs] = topic_ptr;
            users[i].n_subs++;
            
            printf("Utilizador '%s' subscreveu ao tópico '%s'.\n", users[i].username, topic);
           
            // Adiciona +1 subscrição ao tópico
            for (int t = 0; t < topic_count; t++) {
                if (strcmp(topics[t].topic_name, topic) == 0) {
                    topics[t].n_subsTopic++;
                    send_topic_messages(topic_ptr, request.pid);
                    if (topics[t].n_subsTopic == 1)
                        printf("O tópico %s tem agora %d subscrição.\n", topic, topics[t].n_subsTopic);                
                    else 
                        printf("O tópico %s tem agora %d subscrições.\n", topic, topics[t].n_subsTopic);                
                    break; 
                }
            }    

            break;
        }
    }
}

/* Função para tratar o pedido de cancelamento de subscrição de um tópico */
void handle_unsubscribe(userRequest request) {
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE);

    char npfifo[50];
    sprintf(npfifo, NPCLIENT, request.pid);

    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == request.pid) {
            for (int j = 0; j < users[i].n_subs; j++) {
                if (strcmp(users[i].subs[j]->topic_name, topic) == 0) {
                    for (int k = j; k < users[i].n_subs - 1; k++) {
                        users[i].subs[k] = users[i].subs[k + 1];
                    }
                    users[i].n_subs--;

                    printf("Utilizador '%s' removeu a sua subscrição ao tópico '%s'.\n", users[i].username, topic);
                    sprintf(message, "<MANAGER> Removeu a sua subscrição ao tópico %s com sucesso.", topic);
                    send_message(npfifo, message);

                    // Atualiza as subscrições do tópico
                    for (int t = 0; t < topic_count; t++) {
                        if (strcmp(topics[t].topic_name, topic) == 0) {
                            topics[t].n_subsTopic--;
                            
                            // Verifica se o tópico deve ser eliminado (subscrições <= 0 e msgsPersistentes <= 0)
                            if (topics[t].n_subsTopic <= 0 && topics[t].n_msgs <= 0) {
                                printf("Tópico '%s' foi eliminado por não ter mais subscritores nem mensagens persistentes.\n", topics[t].topic_name);

                                // Remove o tópico da lista de tópicos
                                for (int m = t; m < topic_count - 1; m++) {
                                    topics[m] = topics[m + 1];
                                }
                                topic_count--;
                            }
                            break;
                        }
                    }

                    return;
                }
            }
            printf("Utilizador '%s' não está subscrito ao tópico '%s'.\n", users[i].username, topic);
            sprintf(message, "<MANAGER> Atenção: %s não está subscrito ao tópico %s.\n", users[i].username, topic);
            send_message(npfifo, message);
            return;
        }
    }
}

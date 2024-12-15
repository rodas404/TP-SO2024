#include "manager.h"
#include "manager_topics.h"
#include "manager_messages.h"

extern User users[MAX_USERS];
extern int num_users; 
extern Topic topics[MAX_TOPICS];
extern int topic_count;
extern pthread_mutex_t topic_mutex;
extern char message[350]; 

// Envia mensagem para um feed específico
int send_message(const char *fifo_name, const char *message) {
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir FIFO.\n");
        return 0;
    }

    int size = write(fd, message, strlen(message) + 1);
    if (size == -1) {
        perror("Erro ao escrever mensagem.\n");
        close(fd);
        return 0;
    }
    if (close(fd) == -1) {
        perror("Erro ao fechar FIFO.");
        return 0;
    }
    return 1;
}

// Obtém o nome de utilizador através do PID
char* get_username_by_pid(int pid) {
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == pid) {
            return users[i].username;
        }
    }
    return NULL;
}

// Armazena uma mensagem persistente para um tópico (em memória)
int armazena_mensagem(const char *topic, int duracao, const char *msg, int pid) {
    char *username = get_username_by_pid(pid);
    if (username == NULL) {
        printf("Não foi possivel encontrar o username do nº%d.", pid);
        return -1;
    }

    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic) == 0) {
            printf("Topico encontrado %s\n", topics[i].topic_name); 
            if (topics[i].n_msgs >= MAX_MSG_PST) {
                printf("Limite atingido: %d", topics[i].n_msgs); 
                return 0; // Limite de mensagens atingido
            }
            strcpy(topics[i].mensagens[topics[i].n_msgs].conteudo, msg);
            topics[i].mensagens[topics[i].n_msgs].tempExp = duracao;
            strcpy(topics[i].mensagens[topics[i].n_msgs].autor, username);
            topics[i].n_msgs++;
            printf("Msg armazenada %d", topics[i].n_msgs); 
            
            printf("Mensagens persistentes do tópico '%s':\n", topics[i].topic_name);
            for (int j = 0; j < topics[i].n_msgs; j++) {
                printf("- %s: %s (%d)\n", topics[i].mensagens[j].autor, topics[i].mensagens[j].conteudo, topics[i].mensagens[j].tempExp);
            }

            return 1; // Mensagem armazenada com sucesso

           
        }
    }
    return 0; // Tópico não encontrado
}

// Armazena mensagem lida do ficheiro no tópico correspondente
int recupera_mensagem(const char *topic, int duracao, const char *msg, char* username) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic) == 0) {
            if (topics[i].n_msgs >= MAX_MSG_PST) {
                fprintf(stderr, "Erro: Tópico '%s' atingiu o limite máximo de mensagens (%d).\n", topic, MAX_MSG_PST);
                return 0;
            }

            strncpy(topics[i].mensagens[topics[i].n_msgs].conteudo, msg, TAM_MSG - 1);
            topics[i].mensagens[topics[i].n_msgs].conteudo[TAM_MSG - 1] = '\0';

            topics[i].mensagens[topics[i].n_msgs].tempExp = duracao;
            strncpy(topics[i].mensagens[topics[i].n_msgs].autor, username, TAM_USR_TOP - 1);
            topics[i].mensagens[topics[i].n_msgs].autor[TAM_USR_TOP - 1] = '\0';

            topics[i].n_msgs++;
            return 1; // Mensagem recuperada e armazenada com sucesso
        }
    }
    return 0; // Tópico não encontrado
}

// Remove uma mensagem de um tópico
void remove_message(Topic *topic, int index) {
    for (int i = index; i < topic->n_msgs - 1; i++) {
        topic->mensagens[i] = topic->mensagens[i + 1];
    }
    topic->n_msgs--;
}

// Mostra todas as mensagens persistentes de um tópico
void show_messages(char *topico) {
    if(topic_count <= 0){
       printf("De momento, não existe nenhum tópico ativo.\n"); 
       return; 
    }

    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topico) == 0) {
            if(topics[i].n_msgs <= 0){
                printf("De momento, não existem mensagens persistentes no tópico %s.\n", topics[i].topic_name); 
                return; 
            }
            printf("Mensagens persistentes do tópico '%s':\n", topics[i].topic_name);
            for (int j = 0; j < topics[i].n_msgs; j++) {
                printf("- %s: %s (%d)\n", topics[i].mensagens[j].autor, topics[i].mensagens[j].conteudo, topics[i].mensagens[j].tempExp);
            }
            return;
        }
    }
    printf("Tópico '%s' não foi encontrado.\n", topico);
}

// Trata comando das mensagens (do feed)
void handle_message(userRequest request){
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE); 
    int res = add_topic(topic);

    char npfifo[50];
    sprintf(npfifo, NPCLIENT, request.pid);

    if(!res){
        send_message(npfifo, "<MANAGER> Não foi possível enviar a sua mensagem, dado que foi atingido o número máximo de tópicos.");
        return;
    }
    int duracao = atoi(strtok(NULL, SPACE));
    char *msg = strtok(NULL, "\0");
    
    Topic *topic_ptr = find_topic(topic);
    if (topic_ptr == NULL) {
        char npfifo[50];
        snprintf(npfifo, sizeof(npfifo), NPCLIENT, request.pid);
        send_message(npfifo, "<MANAGER> Tópico não encontrado.");
        return;
    }

    if (topic_ptr->locked) {
        char npfifo[50];
        sprintf(npfifo, NPCLIENT, request.pid);
        send_message(npfifo, "<MANAGER> Não foi possivel enviar a sua mensagem, dado que o tópico em causa encontra-se bloqueado.");
        return;
    }
    printf("Duração: %d\n", duracao); 
    if(duracao > 0)
        armazena_mensagem(topic, duracao, msg, request.pid);

    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == request.pid) {
            int already_subscribed = 0;
            for (int j = 0; j < users[i].n_subs; j++) {
                if (users[i].subs[j] == topic_ptr) {
                    already_subscribed = 1;
                    break;
                }
            }
            if (!already_subscribed) {
                if (users[i].n_subs < MAX_TOPICS) {
                    users[i].subs[users[i].n_subs] = topic_ptr;
                    users[i].n_subs++;
                    printf("Utilizador '%s' subscrito ao tópico '%s'.\n", users[i].username, topic);
                    sprintf(message, "<MANAGER> Foi subscrito ao tópico %s.\n", topic);
                    send_message(npfifo, message);
                } else{
                    printf("Utilizador '%s' atingiu o limite de subscrições.\n", users[i].username);
                    sprintf(message, "<MANAGER> Atenção: Atingiu o limite máximo de subscrições.\n");
                    send_message(npfifo, message);
                }
            }
            break;
        }
    }
    
    char *username = get_username_by_pid(request.pid);
    char message[MAX_TAM_BUFFER];
    sprintf(message, "<%s> %s - %s", topic, username, msg);
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid != request.pid) {
            for (int j = 0; j < users[i].n_subs; j++) {
                if (strcmp(users[i].subs[j]->topic_name, topic) == 0) {
                    send_message(users[i].np_cliente, message);
                    break;
                }
            }
        }
    }
    sprintf(message, "<MANAGER> Mensagem para o tópico %s enviada com sucesso .\n", topic);
    send_message(npfifo, message);
}


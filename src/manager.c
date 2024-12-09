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

#include "manager.h"

User users[MAX_USERS]; //Lista de utilizadores
int  num_users = 0; //utilizadores ativos 

Topic topics[MAX_TOPICS];
int topic_count = 0;

pthread_t thread[2]; // a segunda thread será para o contador
tdados td[2]; 
int ready = 0; //Para ver se a thread está pronta

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    close_program();
    if(unlink(NPSERVER) == -1)
        perror("Erro ao remover FIFO do servidor.");
    printf("\nServidor encerrado\n");
    sleep(1);
    exit(1);
}


void acorda(int s, siginfo_t *info, void *c){} // utilizar para fechar thread

// Encerra o programa
void close_program() {
    td[0].stop=1; 
    if(pthread_kill(thread[0], SIGUSR1) != 0)
        perror("Erro ao enviar sinal para a thread.");

    for (int i = 0; i < num_users; i++) {
        send_message(users[i].np_cliente, "<MANAGER> O manager foi encerrado. O programa irá terminar.");
        kill(users[i].pid, SIGINT);
    }
}

//thread para os comandos do feed
void *listen(void *dados) {
    tdados *pdados = (tdados*)dados;

    pdados->fs = open(NPSERVER, O_RDWR);
    if (pdados->fs == -1) {
        perror("Erro ao abrir FIFO do manager");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] Thread de login iniciada\n");
    userRequest request;
    ready = 1;

    do{
       int size = read(pdados->fs, &request, sizeof(userRequest));
            if(size > 0){
                printf("Recebido: %s (%d)\n", request.content, request.type); 
                switch(request.type){
                    case 0: //funcao topics
                            handle_topics(request.pid);
                            break;
                    case 1: //funcao msg  
                            handle_message(request);        
                            break;
                    case 2: //funcao subscribe
                            handle_subscribe(request);
                            break;
                    case 3: //funcao unsubscribe
                            handle_unsubscribe(request);
                            break;
                    case 4:
                        handle_exit(request.pid); 
                        break;
                    case 5: //login    
                        handle_login(request);
                        break; 
                }
            }
    }while(pdados->stop == 0);

    if(close(pdados->fs) == -1)
        perror("Erro ao fechar FIFO");
    pthread_exit(NULL);
}


void handle_topics(int pid) {
    char npfifo[50];
    sprintf(npfifo, NPCLIENT, pid);
    char message[TAM_BUFFER];

    send_message(npfifo, "Tópicos:");
    for (int i = 0; i < topic_count; i++) {
        snprintf(message, sizeof(message), "- %s", topics[i].topic_name);
        if(!send_message(npfifo, message)){
            printf("Erro ao enviar mensagem ao user nº%d", pid);
            exit(EXIT_FAILURE);
        }
    }
}

void organizaComando(char *str) { 
    int n = strlen(str);
    int i, j = 0;
    int spaceFound = 0;

    while (str[j] == SPACE[0]) {
        j++;
    }

    for (i = 0; j < n; j++) {
        if (str[j] != SPACE[0]) {
            str[i++] = str[j];
            spaceFound = 0;
        } else if (!spaceFound) {
            str[i++] = SPACE[0];
            spaceFound = 1;
        }
    }
    if (i > 0 && str[i - 1] == SPACE[0]) {
        i--;
    }

    str[(i == 0 ? i : i-1)] = '\0';
}
// --------- Função para adicionar utilizador
int add_user(char *username, int pid) {
    if (num_users >= MAX_USERS) {
        return -1;  // Limite de utilizadores atingido
    }

    //Adicionar mutex aqui...

    // Verifica se o utilizador já existe
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return -2; //Já existe
        }
    }

    // Se tudo Ok, adiciona o utilizador à lista
    strcpy(users[num_users].username, username);
    users[num_users].pid = pid;
    sprintf(users[num_users].np_cliente, NPCLIENT, pid);
    users[num_users].n_subs = 0;
    num_users++;
    return 0;  // Login realizado com sucesso
}

// Função para tratar o login
void handle_login(userRequest request) {
    char username[TAM_USR_TOP];
    char fifo_name[20];

    // Extrai o nome do utilizador e o nome do FIFO 
    if(sscanf(request.content, "LOGIN %s %s", username, fifo_name) == -1){
        printf("Erro ao extrair o nome de utilizador e fifo.");
        return;
    }

    // Verifica se é possível adicionar user
    int result = add_user(username, request.pid);

    // Abre o FIFO do utilizador para enviar a resposta
    int fd_feed = open(fifo_name, O_WRONLY);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do utilizador");
        return;
    }

    char response[TAM_BUFFER];
    if (result == 0) {
        sprintf(response, "LOGIN_SUCESSO");
    } else if (result == -1) {
        sprintf(response, "LOGIN_ERRO: Limite de utilizadores atingido");
    } else if (result == -2) {
        printf("%s\n", username);
        sprintf(response, "LOGIN_ERRO: Utilizador já inscrito");
    }

    if(write(fd_feed, response, strlen(response) + 1) == -1){
        printf("Erro ao escrever mensagem.\n");
        kill(getpid(), SIGINT);
    }
    if(close(fd_feed) == -1){
        perror("Erro ao fechar FIFO");
        kill(getpid(), SIGINT);
    }

    printf("[INFO:] Utilizador: %s -  %s\n", username, response);
    
}

// Envia mensagem para um feed específico
int send_message(const char *fifo_name, const char *message) {
    int fd = open(fifo_name, O_WRONLY);
    if (fd == -1) {
        perror("Erro ao abrir FIFO.");
        return 0;
    }
    int size = write(fd, message, strlen(message) + 1);
    if(size == -1){
        perror("Erro ao escrever mensagem.");
        return 0;
    }
    if(close(fd) == -1){
        perror("Erro ao fechar FIFO.");
        return 0;    
    }
  return 1;
}

//obtem username atraves do pid
char* get_username_by_pid(int pid) {
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == pid) {
            return users[i].username;
        }
    }
    return NULL;
}

//armazena mensagens persistentes
int armazena_messagem(const char *topic, int duracao, const char *msg, int pid) {
    char *username = get_username_by_pid(pid);
    if(username == NULL){
        printf("Não foi possivel encontrar o username do nº%d.", pid);
        return -1;
    }

    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic) == 0) {
            if (topics[i].n_msgs >= MAX_MSG_PST)
                return 0; 
                
            strcpy(topics[i].mensagens[topics[i].n_msgs].conteudo, msg);
            topics[i].mensagens[topics[i].n_msgs].tempExp = duracao;
            strcpy(topics[i].mensagens[topics[i].n_msgs].autor, username);
            topics[i].n_msgs++;            
        }
    }
    return 1;
}

//adiciona o topico
int add_topic(const char *topic) {
    int flag = 1;
    //verifica limite
    if (topic_count >= MAX_TOPICS) 
        return 0;

    // verifica se ja existe
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic) == 0) {
            flag = 0; 
        }
    }
    if(flag){
        // Adiciona o novo tópico
        strcpy(topics[topic_count].topic_name, topic);
        topics[topic_count].locked = 0; 
        topics[topic_count].n_msgs = 0;
        topic_count++; 
    }
    return 1;
}

//comando das mensagens
void handle_message(userRequest request){
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE); 
    int res = add_topic(topic);
    if(!res){
        char npfifo[50];
        sprintf(npfifo, NPCLIENT, request.pid);
        send_message(npfifo, " <MANAGER> Não foi possível enviar a sua mensagem, dado que foi atingido o número máximo de tópicos.");
        return;
    }
    int duracao = atoi(strtok(NULL, SPACE));
    char *msg = strtok(NULL, "\0");
    
    Topic *topic_ptr = find_topic(topic);
    if (topic_ptr->locked) {
        char npfifo[50];
        sprintf(npfifo, NPCLIENT, request.pid);
        send_message(npfifo, "<MANAGER> Não foi possivel enviar a sua mensagem, dado que o tópico em causa encontra-se bloqueado.");
        return;
    }

    if(duracao > 0)
        armazena_messagem(topic, duracao, msg, request.pid);

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
                } else 
                    printf("Utilizador '%s' atingiu o limite de subscrições.\n", users[i].username);
            }
            break;
        }
    }
    
    char *username = get_username_by_pid(request.pid);
    char message[TAM_BUFFER];
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
}

//comando show
void show_messages(char *topico){
    for(int i=0; i<topic_count; i++){
        if(strcmp(topics[i].topic_name, topico) == 0){
            printf("Mensagens persistentes do tópico '%s':\n", topics[i].topic_name);
            for(int j=0; j<topics[i].n_msgs; j++){
                printf("- %s: %s (%d)\n", topics[i].mensagens[j].autor, topics[i].mensagens[j].conteudo, topics[i].mensagens[j].tempExp);
            }
            return;
        }
    }
    printf("Topico '%s' não foi encontrado.\n", topico);
}



void handle_unsubscribe(userRequest request){
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE);

    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == request.pid) {
            for (int j = 0; j < users[i].n_subs; j++) {
                if (strcmp(users[i].subs[j]->topic_name, topic) == 0) {
                    for (int k = j; k < users[i].n_subs - 1; k++) {
                        users[i].subs[k] = users[i].subs[k + 1];
                    }
                    users[i].n_subs--;
                    printf("Utilizador '%s' removeu a sua subscrição ao tópico '%s'.\n", users[i].username, topic);
                    return;
                }
            }
            printf("Utilizador '%s' não está subscrito ao tópico '%s'.\n", users[i].username, topic);
            return;
        }
    }
}

void handle_subscribe(userRequest request) {
    strtok(request.content, SPACE);
    char *topic = strtok(NULL, SPACE);

    // Adiciona o tópico se não existir
    int res = add_topic(topic);
    if (!res) {
        char npfifo[50];
        sprintf(npfifo, NPCLIENT, request.pid);
        send_message(npfifo, "<MANAGER> Não foi possível adicionar o tópico, dado que foi atingido o número máximo de tópicos.");
        return;
    }

    Topic *topic_ptr = find_topic(topic);
    if(topic_ptr == NULL){
        printf("Topico nao encontrado.\n");
        return;
    }

    // Adiciona o tópico às subscrições do user
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == request.pid) {
            if (users[i].n_subs >= MAX_TOPICS) {
                printf("Utilizador '%s' atingiu o limite de subscrições.\n", users[i].username);
                return;
            }
            users[i].subs[users[i].n_subs] = topic_ptr;
            users[i].n_subs++;
            printf("Utilizador '%s' subscreveu ao tópico '%s'.\n", users[i].username, topic);
            break;
        }
    }

    // Envia todas as mensagens persistentes do tópico ao user
    send_topic_messages(topic_ptr, request.pid);
}


Topic* find_topic(const char *topic_name) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic_name) == 0) {
            return &topics[i];
        }
    }
    return NULL;
}

void send_topic_messages(Topic *topic_ptr, int pid) {
    char npfifo[50];
    sprintf(npfifo, NPCLIENT, pid);
    for (int j = 0; j < topic_ptr->n_msgs; j++) {
        char message[350];
        sprintf(message, "<%s> %s - %s", topic_ptr->topic_name, topic_ptr->mensagens[j].autor, topic_ptr->mensagens[j].conteudo);
        if(send_message(npfifo, message) == 0){
            printf("Erro ao enviar mensagem ao user nº%d", pid);
            return;
        }
    }
}

void handle_exit(int pid) {
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == pid) {
            printf("User '%s' offline.\n", users[i].username);
            kill(users[i].pid, SIGINT);
            for (int j = i; j < num_users - 1; j++) {
                users[j] = users[j + 1];
            }
            num_users--;          
            return;
        }
    }
    printf("Utilizador nº%d não encontrado.\n", pid);
}

// Lida com o comando "users"
void list_users() {
    printf("Utilizadores ativos:\n");
    for (int i = 0; i < num_users; i++) {
        printf("- %s (%d)\n", users[i].username, users[i].pid);
    }
}

// Remove um utilizador
void remove_user(const char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if(send_message(users[i].np_cliente, "<MANAGER> Foste removido pelo administrador. O programa irá encerrar.") == 0)
                printf("Erro ao enviar mensagem ao user '%s'.\n", users[i].username);
            kill(users[i].pid, SIGINT);
            sleep(1);
            printf("Utilizador '%s' removido.\n", users[i].username);

            for (int j = 0; j < num_users; j++) {
                 if (j != i) {
                    char notification[TAM_BUFFER];
                    sprintf(notification, "Utilizador '%s' foi removido.", username);
                    if(send_message(users[j].np_cliente, notification) == 0)
                        printf("Erro ao enviar notificação ao user '%s'.\n", users[j].username);
                 }
             }
    
            // Remove o utilizador da lista
            for (int j = i; j < num_users - 1; j++) {
                users[j] = users[j + 1];
            }
            num_users--;
            return;
        }
    }
    printf("Utilizador '%s' não encontrado.\n", username);
}

// Comando "topics"
void list_topics() {
    printf("Tópicos disponíveis:\n");
    for (int i = 0; i < topic_count; i++) {
        printf("- %s (bloqueado: %s)\n", topics[i].topic_name, topics[i].locked ? "sim" : "não");
    }
}

// Bloqueia ou desbloqueia um tópico
void toggle_topic_lock(const char *topic_name, int lock) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic_name, topic_name) == 0) {
            topics[i].locked = lock;
            printf("Tópico '%s' foi %s.\n", topic_name, lock ? "bloqueado" : "desbloqueado");
            return;
        }
    }
    printf("Tópico '%s' não encontrado.\n", topic_name);
}



//comandos manager
int validaComando(char *command){
    const char *comms_list[] = {"users", "remove", "topics", "show", "lock", "unlock", "close"};
    const char *comms_guide[] = {"users", "remove <username>", "topics", "show <topico>", "lock <topico>", "unlock <topico>", "close"};
    int num_comms = sizeof(comms_list) / sizeof(comms_list[0]);
    char *token;

    char command_copy[30];
    strcpy(command_copy, command);
    command_copy[sizeof(command_copy) - 1] = '\0';

    token = strtok(command_copy, SPACE);
    int ind, flag = 0;

    for (ind = 0; ind < num_comms; ind++) {
        if (strcmp(token, comms_list[ind]) == 0){
            flag = 1;
            break;
        } 
    }

    if(flag == 0){
        printf("Comando invalido!\n");
        printf("Comandos disponiveis:\n");
        for(ind = 0; ind < num_comms; ind++)
            printf("- %s\n", comms_guide[ind]);   
        return -1;
    }

    switch(ind){
        case 0: //users
            list_users();
            break;
        case 1: //remove
            token = strtok(NULL, SPACE); //topic
            if (token == NULL){ 
                flag = 0;
                break;
            }
            remove_user(token);
            break;
        case 2: //topics
            list_topics();
            break;
        case 3: //show
            token = strtok(NULL, SPACE);
            if (token == NULL){ 
                flag = 0;
                break;
            }
            show_messages(token);
            break; 
        case 4: //lock
            token = strtok(NULL, SPACE);
            if (token == NULL){ 
                flag = 0;
                break;
            }
            toggle_topic_lock(token, 1);
            break;
        case 5: //unlock
            token = strtok(NULL, SPACE);
            if (token == NULL){ 
                flag = 0;
                break;
            }
            toggle_topic_lock(token, 0);
            break;
        case 6: //close
            kill(getpid(), SIGINT);
            break; 
    }

    if(!flag){ 
        printf("Sintaxe incorreta!\n");
        printf("Instruções: %s\n", comms_guide[ind]);
        return -1;
    }
    return ind;
}

int main(){

    setbuf(stdout, NULL);
    
    int fs;
    char comando[30];

    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction act;
    act.sa_sigaction=acorda;
  	sigaction(SIGUSR1,&act,NULL); //sinal para fechar thread

    if (access(NPSERVER, F_OK) != 0) {
        if (mkfifo(NPSERVER, 0666) == -1) {
            perror("Erro ao abrir servidor");
            exit(EXIT_FAILURE);
        } 
        else 
            printf("Servidor aberto com sucesso.\n");
    } else {
        printf("Servidor já se encontra aberto\n");
        exit(EXIT_FAILURE);
    }

    
    pthread_mutex_t mutex;
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Erro ao inicializar o mutex");
        exit(EXIT_FAILURE);
    }

    td[0].stop = 0;
    td[0].mutex = &mutex;

    if (pthread_create(&thread[0], NULL, listen, &td[0]) != 0) {
        perror("Erro ao criar thread de login\n");
        close(fs);
        exit(EXIT_FAILURE);
    }

    // Aguarda pela thread antes de continuar
    while (!ready) sleep(1);

    printf("[INFO] Servidor pronto para receber logins\n"); //para debug 
     while (1) {
        printf("> ");
            if (fgets(comando, sizeof(comando), stdin)) {
                organizaComando(comando);
                validaComando(comando);
            }  
    }
   
    // Encerra thread de login
    if(pthread_join(thread[0], NULL) != 0) {
        perror("Erro ao juntar a thread de notificações");
        exit(EXIT_FAILURE);
    }
    if(close(fs) == -1){
        perror("Erro ao fechar FIFO");
        exit(EXIT_FAILURE);
    }
    if(unlink(NPSERVER) == -1)
        perror("Erro ao remover FIFO do servidor."); 
    return 0;
}

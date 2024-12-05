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

#define MAX_BUFFER 256

User users[MAX_USERS]; //Lista de utilizadores
int  num_users = 0; //utilizadores ativos 

int ready; //Para ver se a thread está pronta

Topic topics[MAX_TOPICS];
int topic_count = 0;

userRequest request;

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    unlink(NPSERVER);
    printf("\nServidor encerrado\n");
    exit(1);
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
// ------------------------------------------------ Função para adicionar utilizador
int add_user(char *username, pid_t pid) {
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
    num_users++;
    return 0;  // Login realizado com sucesso
}

// ---------------------------------------------- Função para tratar o login
void handle_login(userRequest request) {
    char username[50];
    char fifo_name[100];

    // Extrai o nome do utilizador e o nome do FIFO 
    sscanf(request.content, "LOGIN %s %s", username, fifo_name);

    // Verifica se é possível adicionar user
    int result = add_user(username, request.pid);

    // Abre o FIFO do utilizador para enviar a resposta
    int fd_feed = open(fifo_name, O_WRONLY);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do utilizador");
        return;
    }

    char response[MAX_BUFFER];
    if (result == 0) {
        snprintf(response, MAX_BUFFER, "LOGIN_SUCESSO");
    } else if (result == -1) {
        snprintf(response, MAX_BUFFER, "LOGIN_ERRO: Limite de utilizadores atingido");
    } else if (result == -2) {
        printf("%s\n", username);
        snprintf(response, MAX_BUFFER, "LOGIN_ERRO: Utilizador já inscrito");
    }

    write(fd_feed, response, strlen(response) + 1);
    close(fd_feed);

    printf("[INFO:] Utilizador: %s -  %s\n", username, response);
    
}

/*
void handle_login(char *message) {
    char username[50];
    char fifo_name[100];

    // Extrai o nome do utilizador e o nome do FIFO 
    sscanf(message, "LOGIN %s %s", username, fifo_name);

    // Verifica se é possível adicionar user
    int result = add_user(username, getpid());

    // Abre o FIFO do utilizador para enviar a resposta
    int fd_feed = open(fifo_name, O_WRONLY);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do utilizador");
        return;
    }

    char response[MAX_BUFFER];
    if (result == 0) {
        snprintf(response, MAX_BUFFER, "LOGIN_SUCESSO");
    } else if (result == -1) {
        snprintf(response, MAX_BUFFER, "LOGIN_ERRO: Limite de utilizadores atingido");
    } else if (result == -2) {
        snprintf(response, MAX_BUFFER, "LOGIN_ERRO: Utilizador já inscrito");
    }

    write(fd_feed, response, strlen(response) + 1);
    close(fd_feed);

    printf("[INFO:] Utilizador: %s -  %s\n", username, response);
    
}
*/

// ---------------------------------------------- Função para mensagens recebidas do cliente
/*void handle_message(const char* buffer) { //Percebi que dava para enviar o login como request, para evitar fazer isso
    if (strncmp(buffer, "LOGIN:", 6) == 0) {
        // É login
        handle_login(buffer);
    } else if (strncmp(buffer, "COMANDO:", 8) == 0) {
        // É um comando normal
        handle_command(buffer + 8); // Passa o comando sem "COMANDO"
    } else { // Mensagem desconhecida ou algo parecido     
        fprintf(stderr, "Mensagem desconhecida: %s\n", buffer);
    }
}*/

void *listen_logins(void *arg) {
    int fs = *((int*)arg);
    char buffer[MAX_BUFFER];

    printf("[INFO] Thread de login iniciada\n");
    ready = 1; // Thread pronta

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fs, &readfds);

        // Timeout de 1 segundo para dar tempo
        struct timeval timeout = {1, 0};
        int retval = select(fs + 1, &readfds, NULL, NULL, &timeout); //Problema da thread foi resolvido com o select

        if (retval > 0 && FD_ISSET(fs, &readfds)) {
           /*int size = read(fs, buffer, sizeof(buffer) - 1);
            if (size > 0) {
                buffer[size] = '\0'; 
                handle_login(buffer);
            }*/ 
            int size = read(fs, &request, sizeof(userRequest));
            if(size > 0){
                handle_login(request); 
            }
        }
    }

    pthread_exit(NULL);
}

// Envia mensagem para um feed específico
void send_message(const char *fifo_name, const char *message) {
    int fd = open(fifo_name, O_WRONLY);
    if (fd != -1) {
        write(fd, message, strlen(message) + 1);
        close(fd);
    }
}

// Lida com o comando "users"
void list_users() {
    printf("Utilizadores ativos:\n");
    for (int i = 0; i < num_users; i++) {
        printf("- %s\n", users[i].username);
    }
}

// Remove um utilizador
void remove_user(const char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            send_message(users[i].np_cliente, "Foste removido pelo administrador. O programa irá encerrar.");
            sleep(2);
            kill(users[i].pid, SIGINT);
            printf("Utilizador '%s' removido.\n", users[i].username);

            for (int j = 0; j < num_users; j++) {
                 if (j != i) {
                    char notification[MAX_BUFFER];
                     snprintf(notification, MAX_BUFFER, "Utilizador '%s' foi removido.", username);
                    send_message(users[j].np_cliente, notification);
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

// Encerra o programa
void close_program() {
    for (int i = 0; i < num_users; i++) {
        send_message(users[i].np_cliente, "O manager foi encerrado. O programa irá terminar.\n");
        kill(users[i].pid, SIGUSR1);
    }
}


int validaComando(char *command){
    const char *comms_list[] = {"users", "remove", "topics", "show", "lock", "unlock", "close"};
    const char *comms_guide[] = {"users", "remove <username>", "topics", "show <topico>", "lock <topico>", "unlock <topico>", "close"};
    int num_comms = sizeof(comms_list) / sizeof(comms_list[0]);
    char *token;

    char command_copy[350];
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
            printf("Entrou aqui\n"); 
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
            break; 
        case 4: //lock
          toggle_topic_lock(token, 1);
        case 5: //unlock
            token = strtok(NULL, SPACE);
            if (token == NULL) flag = 0;
            toggle_topic_lock(token, 0);
            break;
        case 6: //close
            close_program(); 
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
    
    int fs, fc, size;
    char comando[350];
    char buffer[MAX_BUFFER];

    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    if (access(NPSERVER, F_OK) != 0) {
        if (mkfifo(NPSERVER, 0666) == -1) {
            perror("Erro ao abrir servidor");
            exit(EXIT_FAILURE);
        } 
        else {
            printf("Servidor aberto com sucesso.\n");
        }
    } else {
        printf("Servidor já se encontra aberto\n");
        exit(EXIT_FAILURE);
    }

    //Abrir FIFO do Manager
    fs = open(NPSERVER, O_RDWR);
    if (fs == -1) {
        perror("Erro ao abrir FIFO do manager");
        exit(EXIT_FAILURE);
    }


    ready = 0; 
    //Criacao da thread para estar sempre a aceitar logins 
    pthread_t login_thread;
    if (pthread_create(&login_thread, NULL, listen_logins, &fs) != 0) {
        perror("[Erro] Erro ao criar thread de login\n");
        close(fs);
        exit(EXIT_FAILURE);
    }

    // Aguarda pela thread antes de continuar
     while (!ready) sleep(1);

    printf("[INFO] Servidor pronto para receber logins\n"); //para debug 

    // Loop principal para comandos do admin e mensagens dos cientes
    fd_set readfds;

     while (1) {
        FD_ZERO(&readfds);
        FD_SET(fs, &readfds);  // FIFO do servidor
        FD_SET(STDIN_FILENO, &readfds);  // Entrada padrão (admin)

        int max_fd = fs > STDIN_FILENO ? fs : STDIN_FILENO;

        // O select vai verificar a atividade nos descritores (stdin ou fifo)
        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("[Erro] Erro no select do loop");
            break;
        }

        // Verifica logins ou as mensagens dos clientes
        if (FD_ISSET(fs, &readfds)) {
            size = read(fs, &request, sizeof(userRequest));
            if(size > 0){
                printf("Mensagem Recebida: %s\n", request.content); 
                switch(request.type){
                    case 1: //funcao topics           
                            break;
                    case 2: //funcao msg
                            break;
                    case 3: //funcao subscribe
                            break;
                    case 4: //funcao unsubscribe
                            break;
                    case 5: //login    
                        handle_login(request);
                        break; 
                }
            }
        }

        // Verifica os comando do admin
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            printf("> ");
            if (fgets(comando, sizeof(comando), stdin)) {
                organizaComando(comando);
                validaComando(comando);
            }
        }
    }
    
   /* while(1){
        size = read(fs, &request, sizeof(userRequest));
        if(size > 0){
            switch(request.type){
                case 1: //funcao topics
                        break;
                case 2: //funcao msg
                        break;
                case 3: //funcao subscribe
                        break;
                case 4: //funcao unsubscribe
                        break;
            }
        }

        printf("comando server: ");
        fgets(comando, sizeof(comando), stdin); //isto ta com buffer overflow, sendo que nem é suposto ter mas prontos
        organizaComando(comando);
        validaComando(comando);

    }*/

    // Encerra thread de login
    pthread_join(login_thread, NULL);

    close(fs);
    unlink(NPSERVER); 
    return 0;
}
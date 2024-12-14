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

#include "feed.h"

int running = 1;

int exiting = 0; 
volatile sig_atomic_t communicated_exit = 0;

char npCliente[50];

// --------- Thread para receber mensagens do manager
void *listen_manager(void *arg) {
    int fd_feed = open(npCliente, O_RDWR);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do feed\n");
        exit(1);
    }

    char buffer[TAM_BUFFER];
    while (running && read(fd_feed, buffer, sizeof(buffer)) > 0) {
        printf("\n%s\n", buffer);
    }
    if(close(fd_feed) == -1)
        perror("Erro ao fechar FIFO do cliente."); 
    pthread_exit(NULL);
}

//---------- Signal para terminar o feed quando o manager terminar
void handle_signal_close(int sig) {
    if (sig == SIGUSR1) {
        printf("\nNotificação: O manager encerrou. A terminar o feed...\n");
        running = 0;
        
        printf("Fechei exit: %d", exiting); 
    }
}

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    if (exiting==1){
        printf("Exiting"); 
        return; 
    }

   if (i->si_pid == getpid()){
      userRequest request;
      request.type = 4;  //"exit"
      request.pid = getpid();
      request.exit_reason = 1; //com sigint 
      snprintf(request.content, sizeof(request.content), "exit");

      int fs = open(NPSERVER, O_WRONLY);
      if (fs != -1) {
          write(fs, &request, sizeof(userRequest));
          close(fs);
      } else {
        perror("Erro ao comunicar saída ao servidor.");
       }
      
    }else{
        exiting = 1; 
    }

   if(unlink(npCliente) == -1)
        perror("Erro ao remover FIFO do cliente.");
      printf("\nAté à proxima!\n");
      sleep(1);
      exit(1);
}

int validaComando(char *command){
    const char *comms_list[] = {"topics", "msg", "subscribe", "unsubscribe", "exit"};
    const char *comms_guide[] = {"topics", "msg <topico> <duração> <mensagem>", "subscribe <topico>", "unsubscribe <topico>", "exit"};
    int num_comms = sizeof(comms_list) / sizeof(comms_list[0]);
    char *token;

    char command_copy[TAM_BUFFER];
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

    if(!flag){
        printf("Comando invalido!\n");
        printf("Comandos disponiveis:\n");
        for(ind = 0; ind < num_comms; ind++)
            printf("- %s\n", comms_guide[ind]);   
        return -1;
    }

    switch(ind){
        case 0: //topics
            break;
        case 1: //msg
            token = strtok(NULL, SPACE); //topic
            if (token == NULL) {
                flag = 0;
                break;
            }
            else if(strlen(token) > TAM_USR_TOP){
                flag = -1;
                break;
            }
            token = strtok(NULL, SPACE); //duracao
            if (token == NULL) {
                flag = 0;
                break;
            }
            if(atoi(token) == 0 && strcmp(token, "0") != 0) {
                flag = 0;
                break;
            }
            token = strtok(NULL, "\0"); //msg
            if(token == NULL)
                flag = 0;
            else if (strlen(token) > TAM_MSG)
                flag = -2;    
            break;
        case 2: //subscribe
        case 3: //unsubscribe
            token = strtok(NULL, SPACE);
            if (token == NULL) flag = 0;
            else if(strlen(token) > TAM_USR_TOP) flag = -2;
            break;
        case 4: //exit
            running = 0;
            exiting = 1; 
            printf("Mudei exit: %d", exiting); 
            break;
    }

    if(flag == 1){
        return ind;
    }
    else{
        if(flag == 0){
            printf("Sintaxe incorreta!\n");
            printf("Instruções: %s\n", comms_guide[ind]);
        }
        else if(flag == -1)
            printf("Excedido limite de caracteres (%d) para o tópico.\n", TAM_USR_TOP);
        else if(flag == -2)
            printf("Excedido limite de caracteres (%d) para a mensagem.\n", TAM_MSG);
        
        return -1;
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

int main(int argc, char *argv[]){

    setbuf(stdout, NULL);

    userRequest request;
    loginRequest login; 

    int fs, fc, size, nBytes;
    char comando[TAM_BUFFER];

    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);


    /*Função para verficar se o manager está em execução */
	if(access(NPSERVER, F_OK) != 0){
        printf("[ERRO] O manager não se encontra em execução. \n");
        exit(2);
	}

    /* Inicializar o feed (login) */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <username>\n", argv[0]);
        exit(1);
    }
 
    // Sinal para notificações do manager
    signal(SIGUSR1, handle_signal_close);

    strcpy(login.username,argv[1]);
    login.pid = getpid();
	sprintf(npCliente,NPCLIENT,login.pid);

    // se access != 0, FIFO nao existe, entao cria o fifo com pid
	if(access(npCliente,F_OK) != 0){
	   mkfifo(npCliente, 0600);
	}
    else{
        perror("Erro ao criar FIFO");
        exit(EXIT_FAILURE);
    }

    /* Enviar login para o manager */  
    fs = open(NPSERVER, O_WRONLY);
    if (fs == -1) {
        perror("Erro ao conectar ao manager\n");
        exit(1);
    }

     printf("[INFO] Conectado ao servidor.\n"); //debug

   /*
    char init_message[MAX_BUFFER];
    snprintf(init_message, MAX_BUFFER, "LOGIN %s %s", login.username, npCliente);
    write(fs, init_message, strlen(init_message) + 1);
    */
    request.type = 5; 
    request.pid = getpid(); 
    snprintf(request.content, sizeof(request.content), "LOGIN %s %s", login.username, npCliente);

    if (write(fs, &request, sizeof(userRequest)) == -1) {
        perror("Erro ao enviar login para o servidor\n");
        if(close(fs) == -1)
            perror("Erro ao fechar FIFO do servidor.");
        if(unlink(npCliente) == -1)
            perror("Erro ao remover FIFO do cliente.");
        exit(1);
    }


    /*Lê a resposta do manager*/
    fc = open(npCliente, O_RDONLY);
    if (fc == -1) {
        perror("Erro ao abrir FIFO exclusivo do cliente.");
        exit(1);
    }

    char response[TAM_BUFFER];
    if(read(fc, response, sizeof(response)) == -1){
        perror("Erro ao ler resposta do servidor.");
        exit(1);
    }

    if (strcmp(response, "LOGIN_SUCESSO") != 0) {
        printf("Erro: %s\n", response);
        if(close(fs) == -1) 
            perror("Erro ao fechar o FIFO do servidor");
        if (unlink(npCliente) == -1) 
            perror("Erro ao remover o FIFO do cliente.");
        exit(1);
    }
    printf("%s\n", response); 
    
    if(close(fc) == -1){
        perror("Erro ao fechar FIFO exclusivo do cliente.");
        exit(1);
    }

    printf("Seja bem-vindo(a) '%s'. Programa pronto para comandos.\n", login.username);

    // Thread para ler mensagens do manager
    pthread_t listener_thread;
    if(pthread_create(&listener_thread, NULL, listen_manager, NULL) != 0){
        perror("Erro ao criar thread de notificações.");
        exit(1);
    }


    do{
    printf("> ");
    memset(comando, 0, sizeof(comando));
    if (fgets(comando, sizeof(comando), stdin)) {
                organizaComando(comando);
                request.type = validaComando(comando);
            }
    if(request.type != -1){
        request.pid = getpid();
        strcpy(request.content, comando);

       if (request.type == 4){
            request.exit_reason = 0; //exit
            exiting = 1; 
        }
        fs = open(NPSERVER, O_WRONLY);
        if(fs == -1){
            printf("Erro ao conectar com o servidor.\n");
            exit(EXIT_FAILURE);
        }

        nBytes = write(fs, &request, sizeof(userRequest));
        if (nBytes == -1) {
            perror("Erro ao escrever no FIFO do servidor");
            close(fs);
            exit(EXIT_FAILURE);
        }

        if (close(fs) == -1) {
            perror("Erro ao fechar o FIFO do servidor");
            exit(EXIT_FAILURE);
        }
    }
    }while(running);


    if(pthread_join(listener_thread, NULL) != 0) {
        perror("Erro ao juntar a thread de notificações");
        exit(EXIT_FAILURE);
    }

    if(close(fc) == -1){
        perror("Erro ao fechar o FIFO do cliente.");
        exit(EXIT_FAILURE);
    }
  
    if (unlink(npCliente) == -1) {
        perror("Erro ao remover o FIFO do cliente.");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}
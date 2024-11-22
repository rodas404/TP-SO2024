#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "feed.h"

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    printf("\nAté à proxima!\n");
    exit(1);
}

int validaComando(char *command){
    const char *comms_list[] = {"topics", "msg", "subscribe", "unsubscribe", "exit"};
    const char *comms_guide[] = {"topics", "msg <topico> <duração> <mensagem>", "subscribe <topico>", "unsubscribe <topico>", "exit"};
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
        case 0: //topics
            break;
        case 1: //msg
            token = strtok(NULL, SPACE); //topic
            if (token == NULL) {
                flag = 0;
                break;
            }
            else if(strlen(token) > TAM_USR_TOP){
                flag = 2;
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
                flag = 3;    
            break;
        case 2: //subscribe
        case 3: //unsubscribe
            token = strtok(NULL, SPACE);
            if (token == NULL) flag = 0;
            else if(strlen(token) > TAM_USR_TOP) flag = 2;
            break;
        case 4: //exit
            kill(getpid(), SIGINT);
    }

    switch(flag){ //vou mudar isto depois que isto ta feio
        case 0:
            printf("Sintaxe incorreta!\n");
            printf("Instruções: %s\n", comms_guide[ind]);
            break;
        case 1: 
            return ind;
        case 2: 
            printf("Excedido limite de caracteres (%d) para o tópico.\n", TAM_USR_TOP);
            break;
        case 3: 
            printf("Excedido limite de caracteres (%d) para a mensagem.\n", TAM_MSG);
            break;
    }
    return -1;
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
    char npCliente[50];
    int fs, fc, size, nBytes;
    char comando[350];

    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    do{
    printf("comando: ");
    fgets(comando, sizeof(comando), stdin); //isto ta com buffer overflow, sendo que nem é suposto ter mas prontos
    organizaComando(comando);
    
    request.type = validaComando(comando);

    if(request.type != -1){
        request.pid = getpid();
        strcpy(request.content, comando);

        fs = open(NPSERVER, O_WRONLY);
        if(fs == -1){
            printf("Erro ao conectar com o servidor.\n");
            exit(EXIT_FAILURE);
        }

        nBytes = write(fs, &request, sizeof(userRequest));
        close(fs);

    }
    }while(1);
    return 0;
}
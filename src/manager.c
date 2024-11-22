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

#include "manager.h"

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    unlink(NPSERVER);
    printf("Servidor encerrado\n");
    exit(1);
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
            break;
        case 1: //remove
            token = strtok(NULL, SPACE); //topic
            if (token == NULL) flag = 0;
            break;
        case 2: //topics
                break;
        case 3: //show
        case 4: //lock
        case 5: //unlock
            token = strtok(NULL, SPACE);
            if (token == NULL) flag = 0;
            break;
        case 6: //close
            kill(getpid(), SIGINT);
    }

    if(!flag){ 
        printf("Sintaxe incorreta!\n");
        printf("Instruções: %s\n", comms_guide[ind]);
        return -1
    }
    return ind;
}

int main(){
    userRequest request;
    int fs, fc, size;

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

    fs = open(NPSERVER, O_RDONLY);
    while(1){
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
    }
    close(fs);
    return 0;
}
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


int validaComando(char *command){
    const char *comms_list[] = {"topics", "msg", "subscribe", "unsubscribe", "exit"};
    const char *comms_guide[] = {"topics", "msg <topico> <duração> <mensagem>", "subscribe <topico>", "unsubscribe <topico>", "exit"};
    int num_comms = sizeof(comms_list) / sizeof(comms_list[0]);
    char *token;
    token = strtok(command, SPACE);
    int i, flag = 0;

    for (i = 0; i < num_comms; i++) {
        if (strcmp(token, comms_list[i]) == 0){
            flag = 1;
            break;
        } 
    }

    if(!flag){
        printf("Comando invalido!\n");
        printf("Comandos disponiveis:\n");
        for(i = 0; i < num_comms; i++)
            printf("- %s\n", comms_guide[i]);   
        return 0;
    }

    switch(i){
        case 0: //topics
            break;
        case 1: //msg
            token = strtok(NULL, SPACE); //topic
            if (token == NULL) {
                flag = 0;
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
            token = strtok(NULL, ""); //msg
            if(token == NULL)
                flag = 0;
            else{
                for(char *aux = token; *aux != '\0'; aux++){ //ciclo para perceber se o token não são apenas espaços
                    if(*aux != SPACE[0]){ // space é uma string, space[0] é apenas um caracter
                        flag = 1;
                        break;
                    }
                    flag = 0;
                }
            }
            break;
        case 2: //subscribe
        case 3: //unsubscribe
            token = strtok(NULL, SPACE);
            if (token == NULL) flag = 0;
            break;
        case 4: break;
    }
    if(!flag){
        printf("Sintaxe incorreta!\n");
        printf("Instruções: %s\n", comms_guide[i]);
        return 0;
    }
    printf("certissimo senhor\n");
    return 1;
}

int main(int argc, char *argv[]){
    printf("comando: ");
    char comando[400];
    do{
    fgets(comando, 399, stdin);
    comando[strlen(comando)-1] = '\0';
    validaComando(comando);
    }while(strcmp(comando, "sai") !=0);
    return 0;
}
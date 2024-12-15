#include "feed_comandos.h"

extern int running;


int validaComando(char *command){
    const char *comms_list[] = {"topics", "msg", "subscribe", "unsubscribe", "exit"};
    const char *comms_guide[] = {"topics", "msg <topico> <duração> <mensagem>", "subscribe <topico>", "unsubscribe <topico>", "exit"};
    int num_comms = sizeof(comms_list) / sizeof(comms_list[0]);
    char *token;

    char command_copy[MAX_TAM_BUFFER];
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
            else if(strlen(token) > TAM_USR_TOP) flag = -1;
        
            token = strtok(NULL, SPACE); //Caso o topico tenha mais do que 1 palavra 
            if (token != NULL) {
                printf("O comando '%s' só pode ter 1 palavra (para o tópico).\n", comms_list[ind]);
                flag = 0;
                break;
            }
            break;
        case 4: //exit
            running = 0;
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
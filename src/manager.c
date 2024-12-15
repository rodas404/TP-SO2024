#include "manager_users.h"
#include "manager_topics.h"
#include "manager_messages.h"
#include "manager_lifecycle.h" 
#include "manager.h"

User users[MAX_USERS]; //Lista de utilizadores
int  num_users = 0; //utilizadores ativos 

Topic topics[MAX_TOPICS];
int topic_count = 0;

pthread_t thread[2]; // a segunda thread será para o contador
tdados td[2]; 
int ready = 0; //Para ver se a thread está pronta

char message[350];

pthread_mutex_t topic_mutex = PTHREAD_MUTEX_INITIALIZER;

char msg_file_path[256] = ""; // Caminho para o ficheiro de mensagens

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
                        handle_exit(request.pid, request.exit_reason); 
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

void acorda(int s, siginfo_t *info, void *c){} // utilizar para fechar thread

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

    initialize_manager();
   
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

    //criação da thread do ciclo de vida das msgs 
    pthread_t lifecycle_thread;
    pthread_create(&lifecycle_thread, NULL, manage_message_lifecycle, NULL);


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

    finalize_manager();
   
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

#include "manager.h"
#include "manager_lifecycle.h"
#include "manager_messages.h"
#include "manager_users.h"

extern User users[MAX_USERS]; 
extern int num_users; 
extern pthread_t thread[2]; // a segunda thread será para o contador
extern tdados td[2]; 
extern int ready; //Para ver se a thread está pronta

extern char message[350];

extern pthread_mutex_t topic_mutex;

// Função para adicionar um utilizador
int add_user(char *username, int pid) {
    if (num_users >= MAX_USERS) {
        return -1;  // Limite de utilizadores atingido
    }

    // Verifica se o utilizador já existe
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return -2; // Já existe
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

// Função de login
void handle_login(userRequest request) {
    char username[TAM_USR_TOP];
    char fifo_name[20];

    // Extrai o nome do utilizador e o nome do FIFO 
    if(sscanf(request.content, "LOGIN %s %s", username, fifo_name) == -1){
        printf("Erro ao extrair o nome de utilizador e fifo.\n");
        return;
    }

    // Verifica se é possível adicionar o utilizador
    int result = add_user(username, request.pid);

    // Envia a resposta para o utilizador
    int fd_feed = open(fifo_name, O_WRONLY);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do utilizador");
        return;
    }

    char response[MAX_TAM_BUFFER];
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

// Função para tratar a saída do utilizador
void handle_exit(int pid, int exit_reason) {
    printf("Entrei para o pid: %d\n", pid); 
    for (int i = 0; i < num_users; i++) {
        if (users[i].pid == pid) {
            printf("User '%s' offline.\n", users[i].username);
            printf("Saída: %d", exit_reason); 
            if(exit_reason == 0){
                if (kill(users[i].pid, SIGINT) == -1) {
                    perror("Erro ao enviar sinal\n");
                }
               //kill(users[i].pid, SIGINT);
               printf("mandei sair %d", users[i].pid); 
            }                  

            // Remove o utilizador da lista
            for (int j = i; j < num_users - 1; j++) {
                users[j] = users[j + 1];
            }
            num_users--; 
            return;
        }
    }
    printf("Utilizador nº%d não encontrado.\n", pid);
}

// Lida com o comando "users" do manager
void list_users() {
    if(num_users > 0){
       printf("Utilizadores ativos:\n");
       for (int i = 0; i < num_users; i++) {
            printf("[%d]- %s (%d)\n", i + 1, users[i].username, users[i].pid);
        }
    }else{
        printf("De momento, não existem utilizadores ativos.\n"); 
    }
    
}

// Remove um utilizador
void remove_user(const char *username) {
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (send_message(users[i].np_cliente, "<MANAGER> Foste removido pelo administrador. O programa irá encerrar.") == 0)
                printf("Erro ao enviar mensagem ao user '%s'.\n", users[i].username);
            
            if (kill(users[i].pid, SIGINT) == -1) {
                perror("Erro ao enviar sinal para terminar o processo");
            }

            waitpid(users[i].pid, NULL, 0);

            printf("Utilizador '%s' removido.\n", users[i].username);

            // Notifica os outros utilizadores sobre a remoção
            for (int j = 0; j < num_users; j++) {
                if (j != i) {
                    char notification[MAX_TAM_BUFFER];
                    sprintf(notification, "Utilizador '%s' foi removido.", username);
                    if (send_message(users[j].np_cliente, notification) == 0)
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


void handler_sigalrm(int s, siginfo_t *i, void *v) {
    finalize_manager();
    close_program();
    if(unlink(NPSERVER) == -1)
        perror("Erro ao remover FIFO do servidor.");
    printf("\nServidor encerrado\n");
    sleep(1);
    exit(1);
}

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


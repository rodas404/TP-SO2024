#include "feed_comandos.h"
#include "feed.h"

int running = 1;

char npCliente[50];

// --------- Thread para receber mensagens do manager
void *listen_manager(void *arg) {
    int fd_feed = open(npCliente, O_RDWR);
    if (fd_feed == -1) {
        perror("Erro ao abrir FIFO do feed\n");
        exit(1);
    }

    char buffer[MAX_TAM_BUFFER + 1]; //para terminar a msg 
    ssize_t bytes_read;

    while (running) {
        bytes_read = read(fd_feed, buffer, sizeof(buffer));
 
        if (bytes_read == -1) {
            perror("Erro ao ler do FIFO\n");
            break; 
        } else if (bytes_read > 0) {
            if (bytes_read < sizeof(buffer)) {
                buffer[bytes_read] = '\0';
            }
            printf("%s\n", buffer);
        }
    }

    if(close(fd_feed) == -1)
       perror("Erro ao fechar FIFO do feed.\n"); 
    pthread_exit(NULL);
}

void handler_sigalrm(int s, siginfo_t *i, void *v) {
    userRequest request;
    request.type = 4;  //"exit"
    request.pid = getpid();
    strcpy(request.content, "exit");
    int fs = open(NPSERVER, O_WRONLY);
    if (fs != -1) {
        write(fs, &request, sizeof(userRequest));
        if(close(fs) == -1)
            perror("Erro ao fechar FIFO do server.\n");
    } 
    else 
        perror("Erro ao comunicar saída ao servidor.\n");

    handler_sigclose();   
}

void handler_sigclose(){
    running = 0;
    printf("\nAté à proxima!\n");
    if(unlink(npCliente) == -1)
         perror("Erro ao remover FIFO do feed.");
    sleep(1);
    exit(1);
}


int main(int argc, char *argv[]){

    setbuf(stdout, NULL);

    userRequest request;
    loginRequest login; 

    int fs, fc, size, nBytes;
    char comando[MAX_TAM_BUFFER];

    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction act;
    act.sa_sigaction=handler_sigclose;
  	sigaction(SIGUSR1,&act,NULL); 

    /*Função para verficar se o manager está em execução */
	if(access(NPSERVER, F_OK) != 0){
        printf("[ERRO] O manager não se encontra em execução. \n");
        exit(2);
	}

    /* Inicializa o feed (login) */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <username>\n", argv[0]);
        exit(1);
    }
 
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

    /* Envia login para o manager */  
    fs = open(NPSERVER, O_WRONLY);
    if (fs == -1) {
        perror("Erro ao conectar ao manager\n");
        exit(1);
    }

     printf("[INFO] Conectado ao servidor.\n"); 

    request.type = 5; 
    request.pid = login.pid;
    snprintf(request.content, sizeof(request.content), "LOGIN %s %s", login.username, npCliente);

    if (write(fs, &request, sizeof(userRequest)) == -1) {
        perror("Erro ao enviar login para o servidor\n");
        if(close(fs) == -1)
            perror("Erro ao fechar FIFO do servidor.\n");
        if(unlink(npCliente) == -1)
            perror("Erro ao remover FIFO do feed.\n");
        exit(1);
    }

    /*Lê a resposta do manager*/
    fc = open(npCliente, O_RDONLY);
    if (fc == -1) {
        perror("Erro ao abrir FIFO exclusivo do feed.\n");
        exit(1);
    }

    char response[MAX_TAM_BUFFER];
    if(read(fc, response, sizeof(response)) == -1){
        perror("Erro ao ler resposta do servidor.\n");
        exit(1);
    }

    if (strcmp(response, "LOGIN_SUCESSO") != 0) {
        printf("Erro: %s\n", response);
        if(close(fs) == -1) 
            perror("Erro ao fechar o FIFO do servidor\n");
        if (unlink(npCliente) == -1) 
            perror("Erro ao remover o FIFO do feed.\n");
        exit(1);
    }
    printf("%s\n", response); 
    
    if(close(fc) == -1){
        perror("Erro ao fechar FIFO exclusivo do feed.");
        exit(1);
    }

    printf("Seja bem-vindo(a) '%s'. Programa pronto para comandos.\n", login.username);

    // Thread para ler mensagens do manager
    pthread_t listener_thread;
    if(pthread_create(&listener_thread, NULL, listen_manager, NULL) != 0){
        perror("Erro ao criar thread de leitura do manager.\n");
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
        fs = open(NPSERVER, O_WRONLY);
        if(fs == -1){
            printf("Erro ao conectar com o servidor.\n");
            exit(EXIT_FAILURE);
        }

        nBytes = write(fs, &request, sizeof(userRequest));
        if (nBytes == -1) {
            perror("Erro ao escrever no FIFO do servidor\n");
            close(fs);
            exit(EXIT_FAILURE);
        }

        if (close(fs) == -1) {
            perror("Erro ao fechar o FIFO do servidor\n");
            exit(EXIT_FAILURE);
        }
    }
    }while(running);


    if(pthread_join(listener_thread, NULL) != 0) {
        perror("Erro ao juntar a thread de notificações\n");
        exit(EXIT_FAILURE);
    }

    if(close(fc) == -1){
        perror("Erro ao fechar o FIFO do feed.\n");
        exit(EXIT_FAILURE);
    }
  
    if (unlink(npCliente) == -1) {
        perror("Erro ao remover o FIFO do feed.\n");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}
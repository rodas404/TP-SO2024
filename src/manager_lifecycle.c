#include "manager.h"
#include "manager_messages.h"
#include "manager_topics.h"
#include "manager_lifecycle.h"

extern Topic topics[MAX_TOPICS];
extern int topic_count;
extern char msg_file_path[256]; // Caminho para o ficheiro de mensagens
extern pthread_mutex_t topic_mutex;

// Lê as mensagens persistentes do ficheiro 
void load_persistent_messages() {
    FILE *file = fopen(msg_file_path, "r");
    if (!file) {
        if (errno != ENOENT) {
            perror("Erro ao abrir o ficheiro de mensagens");
            exit(EXIT_FAILURE);
        }
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        Mensagem msg;
        char topic_name[TAM_USR_TOP];

       if (sscanf(line, "%s %s %d %[^\n]", topic_name, msg.autor, &msg.tempExp, msg.conteudo) != 4) {
           fprintf(stderr, "Linha mal formatada: %s", line);
           continue;
        }       
        
        // Procura o tópico correspondente
        pthread_mutex_lock(&topic_mutex);
        Topic *topic_ptr = find_topic(topic_name);
        if (topic_ptr == NULL) {
            int topic_index = add_topic(topic_name);
            if (topic_index == 0) {
                fprintf(stderr, "Não foi possível criar o tópico '%s'. Limite de tópicos atingido.\n", topic_name);
                pthread_mutex_unlock(&topic_mutex);
                continue;
            }

            topic_ptr = &topics[topic_count - 1];
        }

         // Armazena a mensagem do ficheiro para o tópico (em memória)
        int result = recupera_mensagem(topic_name, msg.tempExp, msg.conteudo, msg.autor); 
         if (result == 0) {
            fprintf(stderr, "Falha ao armazenar mensagem no tópico '%s' (máximo de mensagens atingido).\n", topic_name);
        } else {
            printf("Mensagem carregada com sucesso no tópico '%s' por '%s'.\n", topic_name, msg.autor);
        }

        pthread_mutex_unlock(&topic_mutex);
    }

    fclose(file);
}


//Guarda as mensagens persistentes no ficheiro
void save_persistent_messages() {
    FILE *file = fopen(msg_file_path, "w");
    if (!file) {
        perror("Erro ao salvar o ficheiro de mensagens\n");
        return;
    }

    pthread_mutex_lock(&topic_mutex);
    for (int i = 0; i < topic_count; i++) {
        Topic *topic = &topics[i];
        for (int j = 0; j < topic->n_msgs; j++) {
            Mensagem *msg = &topic->mensagens[j];
            if (msg->tempExp > 0) {
                fprintf(file, "%s %s %d %s\n", topic->topic_name, msg->autor, msg->tempExp, msg->conteudo);
            }
        }
    }
    pthread_mutex_unlock(&topic_mutex);

    fclose(file);
}

//Ciclo de vida das mensagens 
void *manage_message_lifecycle(void *arg) {
    while (1) {
        sleep(1); //Espera 1 seg para cada verificação
        pthread_mutex_lock(&topic_mutex);
        for (int i = 0; i < topic_count; i++) {
            Topic *topic = &topics[i];
            for (int j = 0; j < topic->n_msgs; j++) {
                Mensagem *msg = &topic->mensagens[j];
                if (msg->tempExp <= 0) {
                    // Remove a mensagem se já estiver expirada
                    remove_message(topic, j);
                    j--; 
                } else {
                    // Decrementa o tempo de vida da mensagem
                    msg->tempExp--;
                }
            }
        }
        pthread_mutex_unlock(&topic_mutex);
    }
    return NULL;
}

//Inicializa o manager 
void initialize_manager() {
    // Se a variável não estiver definida, define 
    if (getenv("MSG_FICH") == NULL) {
        setenv("MSG_FICH", "./mensagens.txt", 1);   
    }

    char *env_path = getenv("MSG_FICH");
    if (!env_path) {
        fprintf(stderr, "Erro: Variável de ambiente MSG_FICH não definida\n");
        exit(EXIT_FAILURE);
    }
    strncpy(msg_file_path, env_path, sizeof(msg_file_path) - 1);
    msg_file_path[sizeof(msg_file_path) - 1] = '\0';

    load_persistent_messages();
}

//Termina o manager guardando as mensagens e terminando o mutex 
void finalize_manager() {
    save_persistent_messages();
    pthread_mutex_destroy(&topic_mutex);
}

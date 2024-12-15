// manager_lifecycle.h

#ifndef MANAGER_LIFECYCLE_H
#define MANAGER_LIFECYCLE_H

#include "manager.h"

// Função que inicializa o ciclo de vida do manager
void initialize_manager();

// Função que termina o ciclo de vida do manager
void finalize_manager();

// Função que gere o ciclo de vida das mensagens
void *manage_message_lifecycle(void *arg);

// Função que lê mensagens persistentes do ficheiro
void load_persistent_messages();

// Função que salva mensagens persistentes no ficheiro
void save_persistent_messages();

#endif // MANAGER_LIFECYCLE_H

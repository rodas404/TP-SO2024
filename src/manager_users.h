#ifndef MANAGER_USERS_H
#define MANAGER_USERS_H

#include "manager.h"


// Função para adicionar um novo utilizador
int add_user(char *username, int pid);

// Função para tratar o login do utilizador
void handle_login(userRequest request);

// Função para tratar a saída do utilizador
void handle_exit(int pid);

// Função para listar utilizadores ativos (comando "users" do Manager)
void list_users();

// Função para remover um utilizador
void remove_user(const char *username);

// Encerra o programa
void close_program(); 

void handler_sigalrm(int s, siginfo_t *i, void *v); 

#endif // MANAGER_USERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#define MAXPATH 128
#define MAXFILE 64

char path[MAXPATH];
//lista dei file
typedef struct s1{
	char file_name[MAXFILE];
	struct stat stat_file;
	char path_name[MAXPATH];
}struttura_file;

typedef struct n1{
	struttura_file info;
	struct n1 *next;
}nodo_lista, *p_lista_generale;

p_lista_generale lista_generale;
//prototipo funzione thread
void* thread_function(void* args);
//dichiaro mutex
pthread_mutex_t mutex;
//prototipi funzioni
int inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path);
void stampa_lista(p_lista_generale lista);
p_lista_generale trova_directory(p_lista_generale lista);
int elimina_directory(p_lista_generale *lista);

p_lista_generale ls_directory(char* path_directory);
int linka_liste(p_lista_generale *lista,p_lista_generale lista_da_likare);

int directory_vuota(p_lista_generale *lista, char* path_name);

int main(){






}


void* thread_function(void* args);

pthread_mutex_t mutex;

int inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path);
void stampa_lista(p_lista_generale lista);
p_lista_generale trova_directory(p_lista_generale lista);
int elimina_directory(p_lista_generale *lista);

p_lista_generale ls_directory(char* path_directory);
int linka_liste(p_lista_generale *lista,p_lista_generale lista_da_likare);

int directory_vuota(p_lista_generale *lista, char* path_name);
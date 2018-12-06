/*In questa versione del programma vengono scartate le cartelle nascoste per evitare l'errore "stack smashing".
  Tuttavia nonostante questo accorgimento tale errore potrebbe verificarsi lo stesso.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define MAXPATH 128
#define MAXFILE 64

char path[MAXPATH];
//lista dei file
typedef struct s1{
	char file_name[MAXFILE];
	char path_name[MAXPATH];
	int dir_flag;
}struttura_file;

typedef struct n1{
	struttura_file info;
	struct n1 *next;
}nodo_lista,*p_lista_generale;

p_lista_generale lista_generale;
//prototipo funzione thread
void* thread_function(void* args);
//dichiaro mutex
pthread_mutex_t mutex_lista;
pthread_mutex_t mutex_uscita;
//prototipi funzioni
void inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path, int flag);
void stampa_lista(p_lista_generale lista);
p_lista_generale trova_directory(p_lista_generale lista);
int elimina_directory(p_lista_generale *lista, char *nome_file);

p_lista_generale ls_directory(char* path_directory);
int linka_liste(p_lista_generale *lista,p_lista_generale lista_da_likare);
int num_core;
/*variabile di controllo per la terminazione dei thread, 1 se il thread ha letto solo file nella directory,
 0 se sono ancora presenti directory*/ 
int controllo[6];
void init_lista(p_lista_generale *lista){
	*lista = NULL;
}
void modifica_path(p_lista_generale *dir);

int main(int narg, char* args[1]){
num_core = get_nprocs();//restituisce il numero di core, presente nella libreria <sys/sysinfo.h>
int is_dir; //flag che indica se il file e' una directory o meno
char path_temporaneo[MAXPATH];
init_lista(&lista_generale);
struct dirent *file;
pthread_t thread[num_core];
DIR *directory;
pthread_mutex_init(&mutex_lista,NULL);
pthread_mutex_init(&mutex_uscita,NULL);
for(int i=0;i<num_core;i++)
	controllo[i] = 0;
pthread_mutex_lock(&mutex_lista);
strcpy(path_temporaneo,args[1]);
if(path_temporaneo[strlen(path_temporaneo)-1] == '/')
	strncpy(path,path_temporaneo,strlen(path_temporaneo)-1);
else 
	strcpy(path,args[1]);
if((directory = opendir(path)) != NULL){
	while((file = readdir(directory)) != NULL){
		if(file->d_type == DT_DIR)
			is_dir=1;
		else
			is_dir=0;
		inserimento_in_lista(&lista_generale,file,path,is_dir);
	}
	closedir(directory);
}else
{
	perror("Path errato o inesistente");
	exit(EXIT_FAILURE);
}
for(int i=0;i<num_core;i++){
	pthread_create(&thread[i],NULL,thread_function,(void*)(intptr_t)i);
}

pthread_mutex_unlock(&mutex_lista);
for(int i=0;i<num_core;i++)
	pthread_join(thread[i],NULL);

	stampa_lista(lista_generale);
	pthread_mutex_destroy(&mutex_lista);
	pthread_mutex_destroy(&mutex_uscita);
	exit(EXIT_SUCCESS);
}

void* thread_function(void* args){
	int time_to_exit = 0;
	int n_thread;
	n_thread = (intptr_t)args;
	int unione_riuscita = 0;
	p_lista_generale dir;
	while(!time_to_exit){
		pthread_mutex_lock(&mutex_lista);
		dir = trova_directory(lista_generale);
	if(dir != NULL){
		elimina_directory(&lista_generale,dir->info.file_name);
		dir->next=NULL;
		pthread_mutex_unlock(&mutex_lista);
		pthread_mutex_lock(&mutex_uscita);
		controllo[n_thread]=0;
		pthread_mutex_unlock(&mutex_uscita);
		p_lista_generale contenuto_dir;
		modifica_path(&dir);
		char  path1[MAXPATH];
		strcpy(path1,dir->info.path_name);
		contenuto_dir = ls_directory(path1);
		free(dir);
		while(!unione_riuscita){
			pthread_mutex_lock(&mutex_lista);
			if(contenuto_dir == NULL){
				printf("directory %s vuota, dunque la scarto\n",path1);
				unione_riuscita = 1;
				pthread_mutex_unlock(&mutex_lista);
			}
			else{
				int successo = 0;
				successo = linka_liste(&lista_generale,contenuto_dir);
				if(successo){
					unione_riuscita = 1;
					pthread_mutex_unlock(&mutex_lista);
				}else{
					perror("Impossibile linkare lista");
					pthread_mutex_unlock(&mutex_lista);
				}
			}
		}
		unione_riuscita=0;
	}
	else{
		pthread_mutex_lock(&mutex_uscita);
		int n=0;
		controllo[n_thread]=1;
		for (int i=0;i<num_core;i++){
			if(controllo[i] == 1)
				n = n+1;
			}
			if(n == num_core){
				time_to_exit = 1;
				pthread_mutex_unlock(&mutex_lista);
				pthread_mutex_unlock(&mutex_uscita);
			}
			else{ 
				time_to_exit = 0;
				pthread_mutex_unlock(&mutex_lista);
				pthread_mutex_unlock(&mutex_uscita);
			}		
		}
	}
	pthread_exit(NULL);
}

void inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path, int flag){
	p_lista_generale aux;
	char stringa[MAXFILE];
	strcpy(stringa,file->d_name);
	aux=(p_lista_generale)malloc(sizeof(nodo_lista));
	if(strcmp(stringa,".") != 0 && strcmp(stringa,"..")!=0 && strncmp(stringa,".",1) !=0){
		strcpy(aux->info.file_name,stringa);
		strcpy(aux->info.path_name,path);
		aux->info.dir_flag = flag;
		aux->next= *lista;
		*lista = aux;
	}
}
void stampa_lista(p_lista_generale lista){
	while (lista != NULL){
		printf("Nome file:%s  |  Path assoluto:%s  \n",lista->info.file_name,lista->info.path_name);
		lista = lista->next;
	}
}
p_lista_generale trova_directory(p_lista_generale lista){
	while (lista != NULL){
		if(lista->info.dir_flag == 1)
			return lista;
		else 
			lista = lista->next;
	}
	return NULL;
}
int elimina_directory(p_lista_generale *lista,char *nome_file){
	if((*lista)->info.dir_flag == 1 && strcmp((*lista)->info.file_name,nome_file) == 0)
		*lista = (*lista)->next;
	else 
		return elimina_directory(&(*lista)->next,nome_file);
}
void modifica_path(p_lista_generale *dir){
	char file_name[MAXFILE];
	strcpy(file_name,(*dir)->info.file_name);
	strcat((*dir)->info.path_name,"/");
	strcat((*dir)->info.path_name, file_name);
	//printf("Ho creato il path %s \n",(*dir)->info.path_name);
}
p_lista_generale ls_directory(char* path_directory){
	DIR *dir;
	int is_dir=0;
	struct dirent *file;
	p_lista_generale cont_dir=NULL;
	if((dir = opendir(path_directory)) != NULL){
		while((file = readdir(dir))!=NULL){
			if(file->d_type == DT_DIR)
				is_dir=1;
			else
				is_dir=0;
		inserimento_in_lista(&cont_dir,file,path_directory,is_dir);
	}
	closedir(dir);
  }	else{
		perror("Path errato o inesistente");
		return NULL;
	}
	if(cont_dir != NULL)
		return cont_dir;
	return NULL;
}

int linka_liste(p_lista_generale *lista,p_lista_generale lista_da_likare){
	if((*lista) == NULL){
		*lista = lista_da_likare;
	}
	else if( (*lista)->next == NULL){
		(*lista)->next = lista_da_likare;
		return 1;
	}
	else
		return linka_liste(&(*lista)->next , lista_da_likare);
}



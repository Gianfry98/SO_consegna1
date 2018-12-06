/* -Versione definitiva del programma . In questa implementazione le stringhe path_name e file_name 
   -vengono allocate dinamicamente per evitare l'errore "stack smashing". Il programma lista anche le directory nascoste. 

   -Impossibile ottenere un lista di tutti i file a partire dalla root. 
 
   -Il path name indicato per ogni file non e' "assoluto" in quando contiene il path fino al nome della directory contenente 
   -non inglobando in esso il nome del file stesso. 
	 */



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

typedef struct n1{
	char* file_name;
	char* path_name;
	int dir_flag;
	struct n1 *next;
}nodo_lista,*p_lista_generale;

p_lista_generale lista_generale;
//prototipo funzione thread
void* thread_function(void* args);
//dichiaro mutex
pthread_mutex_t mutex_lista;
pthread_mutex_t mutex_uscita;
//prototipi funzioni
void inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path_da_inserire, int flag);
void stampa_lista(p_lista_generale lista);
p_lista_generale trova_directory(p_lista_generale lista);
int elimina_directory(p_lista_generale *lista, char *nome_file);

p_lista_generale ls_directory(char* path_directory);
int linka_liste(p_lista_generale *lista,p_lista_generale lista_da_likare);
int num_core;
/*variabile di controllo per la terminazione dei thread, 1 se il thread ha letto solo file nella directory,
 0 se sono ancora presenti directory*/ 
int controllo[20];
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
if(strlen(path_temporaneo)!= 1 && path_temporaneo[strlen(path_temporaneo)-1] == '/')
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
		elimina_directory(&lista_generale,dir->file_name);
		dir->next=NULL;
		pthread_mutex_unlock(&mutex_lista);
		pthread_mutex_lock(&mutex_uscita);
		controllo[n_thread]=0;
		pthread_mutex_unlock(&mutex_uscita);
		p_lista_generale contenuto_dir;
		char* path1;
		path1 = (char*)malloc((strlen(dir->file_name)+strlen(dir->path_name)+2)*sizeof(char));
		strcpy(path1,"");
		strcat(path1,dir->path_name);
		strcat(path1,"/");
		strcat(path1,dir->file_name);
		contenuto_dir = ls_directory(path1);
		free(dir);
		while(!unione_riuscita){
			pthread_mutex_lock(&mutex_lista);
			if(contenuto_dir == NULL){
				printf("directory %s vuota, dunque la scarto\n",path1);
				unione_riuscita = 1;
				path1 = NULL;
				free(path1);
				pthread_mutex_unlock(&mutex_lista);
			}
			else{
				path1 = NULL;
				free(path1);
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

void inserimento_in_lista(p_lista_generale *lista, struct dirent *file, char* path_da_inserire, int flag){
	p_lista_generale aux;
	char* stringa;
	char* stringa2;
	stringa = (char*)malloc((strlen(file->d_name)+1)*sizeof(char));
	stringa2 = (char*)malloc((strlen(path_da_inserire)+2)*sizeof(char));
	strcpy(stringa,file->d_name);
	strcpy(stringa2,path_da_inserire);
	aux = (p_lista_generale)malloc((strlen(stringa)+1)*sizeof(char)+(strlen(path_da_inserire)+2)*sizeof(char)+sizeof(int)+sizeof(nodo_lista));
	if(strcmp(stringa,".") != 0 && strcmp(stringa,"..")!=0 /*&& strncmp(stringa,".",1) !=0*/){
		aux->file_name = stringa;
		aux->path_name = path_da_inserire;
		aux->dir_flag = flag;
		aux->next= *lista;
		*lista = aux;
		stringa = NULL;
		free(stringa);
	}
}
void stampa_lista(p_lista_generale lista){
	while (lista != NULL){
		printf("Nome file:%s  |  Path assoluto:%s  \n",lista->file_name,lista->path_name);
		lista = lista->next;
	}
}
p_lista_generale trova_directory(p_lista_generale lista){
	while (lista != NULL){
		if(lista->dir_flag == 1)
			return lista;
		else 
			lista = lista->next;
	}
	return NULL;
}
int elimina_directory(p_lista_generale *lista,char *nome_file){
	if((*lista)->dir_flag == 1 && strcmp((*lista)->file_name,nome_file) == 0)
		*lista = (*lista)->next;
	else 
		return elimina_directory(&(*lista)->next,nome_file);
}
/*void modifica_path(p_lista_generale *dir){
	char file_name[MAXFILE];
	strcpy(file_name,(*dir)->file_name);
	strcat((*dir)->path_name,"/");
	strcat((*dir)->path_name, file_name);
	//printf("Ho creato il path %s \n",(*dir)->info.path_name);
}*/
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



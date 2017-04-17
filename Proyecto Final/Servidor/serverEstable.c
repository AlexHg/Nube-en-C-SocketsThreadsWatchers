#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/stat.h>

#define MAX_COLA 200
#define PATH_MAX 1024 // Max file path length


#define PORT 8157
#define DIRECTORY_CLOUD "server"
#define TRAMAS 1024*10
#define NUMERO_CLIENTES 3

typedef struct {
    int wd;
    char *full_path;
    int event_id, from_id;
} NOTIFY;

typedef struct {
    int i;
    int entrada, salida;
    NOTIFY elementos[MAX_COLA];
} EVENTO_COLA;

typedef struct {
    int idc, idSocket, idThread;
} DATA_THREAD;


char *notify_events_server[] = {"DESCARGANDO", "ELIMINANDO EN LOCAL", "-","ELIMINANDO EN LOCAL", "DESCARGANDO", ""};
int conections[NUMERO_CLIENTES];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int split (char *str, char c, char ***arr){
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0'){
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
        exit(1);

    p = str;
    while (*p != '\0'){
        if (*p == c){
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
                exit(1);

            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
        exit(1);
    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0'){
        if (*p != c && *p != '\0'){
            *t = *p;
            t++;
        }
        else{
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }
    return count;
}

void *receive_from(void * inx){
    struct sockaddr_in servidor, cliente;
    int idc;
    pthread_t *idh;
    int idSocket;
    struct stat *std;
    int *id_hilo = (int*)inx;
    if((idSocket=socket(AF_INET, SOCK_STREAM,0))<0){
        printf("\nerror en la creacion del socket\n");
        exit(1);
    }

    servidor.sin_family=AF_INET;
    servidor.sin_port=htons(PORT+*id_hilo);
    servidor.sin_addr.s_addr=inet_addr("127.0.0.1");

    if(bind(idSocket,(struct sockaddr *)&servidor, sizeof(struct sockaddr_in) )<0){
        printf("\nerror en el blind\n");
        //exit(1);
    }

    if(listen(idSocket, 4)<0){
        printf("\nError en el listen\n");
        exit(1);
    }


    int tam = sizeof(struct sockaddr_in);
    printf("SOY EL HILO %d\n", *id_hilo);
    printf("Puerto: %d\n", PORT+*id_hilo);
        
    //printf("conectó!");

    if((idc=accept(idSocket, (struct sockaddr *)&cliente, &tam))<0){
        printf("\nerror en el accept\n");
        exit(1);
    }
    printf("IDC: %d\n", idc);
    printf("EL CLIENTE %d SE HA CONECTADO!\n",*id_hilo);
    conections[*id_hilo] = 1;
    //printf("conectó!");

    int numBytes;
    char buffer[TRAMAS];
    char file_info[1024]; //file_name,action_id,size
    char **file_info_arr;
    int file_info_size;
    char file_name[1024];
    int action_id; //0,1,2,3,4,5,6
    double size; //MB
    int fd1; 
    float progress = 0;
    int progress_count = 0;
    int breakit = 0;
    char brk[] = "BREAKED";

    LOOP:
        strcpy(file_info,"");
        //Recibe nombre, tipo y tamaño. si los obtiene sigue con el contenido 
        //pthread_mutex_lock(&mutex);
        while(recv(idc, (void *)file_info, sizeof(file_name), 0) > 0){

            progress_count = 0;
            progress = 0;
            action_id = 0;
            size = 0;

            if(file_info_size = split(file_info, ',', &file_info_arr) < 3){
                strcpy(file_name, DIRECTORY_CLOUD);
                strcat(file_name, file_info_arr[0]);
                action_id = strtoumax(file_info_arr[1], NULL, 10); 
                size = strtof(file_info_arr[2], NULL); 
                //size*= TRAMAS;
                //size+= TRAMAS;
                printf("\naction: %d\n", action_id);
                printf("\nsize: %2.f bytes\n", size);
                printf("error");
                exit(1);
            }


            strcpy(file_name, DIRECTORY_CLOUD);
            strcat(file_name, file_info_arr[0]);
            action_id = strtoumax(file_info_arr[1], NULL, 10); 
            size = strtof(file_info_arr[2], NULL); 
            //size*= TRAMAS;
            //size+= TRAMAS;
            printf("\nFile: %s\n", file_name);
            printf("\naction: %d\n", action_id);
            printf("\nsize: %2.f\n", size);

            if(action_id == 0 || action_id == 4){
                fd1 = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0700);
                //if(size == progress_count*TRAMAS){
                    //write(fd1, NULL, 10);
                    //printf("%d/%2.f\n", progress_count*TRAMAS, size);
                    //printf("[%s %s] \t\t\t%2.f%% \n",notify_events_server[action_id], file_name, progress);
                //}else{
                    
                    while(/*progress_count <= size &&*/(numBytes=recv(idc, buffer, TRAMAS, 0) ) > 0 ){

                            progress = progress_count*TRAMAS;
                            progress /= size;
                            progress *= 100;
                            if(/*(progress_count*TRAMAS) <= size*/ 1){
                                if(progress > 100)
                                    printf("[%s %s] \t\t\t100%% \n",notify_events_server[action_id], file_name);
                                else
                                    printf("[%s %s] \t\t\t%2.f%% \n",notify_events_server[action_id], file_name, progress);
                            }

                            if(write(fd1, &buffer, numBytes)<0){
                                printf("\nerror en el write\n");
                            }
                            progress_count++;
                            //usleep(100000);
                            
                            if(strcmp(buffer, brk) == 0){
                                //printf("SII %s\n", buffer);
                                breakit = 1;
                                break;
                            }
                        
                        
                    }
                    
                //}

                close(fd1);
                printf("[%s %s] \t\t\tCOMPLETADO \n",notify_events_server[action_id], file_name);
                breakit = 0;
                strcpy(file_info,"");
                //goto LOOP;
            }else if((action_id == 1 || action_id == 3) /*&& stat(file_name, std) != -1*/){
                printf("[%s %s] \t\t\t100%%\n", notify_events_server[action_id], file_name);
                remove(file_name);
                strcpy(file_info,"");
                //goto LOOP;
            }
            printf("A LA ESPERA DE QUE CLIENTE %d GENERE NUEVAS ACCIONES!\n",*id_hilo);
        } 
        //pthread_mutex_lock(&mutex);

        //idc = 0;


        printf("EL CLIENTE %d SE DESCONECTÓ!\nBuscando nuevos clientes.....\n",*id_hilo);
        conections[*id_hilo] = 0;
        if((idc=accept(idSocket, (struct sockaddr *)&cliente, &tam))<0){
            printf("\nerror en el accept\n");
            exit(1);
        }
        printf("IDC: %d\n", idc);
        printf("EL CLIENTE %d SE HA CONECTADO!\n",*id_hilo);
        conections[*id_hilo] = 1;
        goto LOOP;
}


int main(int arg0, char *arg1[]){
    
    int * args[NUMERO_CLIENTES];
    pthread_t receive[NUMERO_CLIENTES];
    for (int i = 0; i < NUMERO_CLIENTES; i++){
        args[i] = (int*)malloc(sizeof(int));
        *args[i] = i;
        conections[i] = 0;
        pthread_create(&receive[i], NULL, receive_from, args[i]);
    }
    for (int i = 0; i < NUMERO_CLIENTES; i++){
        pthread_join(receive[i],NULL);
    }
    //close(idSocket);
    return 0;
} 
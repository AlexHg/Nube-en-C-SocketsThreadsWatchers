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


#define PORT 8157
#define DIRECTORY_CLOUD "server"
#define TRAMAS 1024*10
#define MAX_COLA 200
#ifndef PATH_MAX
    #define PATH_MAX 1024 // Max file path length
#endif
#define NUMERO_CLIENTES 3
/* STRUCTS */
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

EVENTO_COLA e_cola[NUMERO_CLIENTES];
pthread_mutex_t mutex[NUMERO_CLIENTES];
int conections[NUMERO_CLIENTES]; 
FILE *fp_submit[NUMERO_CLIENTES];
char basedir[PATH_MAX];
char file_log_path[PATH_MAX];
char file_submit_path[PATH_MAX];
char file_receive_path[PATH_MAX];

char *notify_events_local[] = {"SUBIENDO", "ELIMINANDO EN SERVIDOR", "-","ELIMINANDO EN SERVIDOR", "SUBIENDO", "-"};
char *notify_events_server[] = {"DESCARGANDO", "ELIMINANDO EN LOCAL", "-","ELIMINANDO EN LOCAL", "DESCARGANDO", ""};


void crear_cola(EVENTO_COLA *cola){
    cola->i = cola->salida = cola->entrada = 0;
}

int siguiente(int i){
  return ((i+1) % MAX_COLA);
}

int vacia_cola(EVENTO_COLA *cola){
  return (cola->i == 0);
}

int llena_cola(EVENTO_COLA *cola)
{
  return (cola->i == MAX_COLA);
}

int tamano_cola(EVENTO_COLA *cola){
    return cola->i;
}

void encolar(EVENTO_COLA *cola, NOTIFY evento){
  cola->i++;
  cola->elementos[cola->entrada++] = evento;
  if(cola->entrada == MAX_COLA)
    cola->entrada = 0;
  printf("ENCOLADO!\n");
}

void desencolar(EVENTO_COLA *cola, NOTIFY *evento){
  cola->i--;
  *evento = cola->elementos[cola->salida++];
    if (cola->salida == MAX_COLA)
    cola->salida = 0;
}
void revisar(EVENTO_COLA *cola, NOTIFY *evento){
  *evento = cola->elementos[cola->salida++];
}

void *parallel_syncEvent(void *thread_inf){

    DATA_THREAD *thread_info = (DATA_THREAD *) thread_inf;

    int idSocket = thread_info->idSocket;
    int idThread = thread_info->idThread;
    int idc = thread_info->idc;
    int file_submit_local;
    int save_submit = 1;
    while(conections[idThread]){
        if(tamano_cola(&e_cola[idThread]) > 0){
        //pthread_mutex_lock(&mutex[idThread]);
            NOTIFY sync_event;
            char buffer[TRAMAS];
            int numbytes;
            int numEnviados = 0;

            usleep(1600);
            desencolar(&e_cola[idThread], &sync_event);

            if( (sync_event.event_id == 0 || sync_event.event_id == 4) &&  /* SOLO PARA ENVIAR */ sync_event.event_id != 5 && sync_event.full_path != NULL) {
                char fullName_file[1024];
                strcpy(fullName_file,sync_event.full_path);

                //Obtiene el archivo a pasar
                file_submit_local = open(fullName_file, O_RDONLY);

                usleep(150000);
                printf("\nPasando al cliente %d\n",idThread);
                
                float total_tramas = 0; // numero total de tramas de 1024 a pasar
                float progress;
                char path_name[1024];
                char event_id_char[100];
                char size_char[200];

                //Abre el archivo para saber las tramas que hay
                if(sync_event.event_id == 0 || sync_event.event_id == 4){

                    FILE *fich;
                    fich = fopen(fullName_file, "r");
                    fseek(fich, 0, SEEK_END);
                    float bytes = ftell(fich);
                    //if(bytes == 0) 
                        sleep(2);
                    //printf("\n%f",tramas);
                    total_tramas = (bytes/(TRAMAS));
                    fclose(fich);
                    

                    printf("Paso\n");
                    
                    
                    sprintf(event_id_char, "%d", sync_event.event_id);
                    printf("%s\n", event_id_char);

                    sprintf(size_char, "%f", bytes/*redondea(total_tramas)*/);
                    printf("%f\n", bytes);/*redondea(total_tramas));*/
                }else{

                    float bytes = 0;

                    sprintf(event_id_char, "%d", sync_event.event_id);
                    printf("%s\n", event_id_char);

                    sprintf(size_char, "%f", bytes/*redondea(total_tramas)*/);
                    printf("%f\n", bytes);/*redondea(total_tramas));*/
                }
                strcpy(path_name, sync_event.full_path);
                strcat(path_name, ",");
                strcat(path_name, event_id_char);
                strcat(path_name, ",");
                strcat(path_name, size_char);
                int i = 0;
                printf("Paso2\n");
                int sendid = send(idSocket, (void *)path_name, sizeof(path_name), 0);
                char brk[] = "BREAKED";

                if( sendid > 0 && (sync_event.event_id == 0 || sync_event.event_id == 4)){
                    //printf("%ld\n",sizeof(buffer));

                    while((numbytes = read(file_submit_local, &buffer, sizeof(buffer))) > 0 ) {
                    //for (int i = 0; /*numero de tramas de 1024 pasadas*/ /*i < total_tramas; i++){
                        
                        //usleep(100000);
                        progress = (i/total_tramas)*100.0;

                        //(#1) CODIGO PARA GUARDAR LA DIRECCIÓN DEL ARCHIVO Y LA ACCIÓN A REALIZAR JUNTO CON EL PROGRESO EN EL SERVIDOR (ESTE EMPIEZA EN 0 Y DEBE ESTAR EN LA FORMA 0/8 DONDE 0 SON LAS TRAMAS DE 1024 PASADAS Y 8 EL TOTAL DE TRAMAS EN EL ARCHIVO) POR SI NO HAY CONEXION. 
                        /*fp_submit[idThread] = fopen(file_submit_path, "w+");
                            fprintf(fp_submit[idThread], "Archivo\n%s%s\n",sync_event.full_path,sync_event.file_name);
                            fprintf(fp_submit[idThread], "Acción id\n%d\n",sync_event.event_id);
                            fprintf(fp_submit[idThread], "Progreso\n%d\n", i);
                            fprintf(fp_submit[idThread], "Totales\n%d\n", redondea(total_tramas));
                        fclose(fp_submit[idSocket]);*/

                        //CODIGO PARA SINCRONIZAR CON EL SERVIDOR
                            //SI LA CONEXIÓN FALLA RETORNA EL PROGRAMA
                        //if((numbytes = read(file_submit_local, &buffer, sizeof(buffer))) > 0){
                            if((numEnviados=send(idSocket, (void *)buffer, sizeof(buffer), 0))<0){
                                printf("\nerror al enviar\n");
                                exit(1);
                            }
                        //}

                        // SI TODO SALIÓ BIEN AQUI IRÁ EL CODIGO PARA TERMINAR LA CONEXIÓN Y ELIMINAR DEL REGISTRO (#1) LOS DATOS SOBRE EL EVENTO   

                        i++;
                        printf("[%s %s] \t\t\t%.2f%% \n", notify_events_local[sync_event.event_id],sync_event.full_path, progress);

                    }
                    send(idSocket, brk, sizeof(brk), 0);
                }
                close(file_submit_local);
                //SI TODO SALIÓ BIEN
                /*fp_submit[idThread] = fopen(file_submit_path, "w+");
                    fprintf(fp_submit[idThread], "Archivo\n%s%s\n",sync_event.full_path,sync_event.file_name);
                    fprintf(fp_submit[idThread], "Acción id\n%d\n",sync_event.event_id);
                    fprintf(fp_submit[idThread], "Progreso\n%d\n", i);
                    fprintf(fp_submit[idThread], "Totales\n%d\n", redondea(total_tramas));
                fclose(fp_submit[idThread]);*/
                printf("[%s %s] \t\t\t100.00%% \n\n", notify_events_local[sync_event.event_id],sync_event.full_path);
                

            }

        //pthread_mutex_unlock(&mutex[idThread]);
        }
    }
}

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
    NOTIFY evento;
    DATA_THREAD *thread_info;

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


    //HILO PARA ENVIO DE EVENTOS
    //thread_info->idc = idc;
    //thread_info->idSocket = idSocket;
    //thread_info->idThread = *id_hilo;
    pthread_t hilo_envios;
    //pthread_create(&hilo_envios, NULL, parallel_syncEvent, (void *)&thread_info);
    //pthread_join(hilo_envios,NULL);


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
        while(recv(idc, (void *)file_info, sizeof(file_name), 0) > 0){
        //pthread_mutex_lock(&mutex[*id_hilo]);

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
            strcpy(evento.full_path,file_name);
            evento.from_id = *id_hilo;
            evento.event_id = action_id;
            //ENCOLA PARA ENVIO DE LOS OTROS HILOS (CLIENTES)
            for(int c = 0; c < NUMERO_CLIENTES; c++){
                if(c != *id_hilo){
                    encolar(&e_cola[c], evento);
                }
            }
        //pthread_mutex_lock(&mutex[*id_hilo]);
        } 

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


/*mutex[0] = PTHREAD_MUTEX_INITIALIZER;
mutex[1] = PTHREAD_MUTEX_INITIALIZER;
mutex[2] = PTHREAD_MUTEX_INITIALIZER;*/
int main(int arg0, char *arg1[]){

    int * args[NUMERO_CLIENTES];

    pthread_t receive[NUMERO_CLIENTES];
    for (int i = 0; i < NUMERO_CLIENTES; i++){
        args[i] = (int*)malloc(sizeof(int));
        *args[i] = i;
        conections[i] = 0;
        crear_cola(&e_cola[i]);
        pthread_create(&receive[i], NULL, receive_from, args[i]);
    }
    for (int i = 0; i < NUMERO_CLIENTES; i++){
        pthread_join(receive[i],NULL);
    }
    //close(idSocket);
    return 0;
} 
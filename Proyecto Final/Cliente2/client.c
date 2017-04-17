/* INCLUDES */
#ifndef _SN_FILE_TRACK_H
#define _SN_FILE_TRACK_H
 
    /* INCLUDES */
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/inotify.h>
    #include <dirent.h>
    #include <unistd.h>
    #include <signal.h>
    #include <time.h>
    //Hilos
    #include <pthread.h>
    //sockets
    #include <netinet/in.h>
    #include <sys/sem.h>
    #include <sys/types.h>
    #include <sys/shm.h>
    #include <sys/ipc.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>

    #include <sys/wait.h>
    #include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>

#include <fcntl.h>
#include <netdb.h>
#include <math.h>
     
    /* CONSTANTS */
    #define MAX_COLA 200
     
    #ifndef PATH_MAX
        #define PATH_MAX 1024 // Max file path length
    #endif
    #define MAX_EVENTS 1024 // Max number of events to process at one go
    #define LEN_NAME 16 // Assuming that the length of the filename won't exceed 16 bytes
    #define EVENT_SIZE  (sizeof (struct inotify_event)) // Size of one event
    #define BUF_LEN     (MAX_EVENTS * (EVENT_SIZE + LEN_NAME)) // Buffer to store the data of events
 
    /* STRUCTS */
    typedef struct {
        int wd;
        int hour, min, sec, day, month, year;
        char *full_path, *file_name;
        int event_id, type_id;
    } NOTIFY;

    typedef struct {
      int i;
      int entrada, salida;
      NOTIFY elementos[MAX_COLA];
    } EVENTO_COLA;
     
    /* PROTOTYPES */
    unsigned long addNewWatch(int, char *);
    int searchIDFromWD(int);
    void sigalCallback(int);
    void syncFilter(NOTIFY);
    void syncEvent(NOTIFY);
    void sync_erasePath();
    void sync_eraseFile();
    void sync_sendPath();
    void sync_sendFile();
     
    /* VARS */
    int notify_events[] = {IN_CREATE, IN_DELETE, IN_MODIFY, IN_MOVED_FROM, IN_MOVED_TO, IN_ACCESS/*, IN_OPEN, IN_CLOSE/*, IN_CLOSE_WRITE, IN_CLOSE_NOWRITE, IN_ATTRIB*/};
    char *notify_events_name[] = {"CREÓ", "ELIMINÓ", "MODIFICÓ", "ELIMINÓ", "CREÓ", "ACCEDIÓ"/*, "WRITE", "NO WRITE", "CHANGE ATTRIB"*/};
    char *notify_events_local[] = {"SUBIENDO", "ELIMINANDO EN SERVIDOR", "-","ELIMINANDO EN SERVIDOR", "SUBIENDO", "-"};
    char *notify_events_server[] = {"DESCARGANDO", "ELIMINANDO EN LOCAL", "-","ELIMINANDO EN LOCAL", "DESCARGANDO", "-"};
    FILE *fp_log;
    FILE *fp_submit;
    FILE *fp_receive;
    NOTIFY *paths;

    //char recibido[1024]; //<- El path del archivo que se esta sincronizando desde el servidor
    EVENTO_COLA cola_eventos; //<- Eventos de archivos para sincronizarse en servidor

    //Para servidor
    int idc;
    int puerto, idSocket, numB, numL, numR;
    struct sockaddr_in servidor, cliente;


    unsigned int paths_count = 0;
    char basedir[PATH_MAX];
    char last_name[PATH_MAX];
    char file_log_path[PATH_MAX];
    char file_submit_path[PATH_MAX];
    char file_receive_path[PATH_MAX];
    int save_log = 0;
 
#endif

#define LOG_FILE "log.txt"
#define SUBMIT_FILE "submit.txt"
#define RECEIVE_FILE "receive.txt"
#define DIRECTORY_CLOUD "cloud"
#define TRAMAS 1024*10

#define PORT 8168

int firstTime = 1;
char lastPath[1024];
char lastName[1024];
int lastAction = -1;
int thisno = 0;


//Funciones de la cola de eventos

char *cutString(const char *pcsz_input, const char *pcsz_found){
    char *ret = strdup(pcsz_input);
    char *ptmp;
    int n = strlen(pcsz_found);
    if (!ret)
    return ret;

    while (ptmp = strstr(ret, pcsz_found)) {
        *ptmp = '\0';
        strcat(ptmp, ptmp + n);
    }
    return ret;
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


int redondea(double x){
    if(x - (int)x > 0) return (int)x + 1;
    return (int)x;
}


void crear_cola(EVENTO_COLA *cola)
{
    cola->i = cola->salida = cola->entrada = 0;
}

int siguiente(int i)
{
  return ((i+1) % MAX_COLA);
}

int vacia_cola(EVENTO_COLA *cola)
{
  return (cola->i == 0);
}

int llena_cola(EVENTO_COLA *cola)
{
  return (cola->i == MAX_COLA);
}

int tamano_cola(EVENTO_COLA *cola){
    return cola->i;
}

void encolar(EVENTO_COLA *cola, NOTIFY evento)
{
  cola->i++;
  cola->elementos[cola->entrada++] = evento;
  if(cola->entrada == MAX_COLA)
    cola->entrada = 0;
  //printf("ENCOLADO!\n");
}

void desencolar(EVENTO_COLA *cola, NOTIFY *evento)
{
  cola->i--;
  *evento = cola->elementos[cola->salida++];
    if (cola->salida == MAX_COLA)
    cola->salida = 0;
}

void sendDist(char *file_name, int part){
    char buffer[TRAMAS];
    int total_tramas;
    int i = 0;
    int numbytes;
    char brk[] = "BREAKED";

    FILE *fich;
    fich = fopen(file_name, "r");
    fseek(fich, 0, SEEK_END);
    float bytes = ftell(fich);
    //if(bytes == 0) 
    sleep(2);
    //printf("\n%f",tramas);
    total_tramas = redondea( bytes/(TRAMAS*3) ); //entre el numero de clientes
    fclose(fich);


    int in_submit_local = open(file_name, O_RDONLY);

    while((numbytes = read(in_submit_local, &buffer, sizeof(buffer))) > 0 ) {
    //for (int i = 0; /*numero de tramas de 1024 pasadas*/ /*i < total_tramas; i++){
        if( i >= total_tramas*(part) && i < total_tramas*(part+1) ) {   
            //CODIGO PARA SINCRONIZAR CON EL SERVIDOR
            if((send(idSocket, (void *)buffer, sizeof(buffer), 0))<0){
                printf("\nerror al enviar\n");
                exit(1);
            }
            // SI TODO SALIÓ BIEN AQUI IRÁ EL CODIGO PARA TERMINAR LA CONEXIÓN Y ELIMINAR DEL REGISTRO (#1) LOS DATOS SOBRE EL EVENTO   
            //if(i >= in_progreso) { 
            printf("[SUBIENDO (%d/3) %s ] \t\t(%d/%d) \n", part, file_name, i, total_tramas*(part+1));
        }
        i++;
    }
    send(idSocket, brk, sizeof(brk), 0);
    printf("[SUBIENDO (%d/3) %s ] \t\t(%d/%d) \n", part, file_name, total_tramas*(part+1), total_tramas*(part+1));

}

void *parallel_syncByServer(){
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
        //strcpy(file_info,"");
        //Recibe nombre, tipo y tamaño. si los obtiene sigue con el contenido
        thisno = 0; 
        //
        //printf("PROCESANDO");
        while(recv(/*idc*/idSocket, (void *)file_info, sizeof(file_name), 0) > 0){
            thisno = 1;
            //pthread_mutex_lock(&mutex[*id_hilo]);

            progress_count = 0;
            progress = 0;
            action_id = 0;
            size = 0;

            printf("%s\n",file_info);

            if((file_info_size = split(file_info, ',', &file_info_arr)) < 3){
                //printf("error");
                goto LOOP;
            }

            printf("%d\n", file_info_size);

            if(file_info_size == 4 /*&& strcmp(file_info_arr[2], "distribute")*/){
                int part = strtoumax(file_info_arr[0], NULL, 10);
                strcpy(file_name, file_info_arr[1]);
                printf("\nINICIANDO MODO DISTRIBUIDO\n");
                sendDist(file_name, part);
                goto LOOP;
            }


            char *serc = "servercloud";
            strcpy(file_name, DIRECTORY_CLOUD);
            strcat(file_name, cutString(file_info_arr[0], serc));
            action_id = strtoumax(file_info_arr[1], NULL, 10); 
            size = strtof(file_info_arr[2], NULL); 
            //size*= TRAMAS;
            //size+= TRAMAS;
            //printf("\nFile: %s\n", file_name);
            //printf("\naction: %d\n", action_id);
            //printf("\nsize: %2.f\n", size);

            if(action_id == 0 || action_id == 4){
                fd1 = open(file_name, O_WRONLY|O_CREAT|O_TRUNC, 0700);
                //if(size == progress_count*TRAMAS){
                    //write(fd1, NULL, 10);
                    //printf("%d/%2.f\n", progress_count*TRAMAS, size);
                    //printf("[%s %s] \t\t\t%2.f%% \n",notify_events_server[action_id], file_name, progress);
                //}else{
                    
                    while(/*progress_count <= size &&*/(numBytes=recv(/*idc*/idSocket, buffer, TRAMAS, 0) ) > 0 ){

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
                sleep(1);
                breakit = 0;
                //strcpy(file_info,"");
                goto LOOP;
            }else if((action_id == 1 || action_id == 3) /*&& stat(file_name, std) != -1*/){
                printf("[%s %s] \t\t\t100%%\n", notify_events_server[action_id], file_name);
                remove(file_name);
                //strcpy(file_info,"");
                goto LOOP;
            }
            //printf("A LA ESPERA DE QUE CLIENTE %d GENERE NUEVAS ACCIONES!\n",*id_hilo);
            //strcpy(evento.full_path,file_name);
            //evento.from_id = *id_hilo;
            //evento.event_id = action_id;
            //printf("HOLA");
            //ENCOLA PARA ENVIO DE LOS OTROS HILOS (CLIENTES)
            /*for(int c = 0; c < NUMERO_CLIENTES; c++){
                if(c != *id_hilo){
                    encolar(&e_cola[c], evento);
                }
            }*/
            //pthread_mutex_lock(&mutex[*id_hilo]);
            thisno = 0;
        } 

        //idc = 0;


        //printf("EL CLIENTE %d SE DESCONECTÓ!\nBuscando nuevos clientes.....\n",*id_hilo);
        //conections[*id_hilo] = 0;
        /*if((idc=accept(idSocket, (struct sockaddr *)&cliente, &tam))<0){
            printf("\nerror en el accept\n");
            exit(1);
        }*/
        //printf("IDC: %d\n", idc);
        //printf("EL CLIENTE %d SE HA CONECTADO!\n",*id_hilo);
        //conections[*id_hilo] = 1;
        goto LOOP;

}

void *parallel_syncEvent(){
    strcpy(file_submit_path, SUBMIT_FILE);
    int file_submit_local;
    int save_submit = 1;

    while(1){
        if(tamano_cola(&cola_eventos) > 0){
            NOTIFY sync_event;
            fflush(stdout);
            char buffer[TRAMAS];
            int numbytes;
            int numEnviados = 0;


            //Codigo para sacar la primer estructura (NOTIFY) de la cola y guardarla en la variable sync_event
            usleep(1600);
            desencolar(&cola_eventos, &sync_event);

            //printf("DESENCOLADO!\n");
            //printf("Elementos en cola %d\n", tamano_cola(&cola_eventos));
            if( thisno==0 &&/*(/*sync_event.event_id == 0 ||*//* sync_event.event_id == 4) && */ /* SOLO PARA ENVIAR */ sync_event.event_id != 5 && sync_event.file_name != NULL) {

                char fullName_file[1024];
                strcpy(fullName_file,sync_event.full_path);
                strcat(fullName_file,sync_event.file_name);

                //Obtiene el archivo a pasar
                file_submit_local = open(fullName_file, O_RDONLY);


                //COMENZANDO LA OPERACION
                usleep(150000);
                printf("\nSincronizando con la nube\n");
                
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
                    

                    //printf("Paso\n");
                    // Archivo submit progress
                    if (save_submit) {
                        fp_submit = fopen(file_submit_path, "w+");
                        if (fp_submit == NULL) {
                            fp_submit = stdout;
                        }
                    } else
                        fp_submit = stdout;
                    
                    sprintf(event_id_char, "%d", sync_event.event_id);
                    //printf("%s\n", event_id_char);

                    sprintf(size_char, "%f", bytes/*redondea(total_tramas)*/);
                    //printf("%f\n", bytes);/*redondea(total_tramas));*/
                }else{

                    float bytes = 0;

                    sprintf(event_id_char, "%d", sync_event.event_id);
                    //printf("%s\n", event_id_char);

                    sprintf(size_char, "%f", bytes/*redondea(total_tramas)*/);
                    //printf("%f\n", bytes);/*redondea(total_tramas));*/
                }

                


                strcpy(path_name, sync_event.full_path);
                strcat(path_name, sync_event.file_name);
                strcat(path_name, ",");
                strcat(path_name, event_id_char);
                strcat(path_name, ",");
                strcat(path_name, size_char);
                int i = 0;
                //printf("Paso2\n");
                int sendid = send(idSocket, (void *)path_name, sizeof(path_name), 0);
                char brk[] = "BREAKED";
                if( sendid > 0 && (sync_event.event_id == 0 || sync_event.event_id == 4)){
                    //printf("%ld\n",sizeof(buffer));

                    while((numbytes = read(file_submit_local, &buffer, sizeof(buffer))) > 0 ) {
                    //for (int i = 0; /*numero de tramas de 1024 pasadas*/ /*i < total_tramas; i++){
                        
                        //usleep(100000);
                        progress = (i/total_tramas)*100.0;

                        //(#1) CODIGO PARA GUARDAR LA DIRECCIÓN DEL ARCHIVO Y LA ACCIÓN A REALIZAR JUNTO CON EL PROGRESO EN EL SERVIDOR (ESTE EMPIEZA EN 0 Y DEBE ESTAR EN LA FORMA 0/8 DONDE 0 SON LAS TRAMAS DE 1024 PASADAS Y 8 EL TOTAL DE TRAMAS EN EL ARCHIVO) POR SI NO HAY CONEXION. 
                        fp_submit = fopen(file_submit_path, "w+");
                            fprintf(fp_submit, "Archivo\n%s%s\n",sync_event.full_path,sync_event.file_name);
                            fprintf(fp_submit, "Acción id\n%d\n",sync_event.event_id);
                            fprintf(fp_submit, "Progreso\n%d\n", i);
                            fprintf(fp_submit, "Totales\n%d\n", redondea(total_tramas));
                        fclose(fp_submit);

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
                        printf("[%s %s%s] \t\t\t%.2f%% \n", notify_events_local[sync_event.event_id],sync_event.full_path,sync_event.file_name, progress);

                    }
                    send(idSocket, brk, sizeof(brk), 0);
                }
                close(file_submit_local);
                //SI TODO SALIÓ BIEN
                fp_submit = fopen(file_submit_path, "w+");
                    fprintf(fp_submit, "Archivo\n%s%s\n",sync_event.full_path,sync_event.file_name);
                    fprintf(fp_submit, "Acción id\n%d\n",sync_event.event_id);
                    fprintf(fp_submit, "Progreso\n%d\n", i);
                    fprintf(fp_submit, "Totales\n%d\n", redondea(total_tramas));
                fclose(fp_submit);
                printf("[%s %s%s] \t\t\t100.00%% \n\n", notify_events_local[sync_event.event_id],sync_event.full_path,sync_event.file_name);
                
            }
            
        }
    }
}


void syncEvent(NOTIFY sync_event){

    //Codigo para meter el evento a la cola. quitar el siguiente codigo
    //if(sync_event.event_id != 5){
    if(thisno == 0 && (sync_event.event_id == 4 || sync_event.event_id == 0) ){
         //printf("Encolando\n");
         encolar(&cola_eventos, sync_event); 
         //printf("Tamaño de la cola: %d\n",tamano_cola(&cola_eventos));
         //printf("\nmetiendo evento a la cola --> %s%s\n\n", sync_event.full_path,sync_event.file_name);
    }
    
}

void syncFilter(NOTIFY sync_event){
    //SEGUNDO NIVEL DE REVISIÓN DE EVENTOS
    if(sync_event.event_id == 5 && sync_event.type_id == 1 && lastAction == 3){
        //Revisa si se movió de lugar un archivo o carpeta
        sync_event.event_id = 0;
        strcpy(sync_event.file_name,lastName);
        //IMPRIME EN EL ARCHIVO LOG.TXT
        fprintf(fp_log,"\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], sync_event.full_path, sync_event.file_name);
        //IMPRIME EN CONSOLA
        printf("\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], sync_event.full_path, sync_event.file_name);
        //INTENTA SINCRONIZAR EL ARCHIVO
        //CREA UN HILO PARA SINCRONIZAR SIN PARAR DE CHECAR LOS EVENTOS
        syncEvent(sync_event);

    }else if(sync_event.event_id == 4 && lastAction == 0){
        //Revisa si se renombró un archivo o carpeta
        sync_event.event_id = 3;
        strcpy(sync_event.file_name,lastName);
        if(sync_event.type_id == 2){
            fprintf(fp_log,"\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], lastPath, lastName);
            printf("\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], lastPath, lastName );
        }else if(sync_event.type_id == 1){
            fprintf(fp_log,"\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s CARPETA %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], lastPath, lastName);
            printf("\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s CARPETA %s%s \n", sync_event.hour, sync_event.min, sync_event.sec, sync_event.day, sync_event.month, sync_event.year, sync_event.event_id, notify_events_name[sync_event.event_id], lastPath, lastName);
        }
        syncEvent(sync_event);
    }else if(sync_event.event_id != 5 && sync_event.type_id != 1){
        //TODO FUE CORRECTAMENTE EN EL PRIMER FILTRO
        //printf("\nSincronizando con la nube --> %s\n\n", sync_event.file_name);
        //syncEvent(sync_event);
        //printf("\nSincronizando con la nube \n\n"/*--> %s%s\n\n", sync_event.full_path,sync_event.file_name*/);
    }

    return;
}
 
int searchIDFromWD(int wd) {
    //obtiene el id del evento
    if (wd) {
        int i;
        for (i = 0; i < paths_count; i++)
            if (paths[i].wd == wd) 
                return i;
    }
     
    return -1;
}
  
void sigalCallback(int sigal) {
    // LIBERA Y LIMPIA EL LOG.TXT
    if (save_log) {
        fflush(fp_log);
        fclose(fp_log);
    }
     
    // LIVERA MEMORIA
    free(paths);
 
    exit(0);
}
 


void *fileTrack(){
    char current_path[PATH_MAX];
     
    // Check APP params
    // Monitorize custom path
    strcpy(basedir, DIRECTORY_CLOUD);
         
    if (basedir[0] == '.') {
        // Use current path
        if (getcwd(current_path, sizeof(current_path)) != NULL)
            strcpy(basedir, current_path);
        else
            perror("getcwd() error");
    }
         
    if (basedir[strlen(basedir) - 1] != '/')
        strcat(basedir, "/");

    // Save log into file
    strcpy(file_log_path, LOG_FILE);
    save_log = 1;
             
    printf("Guardando registros en '%s'.\n", file_log_path);
 
    // Inicializa el array
    paths = (NOTIFY *) malloc((paths_count + 1) * sizeof(NOTIFY));
    
    // Capture signal
    signal(SIGINT, sigalCallback);
  
    // Archivo log
    if (save_log) {
        fp_log = fopen(file_log_path, "a");
        if (fp_log == NULL) {
            fprintf(fp_log, "Error abriendo el log. '.\n");
            fp_log = stdout;
        }
    } else
        fp_log = stdout;
     
    // Inicia INOTIFY
    int fd_notify;
    #ifdef IN_NONBLOCK
        fd_notify = inotify_init1(IN_NONBLOCK);
    #else
        fd_notify = inotify_init();
    #endif
 
    if (fd_notify < 0) {
        if (save_log)
            fprintf(fp_log, "ERROR: No se puede inicializar iNotify.\n");
        else
            perror("ERROR: No se puede inicializar iNotify.");
             
        exit(0);
    }
 
    // Agrega el watch a la carpeta principal
    unsigned long events_count = addNewWatch(fd_notify, basedir);
    //printf("Total Events: %lu\n\n", events_count);
    
    int length, i;
    char buffer[BUF_LEN];

    while (1) {
        i = 0;
        length = read(fd_notify, buffer, BUF_LEN);  
 
        //if (length < 0)
            //perror("ERROR: read().");
 
        //LEER EVENTOS
        while (i < length) {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            if (event->len) {                
                time_t t = time(NULL);
                struct tm tm = *localtime(&t);
                char fullpath[PATH_MAX];
 
                int id = searchIDFromWD(event->wd);
                if (id >= 0)
                    strcpy(fullpath, paths[id].full_path);
                else
                    fullpath[0] = '\0';
 
                int e;
                for (e = 0; e < sizeof(notify_events) / sizeof(notify_events[0]); e++) {
                    if (event->mask & notify_events[e] ) {
                        if(strstr(event->name, ".goutputstream") == NULL && strstr(event->name, ".fr") == NULL  /*&& e != 5*/){

                            if (strcmp(last_name, event->name) != 0) {
                                /*
                                    EVENTOS
                                        0 -> Crear
                                        1 -> Eliminar
                                        2 -> Modificar
                                        3 -> Eliminar (Mover de) 
                                        4 -> Crear (Mover a)
                                        5 -> Acceder
                                    
                                    CONSIDERACIONES
                                       Los siguientes archivos no se tomarán en cuenta:
                                        .goutputstream  -> Documento de memoria temporal al editar un archivo
                                        .fr             -> Documento de memoria temporal al comprimir un archivo

                                        Solo necesitamos conocer información respecto al EVENTO 5 pues algunas veces falla el reconocimiento del numero 4
                                */
                                
                                paths[id].event_id = e;
                                strcpy(paths[id].file_name, event->name);
                                //printf("\n---->%s\n",event->name );
                                paths[id].hour = tm.tm_hour;
                                paths[id].min = tm.tm_min;
                                paths[id].sec = tm.tm_sec;
                                paths[id].day = tm.tm_mday;
                                paths[id].month = tm.tm_mon+1;
                                paths[id].year = tm.tm_year + 1900;

                                //printf("\nevento: %d\n",e);

                                if (event->mask & IN_ISDIR){
                                    paths[id].type_id = 1;

                                    if(e != 5){
                                        fprintf(fp_log,"\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s CARPETA %s%s \n", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900, paths[id].event_id, notify_events_name[e], paths[id].full_path, event->name);
                                        printf("\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s CARPETA %s%s \n", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900, paths[id].event_id, notify_events_name[e], paths[id].full_path, event->name);
                                    
                                    }
                                }
                                else{
                                    paths[id].type_id = 2;
                                    fprintf(fp_log,"\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900, paths[id].event_id, notify_events_name[e], paths[id].full_path, event->name);    
                                    printf("\n[%02d:%02d:%02d %02d/%d/%d ID: %d] %s ARCHIVO %s%s \n", tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900, paths[id].event_id, notify_events_name[e], paths[id].full_path, event->name);    
                                }

                                syncEvent(paths[id]);

                                syncFilter(paths[id]);

                                //Conocer el nombre del archivo anterior
                                strcpy(lastPath,paths[id].full_path);
                                strcpy(lastName,paths[id].file_name);
                                lastAction = paths[id].event_id;
                            }
                            strcpy(last_name, event->name);
                        }
                    }
                }
 
                if (save_log)
                    fflush(fp_log);
            }
            i += EVENT_SIZE + event->len;
        }
    }
 
    close(fd_notify);
 
    return 0;
}
 
unsigned long addNewWatch(int fd, char *path) {
    int wd;
    unsigned long events_count = 0;
    struct dirent *entry;
    DIR *dp;
 
    // agrega un hash al nombre del directorio
    if (path[strlen(path) - 1] != '/')
        strcat(path, "/");
 
    dp = opendir(path);
    if (dp == NULL) {
        perror("ERROR: Abriendo la carpeta.");
        exit(0);
    }
 
    // Agrega un watch
    wd = inotify_add_watch(fd, path, IN_CREATE | IN_DELETE | IN_MODIFY | IN_ACCESS | IN_MOVE | IN_ATTRIB | IN_OPEN); 
    if (wd == -1)
        fprintf(fp_log, "ERROR: No se puede observar %s\n", path);
    
    else {

        paths = (NOTIFY *) realloc(paths, (paths_count + 1) * sizeof(NOTIFY));

        paths[paths_count].wd = wd;
        paths[paths_count].full_path = (char *) malloc((strlen(path) + 1) * sizeof(char));
        paths[paths_count].file_name = (char *) malloc((strlen(path) + 1) * sizeof(char));
        paths[paths_count].type_id = 0; //0 nada 1 archivo 2 carpeta
        paths[paths_count].event_id = 0; 
        paths[paths_count].hour = 0;
        paths[paths_count].min = 0;
        paths[paths_count].sec = 0;
        paths[paths_count].day = 0;
        paths[paths_count].month = 0;
        paths[paths_count].year = 0;

        strcpy(paths[paths_count].full_path, path); 
        paths_count++;
        events_count++;
    }
 
    while ((entry = readdir(dp))) {
        char new_dir[1024];
        // Si es una carpeta agrega un watch recursivamente dentro de él
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, (char *) "..") != 0 && strcmp(entry->d_name, ".") != 0) {
                strcpy((char *) new_dir, path);
                //strcat((char *) new_dir, (char *) "/");
                strcat((char *) new_dir, entry->d_name);
                events_count += addNewWatch(fd, (char *) new_dir);
            }
        }
    }

    closedir(dp);
    return events_count;
}

/* ENTRY POINT */
int main(int argc, char **argv) {
    //Crea cola de eventos
    crear_cola(&cola_eventos);

    //DEFINE EL SERVIDOR
    //Paso 1: Creacion del socket
    if((idSocket=socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error en socket.\n");
        exit(1);    
    }

    //Paso 2: Llenado de estructura
    servidor.sin_family = AF_INET;
    servidor.sin_port = htons(PORT);
    servidor.sin_addr.s_addr = inet_addr("127.0.0.1");

    if((bind(idSocket,(struct sockaddr *)&servidor, sizeof(struct sockaddr_in)))<0){
        //printf("\nerror en el bind\n");
        //exit(1);
    }
    //printf("1\n");
    if((connect(idSocket,(struct sockaddr *)&servidor, sizeof(struct sockaddr_in)))<0){
        printf("\nerror en el connect\n");
        exit(1);
    }
    printf("CONEXIÓN ESTABLECIDA\n");    
    printf("BUSCANDO ARCHIVOS INTERRUMPIDOS\n");
    //Codigo para verificar si no hay transferencias pendientes
    char linea[9][100];
    int ln = 0;
    FILE *submit = fopen(SUBMIT_FILE, "r");
    FILE *fp_submit;
    char in_path_name[1024];

    while (feof(submit) == 0)
    {
        fgets(linea[ln],100,submit);
        fflush(stdin);
        //printf("%s",linea[ln]);
        for(int i = 0; i < 100; i++){
            if(linea[ln][i] == '\n')
                linea[ln][i] = '\0';
        }
        ln++;
    }
    int in_progreso, in_tamano, in_evento;
    if( (in_progreso = strtoumax(linea[5], NULL, 10)) < (in_tamano = strtoumax(linea[7], NULL, 10)) ){
        in_evento = strtoumax(linea[3], NULL, 10);
        printf("SE HALLÓ UN ARCHIVO INTERRUMPIDO\n");
        //Codigo para partir de donde se interrumpio el archivo
        printf("\nArchivo: %s \nAcción: %d \nProgreso: %d/%d\n\n",linea[1], in_evento, in_progreso, in_tamano);
        strcpy(in_path_name, linea[1]);
        strcat(in_path_name, ",");
        strcat(in_path_name, linea[3]);
        strcat(in_path_name, ",");
        strcat(in_path_name, linea[7]);
        printf("%s\n",in_path_name);
        
        int sendid = send(idSocket, (void *)in_path_name, sizeof(in_path_name), 0);
        char brk[] = "BREAKED";
        int i = 0;
        int numbytes, numEnviados;
        char buffer[TRAMAS];
        float progress = 0;
        int in_submit_local;
        if( sendid > 0 && (in_evento == 0 || in_evento == 4)){
            //printf("%ld\n",sizeof(buffer));
            in_submit_local = open(linea[1], O_RDONLY);

            while((numbytes = read(in_submit_local, &buffer, sizeof(buffer))) > 0 ) {
            //for (int i = 0; /*numero de tramas de 1024 pasadas*/ /*i < total_tramas; i++){
                //if(i >= in_progreso) {   
                    //usleep(100000);
                    progress = (i/in_tamano)*100.0;

                    //(#1) CODIGO PARA GUARDAR LA DIRECCIÓN DEL ARCHIVO Y LA ACCIÓN A REALIZAR JUNTO CON EL PROGRESO EN EL SERVIDOR (ESTE EMPIEZA EN 0 Y DEBE ESTAR EN LA FORMA 0/8 DONDE 0 SON LAS TRAMAS DE 1024 PASADAS Y 8 EL TOTAL DE TRAMAS EN EL ARCHIVO) POR SI NO HAY CONEXION. 
                    fp_submit = fopen(SUBMIT_FILE, "w+");
                        fprintf(fp_submit, "Archivo\n%s\n",linea[1]);
                        fprintf(fp_submit, "Acción id\n%d\n",in_evento);
                        fprintf(fp_submit, "Progreso\n%d\n", i);
                        fprintf(fp_submit, "Totales\n%d\n", in_tamano);

                    //CODIGO PARA SINCRONIZAR CON EL SERVIDOR
                        //SI LA CONEXIÓN FALLA RETORNA EL PROGRAMA
                      //if((numbytes = read(file_submit_local, &buffer, sizeof(buffer))) > 0){
                    if((numEnviados=send(idSocket, (void *)buffer, sizeof(buffer), 0))<0){
                        printf("\nerror al enviar\n");
                            exit(1);
                        }
                    //}

                            // SI TODO SALIÓ BIEN AQUI IRÁ EL CODIGO PARA TERMINAR LA CONEXIÓN Y ELIMINAR DEL REGISTRO (#1) LOS DATOS SOBRE EL EVENTO   
                    fclose(fp_submit);
                if(i >= in_progreso) { 
                    printf("[%s %s ] \t\t(%d/%d) \n", notify_events_local[in_evento],linea[1],i,in_tamano);
                }
                else{

                    printf("Ya se pasó %d/%d\n",i,in_tamano);
                }
                i++;
            }
            send(idSocket, brk, sizeof(brk), 0);
        }
        close(in_submit_local);
        //SI TODO SALIÓ BIEN
        fp_submit = fopen(SUBMIT_FILE, "w+");
            fprintf(fp_submit, "Archivo\n%s\n",linea[1]);
            fprintf(fp_submit, "Acción id\n%d\n",in_evento);
            fprintf(fp_submit, "Progreso\n%d\n", i);
            fprintf(fp_submit, "Totales\n%d\n", in_tamano);
        fclose(fp_submit);
        printf("[%s %s] \t\t\t%.2f%% \n", notify_events_local[in_evento],linea[1], progress);
    }else printf("NO SE HALLARON ARCHIVOS EN COLA\n");

    pthread_t file_track;
    pthread_t watch_events;
    pthread_t watch_server;

    //Escucha a la carpeta Cloud/ en busca de cambios (Eventos), luego los incerta en cola
    pthread_create(&file_track, NULL, fileTrack ,NULL);
    //Escucha a la cola de eventos en busca de uno nuevo. Si lo encuentra lo sincroniza con el servidor
    pthread_create(&watch_events, NULL, parallel_syncEvent, NULL);
    //Escucha al servidor en busca de cambios en su carpeta principal
    pthread_create(&watch_server, NULL, parallel_syncByServer, NULL);
       
    pthread_join(file_track,NULL);
    pthread_join(watch_events,NULL);
    pthread_join(watch_server,NULL);
}

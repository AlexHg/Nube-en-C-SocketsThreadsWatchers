# Nube-en-C-SocketsThreadsWatchers

## Compilar:

gcc -pthread server.c -o serv
gcc -pthread client.c -o cli

## Notas:

Nube de N clientes, definidos en el codigo 3 pero expandibles.
En el proyecto deben ir carpetas especiales donde se guarda el contenido de los clientes y el servidor (cloud/ y servercloud/ respectivamente)
Las peticiones funcionan a partir de watchers por lo que los usuarios no tienen que escribir nada en terminal. Los watchers identifican diferentes eventos a sincronizar: Agregar, Eliminar, Renombrar. 

- Los datos van al servidor y luego este se encarga de reenviarselo a los demás clientes conectados.
- Es a prueba de fallos, si un archivo se está enviando y se desconecta el cliente, cuando vuelve a conectarse reanuda la transferencia (gracias a un registro interno llamado submit.txt que guarda el estado del ultimo archivo subido)
- Se usan hilos por cada cliente por lo que pueden realizar acciones los 3 al mismo tiempo 
- Se usa una cola de acciones, por lo que, aunque se hagan varias acciones en un mismo servidor, esperará a terminar la actual para seguir con las demás, esto tambien es gracias a semaforos
- Usa archivos de registro para ver la actividad del usuario
- Aunque un cliente se conecte posteriormente a una acción, este tambien será sincronizado.


## Estructura de ficheros y carpetas

Cliente1

| cloud/ <- Watcher dirigido a esta carpeta, ahi se mueven los archivos a sincronizar. Agregar los archivos despues de ejecutar

| client.c (Puerto por defecto 8167)

| log.txt (vacio)

| submit.txt (vacio)

Cliente2

| cloud/ <- Watcher dirigido a esta carpeta, ahi se mueven los archivos a sincronizar. Agregar los archivos despues de ejecutar

| client.c (Cambiar puerto a 8168)

| log.txt (vacio)

| submit.txt (vacio)

Cliente3

| cloud/ <- Watcher dirigido a esta carpeta, ahi se mueven los archivos a sincronizar. Agregar los archivos despues de ejecutar

| client.c (Cambiar puerto a 8169)

| log.txt (vacio)

| submit.txt (vacio)

Servidor

| servercloud/ (vacia) <- Aqui se sincronizan todos los archivos para luego reenviarlos a los demás clientes

| server.c


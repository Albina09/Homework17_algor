#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>

struct sockaddr_in serv;
char buff[256];

void errorExit(char err[]);

void errorExit(char err[]){
    perror(err);
    exit(EXIT_FAILURE);
}
void *se(void *arg){
    int *fd =(int*)arg;
    char buff[256];
    
    while(1){
        fgets(buff, sizeof(buff), stdin);

        if(strcmp(buff, "exit\n") == 0){

            if(send(*fd, buff, strlen(buff), 0) == -1) 
                errorExit("send");
                
            pthread_exit(0);

        }else{
            if(send(*fd, buff, strlen(buff), 0) == -1) 
                errorExit("send");
                
        }
    }

}

void *receive(void *arg){
    int *fd = (int*)arg;
    

    while(1){
        /*Принимает сообщение от сервера*/
        if (recv(*fd, buff, sizeof(buff), 0) == -1) 
            errorExit("recv");
            
        /*Выводит на экран*/
        printf("%s\n",buff);
    }
    
}

int main(){
    pthread_t thread[2];
    socklen_t len;
    int fd;

    /*Создаю сокет*/
    fd = socket(AF_INET,SOCK_STREAM,0);
    if(fd == -1)
        errorExit("soket");
        
    /*Инициализирую структуру*/
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9023);
    serv.sin_addr.s_addr = htons(INADDR_ANY);
    
    /*Соединяюсь с сервером*/
    len = sizeof(serv);
    if (connect(fd, (struct sockaddr *)&serv, len) == -1)
        errorExit("accept");
        
    if(pthread_create(&thread[1], NULL, se, (void*)&fd) == -1)
        errorExit("accept");
        
    if(pthread_create(&thread[0], NULL, receive, (void*)&fd) == -1)
        errorExit("pthread_create");
    
    pthread_join(thread[1], NULL);
    pthread_cancel(thread[0]);
    close(fd);
    exit(EXIT_SUCCESS);

}
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>

struct users{
    int new_fd;
    struct sockaddr_in client;
};
void errorExit(char err[]);

void errorExit(char err[]){
    perror(err);
    exit(EXIT_FAILURE);
}

void *server(void *arg){
    struct users user_lock = *(struct users*)arg;
    char buff[256];

    while(1){
        char msg[256] = "Сервер получил: ";

        if ((recv(user_lock.new_fd, buff, sizeof(buff), 0)) == -1)
            errorExit("recv");
            
        if(strcmp(buff,"exit\n") != 0){
            strcat(msg, buff);
            printf("%s\n",buff);

            if(send(user_lock.new_fd, msg, sizeof(msg), 0) == -1)
                errorExit("send");
                
        }else{
            printf("Один из клиентов отключился");
            pthread_exit(0);
        }
    }

}

void *out(){
    
    while(1){
        char ex[5];

        fgets(ex, sizeof(ex), stdin);

        if(strcmp(ex, "exit\n")){
            pthread_exit(0);
        }
    }
}

int main(){
    struct sockaddr_in serv, client;
    struct users user, user_lock;
    int fd;
   
    socklen_t len;
    pthread_t thread[2];
    /*Создаёт сокет*/
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
        errorExit("soket");
        
    /*Инициализирует структуру*/
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9000);
    serv.sin_addr.s_addr = htons(INADDR_ANY);

    /*Делает привязку*/
    if (bind(fd, (struct sockaddr *)&serv, sizeof(serv)) == -1) 
        errorExit("bind");
        
    if (listen(fd, 5) == -1) 
        errorExit("listen");
    len = sizeof(client);

    while(1){
        
        /*подключается к клиенту*/
        user_lock.new_fd = accept(fd, (struct sockaddr *)&client, &len);
        if(user_lock.new_fd == -1)
            errorExit("accept");
           
        user.new_fd = user_lock.new_fd;
        memcpy(&user.client, &user_lock.client, sizeof(user_lock.client));
        if(pthread_create(&thread[0], NULL, server, &user) == -1)
            errorExit("pthread_create");
            
        if(pthread_create(&thread[1], NULL, out, NULL) == -1)
            errorExit("pthread_create");
            
    }
    pthread_join(thread[1], NULL);
    pthread_cancel(thread[0]);
    close(fd);
    exit(EXIT_SUCCESS);
}

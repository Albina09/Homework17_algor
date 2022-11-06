#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/time.h>

#define NAME "/qqqmq"
#define QUEUE 0660
#define M 5

struct users{
    int new_fd;
    struct sockaddr_in client;
};
struct fdd{
    int fd_UDP;
    int fd_TCP;
    fd_set readfds;
    struct users user; 
};

struct sockaddr_in serv;
pthread_mutex_t mutex;
pthread_t thread[M];
mqd_t mqd;

void errorExit(char err[]);
int max(int x, int y);

void errorExit(char err[]){
    perror(err);
    exit(EXIT_FAILURE);
} 

int max(int x, int y){
    if (x > y)
        return x;
    else
        return y;
}


void *server(){
    char buff[256];
    struct fdd fd;
    
    pthread_mutex_lock(&mutex);
    if((mq_receive(mqd,(char *) &fd, sizeof(struct fdd), NULL)) == -1)
        errorExit("mq_receive");
    pthread_mutex_unlock(&mutex);

    if(FD_ISSET(fd.fd_TCP, &fd.readfds)){
        int len = sizeof(fd.user.client);

        fd.user.new_fd = accept(fd.fd_TCP, (struct sockaddr *)&fd.user.client, &len);
        if(fd.user.new_fd == -1)
            errorExit("accept");
                
        while(1){
            char msg[256] = "Сервер получил: ";

            if (recv(fd.user.new_fd, buff, sizeof(buff), 0) == -1) 
                errorExit("recv");

            if(strcmp(buff,"exit\n") != 0){
                strcat(msg, buff);
                printf("%s\n",buff);

                if(send(fd.user.new_fd, msg, strlen(msg), 0) == -1) 
                    errorExit("send");
            }else{
                printf("%s\n", buff);
                pthread_exit(0);
            }            
        }
    } 
    
    if(FD_ISSET(fd.fd_UDP,&fd.readfds)){
        int size = sizeof(fd.user.client);
        while(1){
            char msg[256] = "Сервер получил: ";
                
            if (recvfrom(fd.fd_UDP, buff, sizeof(buff), 0, (struct sockaddr *)&fd.user.client, &size) == -1)
                errorExit("recvfrom");

            if(strcmp(buff,"exit\n") != 0){
                strcat(msg, buff);
                printf("%s\n",buff);

                if(sendto(fd.fd_UDP, msg, strlen(msg), 0, (struct sockaddr *)&fd.user.client, size) == -1)
                    errorExit("sendto");
            }else{
                printf("%s\n", buff);
                pthread_exit(0);
            }    
        }
    }
    
}

void *connec(void *arg){
    struct fdd fd = *(struct fdd*)arg;
    int fd_max = max(fd.fd_TCP, fd.fd_UDP) + 1; 
      
    while(1){
        FD_ZERO(&fd.readfds);
     
        FD_SET(fd.fd_TCP,&fd.readfds);
        FD_SET(fd.fd_UDP,&fd.readfds);

        if(select(fd_max, &fd.readfds, NULL, NULL, NULL) == -1)
            errorExit("select");
        else{
        
            if(mq_send(mqd, (const char*)&fd, sizeof(struct fdd), 0) == -1)
            errorExit("mq_send");
        }
    }
        
}

int main(){
    pthread_t thr_conn;
    struct fdd fd;

    struct timeval tv;
    tv.tv_sec = 50;
    tv.tv_usec = 0;
    /*инициализируем очередь*/
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_msgsize = sizeof(struct fdd);
    attr.mq_maxmsg = 10;
    attr.mq_curmsgs = 0; 

    /*открываем сокет TCP*/
    fd.fd_TCP = socket(AF_INET, SOCK_STREAM, 0);
    if(fd.fd_TCP == -1)
        errorExit("soket");

    /*Инициализирует стуктуру*/
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9028);
    serv.sin_addr.s_addr = htons(INADDR_ANY);

    /*Делает привязку*/
    if (bind(fd.fd_TCP, (struct sockaddr *)&serv, sizeof(serv)) == -1)
        errorExit("bind");

    if (listen(fd.fd_TCP, M) == -1)
        errorExit("listen");


    /*открываем сокет UDP*/
    fd.fd_UDP = socket(AF_INET, SOCK_DGRAM, 0);
    if(fd.fd_UDP == -1)
        errorExit("soket");

    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9028);
    serv.sin_addr.s_addr = htons(INADDR_ANY);

    if (bind(fd.fd_UDP, (struct sockaddr *)&serv, sizeof(serv)) == -1)
        errorExit("bind");

    /*создаём очередь*/    
    mqd = mq_open(NAME, O_RDWR | O_CREAT, QUEUE, &attr);
    if(mqd == (mqd_t)-1)
        errorExit("mq_open");



    for(int i = 0; i < M; i++){

        if(pthread_create(&thread[i], NULL, server, NULL) == -1)
            errorExit("pthread_create");
    }

    if(pthread_create(&thr_conn, NULL, connec, &fd) == -1)
            errorExit("pthread_create");
    
    char ex[5];
    while(1){
        fgets(ex,sizeof(ex),stdin);

        if(!strcmp(ex,"exit")){
            for(int i = 0; i < M; i++)
                pthread_cancel(thread[i]);
            
            pthread_cancel(thr_conn);
            close(fd.fd_TCP);
            close(fd.fd_UDP);
            unlink(NAME);
            
            exit(EXIT_SUCCESS);
        }
    }
    
}


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

#define NAME "/qmq"
#define QUEUE 0660
#define M 5

struct users{
    int new_fd;
    struct sockaddr_in client;
};
struct fdd{
    int fd_sock;
    mqd_t mqd; 
};
struct sockaddr_in serv;
pthread_mutex_t mutex;
pthread_t thread[M];

void errorExit(char err[]);
void errorExit(char err[]){
    perror(err);
    exit(EXIT_FAILURE);
} 

void *out(){
    char ex[5];
    
    while(1){
        fgets(ex, sizeof(ex), stdin);

        if(strcmp(ex, "exit\n")){
            pthread_exit(0);
        }
    }
}

void *server(void *arg){
    struct fdd fd = *(struct fdd*)arg;
    struct users user;
    char buff[256];
    int q;

    while(1){
        pthread_mutex_lock(&mutex);

        if((mq_receive(fd.mqd,(char *) &user, sizeof(struct users), NULL)) == -1)
            errorExit("mq_receive");
        pthread_mutex_unlock(&mutex);
        q = 1;

        while(q){
            char msg[256] = "Сервер получил: ";
        
            if (recv(user.new_fd, buff, sizeof(buff), 0) == -1) 
                errorExit("recv");

            if(strcmp(buff,"exit\n") != 0){
                strcat(msg, buff);
                printf("%s\n",buff);

                if(send(user.new_fd, msg, sizeof(msg), 0) == -1) 
                    errorExit("send");

            }else{
                printf("%s\n", buff);
                q = 0;            
            }

        }
    }
}

void *connec(void *arg){
    struct fdd fd = *(struct fdd*)arg;
    struct users user;
    
    pthread_mutex_init(&mutex, NULL);
    //открываем для ожидания 5 потоков
    for(int i = 0; i < M; i++){
        //index[i] = i; 

        if(pthread_create(&thread[i], NULL, server, &fd) == -1)
            errorExit("pthread_create");
    }

    int len = sizeof(user.client);

    while(1){

        user.new_fd = accept(fd.fd_sock, (struct sockaddr *)&user.client, &len);
        if(user.new_fd == -1)
            errorExit("accept");
        
        if(mq_send(fd.mqd, (const char*)&user, sizeof(struct users), 0) == -1)
            errorExit("mq_send");
    }

}

int main(){
    pthread_t thread_accpt,thread_out;
    struct fdd fd;
    /*инициализируем очередь*/
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_msgsize = sizeof(struct users);
    attr.mq_maxmsg = 10;
    attr.mq_curmsgs = 0; 

    /*открываем сокет*/
    fd.fd_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(fd.fd_sock == -1)
        errorExit("soket");

    /*Инициализирует стуктуру*/
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9024);
    serv.sin_addr.s_addr = htons(INADDR_ANY);

    /*Делает привязку*/
    if (bind(fd.fd_sock, (struct sockaddr *)&serv, sizeof(serv)) == -1)
        errorExit("bind");

    if (listen(fd.fd_sock, M) == -1)
        errorExit("listen");

    /*создаём очередь*/    
    fd.mqd = mq_open(NAME, O_RDWR | O_CREAT, QUEUE, &attr);
    if(fd.mqd == (mqd_t)-1)
        errorExit("mq_open");

    if(pthread_create(&thread_accpt, NULL, connec, &fd) == -1)
        errorExit("pthread_create");

    if(pthread_create(&thread_out, NULL, out, NULL) == -1)
        errorExit("pthread_create");

    pthread_join(thread_out,NULL);

    for(int i = 0; i < M; i++){
        pthread_cancel(thread[i]);
    }
    
    pthread_cancel(thread_accpt,NULL);
    close(fd.fd_sock);
    unlink(fd.mqd);
    exit(EXIT_SUCCESS);
    
}
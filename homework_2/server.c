#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <semaphore.h>
#include <fcntl.h>

#define M 7
#define N 5

struct users{
    int new_fd;
    struct sockaddr_in client;
};

struct users user;
static sem_t sem[M];
static sem_t  sem2;
int q[M];
struct sockaddr_in serv;

void errorExit(char err[]);
void errorExit(char err[]){
    perror(err);
    exit(EXIT_FAILURE);
} 



void *server(void *arg){
    int i =*(int*)arg;
    struct users user_lock;
    char buff[256];

    while(1){
        //ждём пока подключится клиент
        if(sem_wait(&sem[i]) == -1)
            errorExit("sem_wait");
        q[i] = 1;
        memcpy(&user_lock, &user,sizeof(user));

        while(q[i]){
            char msg[256] = "Сервер получил: ";

            if (recv(user_lock.new_fd, buff, sizeof(buff), 0) == -1) 
                errorExit("recv");

            if(strcmp(buff,"exit\n") != 0){
                strcat(msg, buff);
                printf("%s\n",buff);

                if(send(user_lock.new_fd, msg, sizeof(msg), 0) == -1) 
                    errorExit("send");
                
            }else{
                printf("%s\n",buff);
                //printf("поток: %d освобожден",i);
                q[i] = 0;
            }
        }
    }

}

void *connec(void *arg){
    struct users user_lock;
    int fd = *(int*)arg;
    int l = 0, index[M];
    pthread_t thread[M],out_thread;
    int len = sizeof(user_lock.client);
    memset(index, 0, sizeof(index));
    
    //создаём семафоры
    for(int i = 0; i < M; i++){

        if(sem_init(&sem[i], 0, 0)== -1)
            errorExit("sem_init");
        
    }

    if(sem_init(&sem2, 0, 0)== -1)
            errorExit("sem_init");

    //открываем для ожидания 5 потоков
    for(int i = 0; i < N; i++){
        index[i] = i; 

        if(pthread_create(&thread[i], NULL, server, &index[i]) == -1)
            errorExit("pthread_create");
    }
    
    while(1){
        int n = 0;

        user_lock.new_fd = accept(fd,(struct sockaddr *)&user_lock.client, &len);
        if(user_lock.new_fd == -1)
            errorExit("accept");
        
        memcpy(&user, &user_lock,sizeof(user_lock));

        while(n != N+l && n != -1){
            
            if(!q[n]){
                sem_post(&sem[n]);
                n = -2;
            }
            n++;
        }

        if(n == (N + l) && n != M){
            index[n] = n;
            if(pthread_create(&thread[n], NULL, server, &index[n]) == -1)
                errorExit("pthread_create");
                
            sem_post(&sem[n]);
            l++;

        }else if(n == M)
            printf("Сервер заполнен");
            
            
        
    }

    for(int i = 0; i < N + l; i++){
        pthread_join(thread[i], NULL);
    }
    pthread_exit(0);
}

int main(){
    
    pthread_t thread_acpt;
    int fd;
    memset(q, 0, sizeof(q));

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1)
        errorExit("soket");

    /*Инициализирует стуктуру*/
    serv.sin_family = AF_INET;
    serv.sin_port = htons(9023);
    serv.sin_addr.s_addr = htons(INADDR_ANY);

    /*Делает привязку*/
   
    if (bind(fd, (struct sockaddr *)&serv, sizeof(serv)) == -1)
        errorExit("bind");


    if (listen(fd, M) == -1)
        errorExit("listen");
        
   
    if(pthread_create(&thread_acpt, NULL, connec, &fd) == -1)
        errorExit("pthread_create");
        

    while(1){
        char ex[5];
        fgets(ex,sizeof(ex),stdin);

        if(!strcmp(ex,"exit")){
            pthread_cancel(thread_acpt);
            
            close(fd);
            exit(EXIT_SUCCESS);
        }
    }
    
}

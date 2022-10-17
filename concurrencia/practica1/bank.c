#include <stdio.h>
#include <threads.h>
#include <pthread.h>


struct args {
    pthread_mutex_t m;
    pthread_cond_t c;
    int flag;
};

void *f(void *ptr) {
    struct args *args = ptr;

    pthread_mutex_lock(&args->m);
    while(!args->flag)
        pthread_cond_wait(&args->c, &args->m);
    pthread_mutex_unlock(&args->m);
}

int main() {
    struct args arguments[10];
    pthread_t ids[10];

    for (int i = 0; i < 10; i++) {
        pthread_mutex_init(&arguments[i].m, NULL);
        pthread_cond_init(&arguments[i].c, NULL);
    }

    for (int i = 0; i < 10; i++) {
        pthread_create(&ids[i], NULL, f, &arguments[i]);

        for (int i = 0; i < 10; i++) {
            pthread_mutex_lock(&arguments[i].m);
            arguments[i].flag = 1;
            pthread_cond_broadcast(&arguments[i].c);
            pthread_mutex_unlock(&arguments[i].m);
            pthread_join(ids[i], NULL);
        }
    }
}
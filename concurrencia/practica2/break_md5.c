#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>

#define PASS_LEN 6
#define N_THREADS 5
struct progress {
    int          num_paswords;
    int          medicion;
    int          actual;
    int          max;
    char        **md5;
    pthread_mutex_t *mutex;
    pthread_mutex_t *mutex_found;

};
struct args{
    int id;
    struct progress *progress;
};
struct thread_info {
    pthread_t    thread;    // id returned by pthread_create()
    struct args *args;// pointer to the argument
};


long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
};

void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

int hex_value(char c) {
    if (c>='0' && c <='9')
        return c - '0';
    else if (c>= 'A' && c <='F')
        return c-'A'+10;
    else if (c>= 'a' && c <='f')
        return c-'a'+10;
    else return 0;
}

void hex_to_num(char *str, unsigned char *hex) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++)
        hex[i] = (hex_value(str[i*2]) << 4) + hex_value(str[i*2 + 1]);
}
void to_hex(unsigned char *res, char *hex_res) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++) {
        snprintf(&hex_res[i*2], 3, "%.2hhx", res[i]);
    }
    hex_res[MD5_DIGEST_LENGTH * 2] = '\0';
}

void passwd(struct progress *progress){
    int attempts=progress->actual;
    usleep(250000);
    printf("\r\033[60C                 %ld passwd/seg",(long int)(progress->actual-attempts)*4);
}
void *progressbar(void *ptr){
    float i=0;
    int j=0;
    struct args *args= ptr;
    char bar[29];
    int found=1;
    strcpy(bar,"-------------------------");
    while(true) {
        if (found==0) {
            fflush(stdout);
            printf("\r[%s] %.2f%% ",bar,((float) args->progress->actual / (float) args->progress->max)*100);
            printf(" Found! \n");
            break;
        }
        if( i <= (float) args->progress->actual / (float) args->progress->max ) {
            bar[j] = '#';
            found = args->progress->num_paswords;
            j++;
            i += 0.04f;
        }
        passwd(args->progress);
        printf("\r[%s] %.2f%% \t",bar, ((float) args->progress->actual / (float) args->progress->max)*100);
    }
    return NULL;
}
void *break_pass(void *ptr) {
    unsigned char res[MD5_DIGEST_LENGTH];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));
    char hex_res[MD5_DIGEST_LENGTH * 2 + 1];
    long bound = ipow(26, PASS_LEN); // we have passwords of PASS_LEN
    int paswords=0;
    // lowercase chars =>
    //     26 ^ PASS_LEN  different cases
    struct thread_info *thr;
    struct args *args=ptr;
    char *aux;
    for (long i=0; i < bound; i++) {
        pthread_mutex_lock(args->progress->mutex_found);
        if (args->progress->num_paswords != 0) {
            pthread_mutex_unlock(args->progress->mutex_found);
            pthread_mutex_lock(args->progress->mutex);
            args->progress->actual++;
            pthread_mutex_unlock(args->progress->mutex);
            long_to_pass(args->progress->actual, pass);
            MD5(pass, PASS_LEN, res);
            to_hex(res, hex_res);
            pthread_mutex_lock(args->progress->mutex_found);
            for (int i = 0; i < args->progress->num_paswords; ++i) {
                if (!strcmp(hex_res, args->progress->md5[i])) { ;
                    printf("\n%s: %s\n", args->progress->md5[i],pass);
                    for (int j = i; j < (args->progress->num_paswords - 1); j++) {
                        aux = args->progress->md5[j];
                        args->progress->md5[j] = args->progress->md5[j + 1];
                        args->progress->md5[j + 1] = aux;
                    }

                    args->progress->num_paswords--;
                    break;
                }// Found it!
            }
            pthread_mutex_unlock(args->progress->mutex_found);
        }
        else{
            pthread_mutex_unlock(args->progress->mutex_found);
            break;
        }
    }
    return NULL;
}

struct thread_info *start_threads(struct progress *progress) {
    int i;
    struct thread_info *threads;
    printf("creating %d threads\n", N_THREADS);
    threads = malloc(sizeof(struct thread_info) * N_THREADS);
    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }
    // Create num_thread threads running swap()
    for (i = 0; i < N_THREADS+1; i++) {
        threads[i].args = malloc(sizeof(struct args));
        threads[i].args->progress= progress;
        threads[i].args->id= i;
        if(i < N_THREADS) {
            if (0 != pthread_create(&threads[i].thread, NULL, break_pass, threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }
        }
        else{
            if (0 != pthread_create(&threads[i].thread, NULL, progressbar, threads[i].args)) {
                printf("Could not create thread #%d", i);
                exit(1);
            }
        }
    }
    return threads;
}
void wait (struct thread_info *threads,struct progress *progress,char argv[]) {
    for (int i = 0; i < N_THREADS; i++) {
        pthread_join(threads[i].thread, NULL);
    }
    pthread_mutex_destroy(progress->mutex);
    pthread_mutex_destroy(progress->mutex_found);
    /*for (int i = 0; i < N_THREADS; i++)
        free(threads[i].args);*/
    free(threads);
    free(progress->mutex);
    free(progress->mutex_found);

}
void init_data(struct progress *progress, unsigned char *md5,char *argv[],int passwords) {
    progress->md5 = malloc(sizeof(char*) * passwords);
    progress->actual = 0;
    for (int i = 0; i < passwords; ++i)
        progress->md5[i] = argv[i+1];
    progress->medicion=0;
    progress->max= ipow(26, PASS_LEN);
    progress->mutex = malloc(sizeof(pthread_mutex_t));
    progress->mutex_found = malloc(sizeof(pthread_mutex_t));
    progress->num_paswords=passwords;
    if (pthread_mutex_init(progress->mutex, NULL) != 0)
        printf("Error, no se pudo inicializar\n");
}
int main(int argc, char *argv[]) {
    struct thread_info *thrs;
    struct progress progress;
    if(argc < 2) {
        printf("Use: %s string\f", argv[0]);
        exit(0);
    }
    unsigned char md5_num[MD5_DIGEST_LENGTH];
    hex_to_num(argv[1], md5_num);
    /*char *pass = break_pass(md5_num);
    printf("%s: %s\n", argv[1], pass)
    free(pass);*/
    init_data(&progress, md5_num,argv,--argc);

    thrs = start_threads(&progress);

    wait(thrs, &progress,argv[1]);

    return 0;
}
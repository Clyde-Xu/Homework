#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>

#include "ht530_drv.h"

#define DEVICE "/dev/ht530"
#define TOTAL_OPERTION 100
#define MAX_KEY 50
#define MAX_VALUE 1000

const int thread_priority[] = {90, 100, 95, 96};
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int number = 0;
struct timespec period;

void *add(void *arg) {
    while (1) {
        pthread_mutex_lock(&mtx);
        if (number >= TOTAL_OPERTION) {
            pthread_mutex_unlock(&mtx);
            return;
        }
        int fd = (int)arg;
        ht_object_t object;
        object.key = rand() % MAX_KEY;
        // Make sure data is not 0
        object.data = rand() % MAX_VALUE + 1;
        int result = write(fd, &object, sizeof(ht_object_t));
        if (result != -1) {
            printf("write: object key = %d, data = %d is added.\n", object.key, object.data);
        }
        nanosleep(&period, NULL);
        pthread_mutex_unlock(&mtx);
    }
}

void *search(void *arg) {
    while (1) {
        pthread_mutex_lock(&mtx);
        if (number >= TOTAL_OPERTION) {
            pthread_mutex_unlock(&mtx);
            return;
        }
        int fd = (int)arg;
        ht_object_t object;
        object.key = rand() % MAX_KEY; 
        int result = read(fd, &object, sizeof(ht_object_t));
        number++;
        if (result == -1) {
            printf("search: object key = %d does not exist.\n", object.key);
        } else {
            printf("search: key = %d, data = %d\n", object.key, object.data);
        }
        nanosleep(&period, NULL);
        pthread_mutex_unlock(&mtx);
    }
}

void *delete(void *arg) {
    while (1) {
        pthread_mutex_lock(&mtx);
        if (number >= TOTAL_OPERTION) {
            pthread_mutex_unlock(&mtx);
            return;
        }
        int fd = (int)arg;
        ht_object_t object;
        object.key = rand() % MAX_KEY;
        object.data = 0;
        write(fd, &object, sizeof(ht_object_t));
        number++;
        printf("delete: object with key %d is removed.\n", object.key);
        nanosleep(&period, NULL);
        pthread_mutex_unlock(&mtx);
    }
}

int main(void) {
    int fd = open(DEVICE, O_RDWR);
    if (fd == -1) {
        perror(NULL);
        return -1;
    }
    period.tv_sec = 0;
    period.tv_nsec = 1000;
    int i, j;
    for (i = 1; i <= MAX_KEY; i++) {
        ht_object_t object;
        object.key = i;
        object.data = rand() % MAX_VALUE;
        write(fd, &object, sizeof(ht_object_t));
    }
    pthread_t threads[4];
    struct sched_param param;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    int result, policy;
    srand(time(NULL));
    for (i = 0; i < 4; i++) {
        param.sched_priority = thread_priority[i];
        pthread_attr_setschedparam(&attr, &param);
        void *(*func)(void *);
        if (i < 2) {
            func = add;
        } else if (i == 2) {
            func = search;
        } else {
            func = delete;
        }
        result = pthread_create(&threads[i], &attr, func, (void *)fd);
        if (result) {
            printf("Failed to create thread with error %d", result);
        }
    }
    pthread_attr_destroy(&attr);
    for (i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    dump_arg_t arg;
    for (i = 0; i < 128; i++) {
        arg.n = i;
        result = ioctl(fd, DUMP, &arg);
        if (result == -1) {
            printf("Failed to dump.\n");
            continue;
        }
        ht_object_t *object = arg.object_array;
        for (j = 0; j < result; j++) {
            printf("slot #%d: key=%d, data=%d\n", i, object[j].key, object[j].data);
        }
    }
    close(fd);
}

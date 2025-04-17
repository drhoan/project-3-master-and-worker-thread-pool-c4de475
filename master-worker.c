#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

int item_to_produce = 0, curr_buf_size = 0;
int total_items, max_buf_size, num_workers, num_masters;

int *buffer;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void print_produced(int num, int master) {
    printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {
    printf("Consumed %d by worker %d\n", num, worker);
}

// Produce items and place in buffer
void *generate_requests_loop(void *data) {
    int thread_id = *((int *)data);
    free(data);

    while (1) {
        pthread_mutex_lock(&mutex);

        if (item_to_produce >= total_items) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        while (curr_buf_size >= max_buf_size) {
            pthread_cond_wait(&full, &mutex);
        }

        if (item_to_produce < total_items) {
            buffer[curr_buf_size++] = item_to_produce;
            print_produced(item_to_produce, thread_id);
            item_to_produce++;
            pthread_cond_signal(&empty);
        }

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

// Consume items from buffer
void *consume_requests_loop(void *data) {
    int thread_id = *((int *)data);
    free(data);

    while (1) {
        pthread_mutex_lock(&mutex);

        while (curr_buf_size == 0 && item_to_produce < total_items) {
            pthread_cond_wait(&empty, &mutex);
        }

        if (curr_buf_size == 0 && item_to_produce >= total_items) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        int item = buffer[--curr_buf_size];
        print_consumed(item, thread_id);
        pthread_cond_signal(&full);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("./master-worker #total_items #max_buf_size #num_workers #masters\n");
        exit(1);
    }

    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
    num_workers = atoi(argv[3]);
    num_masters = atoi(argv[4]);

    buffer = (int *)malloc(sizeof(int) * max_buf_size);
    pthread_t master_threads[num_masters], worker_threads[num_workers];

    for (int i = 0; i < num_masters; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&master_threads[i], NULL, generate_requests_loop, id);
    }

    for (int i = 0; i < num_workers; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&worker_threads[i], NULL, consume_requests_loop, id);
    }

    for (int i = 0; i < num_masters; i++) {
        pthread_join(master_threads[i], NULL);
    }

    for (int i = 0; i < num_workers; i++) {
        pthread_join(worker_threads[i], NULL);
    }

    free(buffer);
    return 0;
}
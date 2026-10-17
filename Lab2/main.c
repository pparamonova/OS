#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define DECK_SIZE 52
#define RANKS 13

typedef struct
{
    long num_experiments;
    long successes;
} ThreadData;

void *worker(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)pthread_self();

    for (long i = 0; i < data->num_experiments; i++)
    {
        int card1 = rand_r(&seed) % DECK_SIZE;
        int card2 = rand_r(&seed) % DECK_SIZE;

        while (card2 == card1)
        {
            card2 = rand_r(&seed) % DECK_SIZE;
        }

        if ((card1 % RANKS) == (card2 % RANKS))
        {
            data->successes++;
        }
    }
    return NULL;
}

double get_time_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(int argc, char *argv[])
{
    long total_experiments = 1000000;
    int num_threads = 1;
    int opt;

    while ((opt = getopt(argc, argv, "n:t:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            total_experiments = atol(optarg);
            break;
        case 't':
            num_threads = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s [-n experiments] [-t threads]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    printf("Sequential version started...\n");
    double start_seq = get_time_ms();

    ThreadData seq_data = {total_experiments, 0};
    worker(&seq_data);

    double end_seq = get_time_ms();
    double time_seq = end_seq - start_seq;
    double prob_seq = (double)seq_data.successes / total_experiments;

    printf("Sequential Time: %.2f ms\n", time_seq);
    printf("Probability (Seq): %.6f\n\n", prob_seq);

    printf("Parallel version started with %d threads...\n", num_threads);
    double start_par = get_time_ms();

    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData *thread_data = malloc(num_threads * sizeof(ThreadData));

    long exp_per_thread = total_experiments / num_threads;
    long remainder = total_experiments % num_threads;

    for (int i = 0; i < num_threads; i++)
    {
        thread_data[i].num_experiments = exp_per_thread;
        thread_data[i].successes = 0;
        if (i < remainder)
        {
            thread_data[i].num_experiments++;
        }
        pthread_create(&threads[i], NULL, worker, &thread_data[i]);
    }

    long total_successes_par = 0;
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        total_successes_par += thread_data[i].successes;
    }

    double end_par = get_time_ms();
    double time_par = end_par - start_par;
    double prob_par = (double)total_successes_par / total_experiments;

    printf("Parallel Time: %.2f ms\n", time_par);
    printf("Probability (Par): %.6f\n", prob_par);

    double speedup = time_seq / time_par;
    double efficiency = speedup / num_threads;

    printf("\nMetrics\n");
    printf("Speedup: %.4f\n", speedup);
    printf("Efficiency: %.4f\n", efficiency);

    free(threads);
    free(thread_data);

    return 0;
}
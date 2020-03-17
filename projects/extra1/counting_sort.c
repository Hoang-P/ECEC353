/* Program to perform counting sort 
 *
 * Author: Naga Kandasamy
 * Date created: February 24, 2020
 * 
 * Compile as follows: gcc -o counting_sort counting_sort.c -std=c99 -Wall -O3 -lpthread -lm
 */

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>

/* Do not change the range value. */
#define MIN_VALUE 0 
#define MAX_VALUE 1023

/* Comment out if you don't need debug info */
// #define DEBUG
// #define DEBUG_MORE_VERBOSE

/* Shared data structure used by the threads */
typedef struct args_for_thread_t {
    int tid;                          /* The thread ID */
    int num_threads;                  /* Number of worker threads */
    int num_elements;                 /* Number of elements in the vectors */
    int *input;                       /* Input array */
    int *sorted;                      /* Sorted array */
    int *bin;                         /* Bin */
    int *local_bin;                         /* Bin */
    // int *lock;                        /* Locks */
    int range;                        /* Range */
    int offset;                       /* Starting offset for thread within the vectors */
    int chunk_size;                   /* Chunk size */
    pthread_mutex_t *mutex_for_sum;   /* Location of the lock variable protecting sum */
    pthread_barrier_t *barrier;
    pthread_barrier_t *barrier2;
    int *thread_bin;
    sem_t *lock;
} ARGS_FOR_THREAD;

int compute_gold (int *, int *, int, int);
int rand_int (int, int);
void print_array (int *, int);
void print_min_and_max_in_array (int *, int);
void compute_using_pthreads (int *, int *, int, int, int);
int check_if_sorted (int *, int);
int compare_results (int *, int *, int);
void print_histogram (int *, int, int);
void *counting_sort (void *args);

int 
main (int argc, char **argv)
{
    if (argc != 3) {
        printf ("Usage: %s num-elements num-threads\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    int num_elements = atoi (argv[1]);
    int num_threads = atoi (argv[2]);

    int range = MAX_VALUE - MIN_VALUE;
    int *input_array, *sorted_array_reference, *sorted_array_d;

    /* Compute time */
	struct timeval start, stop, start1, stop1;

    /* Populate the input array with random integers between [0, RANGE]. */
    printf ("Generating input array with %d elements in the range 0 to %d\n", num_elements, range);
    input_array = (int *) malloc (num_elements * sizeof (int));
    if (input_array == NULL) {
        printf ("Cannot malloc memory for the input array. \n");
        exit (EXIT_FAILURE);
    }
    srand (time (NULL));
    for (int i = 0; i < num_elements; i++)
        input_array[i] = rand_int (MIN_VALUE, MAX_VALUE);

#ifdef DEBUG
    print_array (input_array, num_elements);
    print_min_and_max_in_array (input_array, num_elements);
#endif

    /* Sort the elements in the input array using the reference implementation. 
     * The result is placed in sorted_array_reference. */
    printf ("\nSorting array using serial version\n");
    int status;
    sorted_array_reference = (int *) malloc (num_elements * sizeof (int));
    if (sorted_array_reference == NULL) {
        perror ("Malloc"); 
        exit (EXIT_FAILURE);
    }
    memset (sorted_array_reference, 0, num_elements);
    gettimeofday (&start, NULL);
    status = compute_gold (input_array, sorted_array_reference, num_elements, range);
    gettimeofday (&stop, NULL);
    if (status == 0) {
        exit (EXIT_FAILURE);
    }

    status = check_if_sorted (sorted_array_reference, num_elements);
    if (status == 0) {
        printf ("Error sorting the input array using the reference code\n");
        exit (EXIT_FAILURE);
    }

    printf ("Counting sort was successful using reference version\n");

#ifdef DEBUG
    print_array (sorted_array_reference, num_elements);
#endif

    /* FIXME: Write function to sort the elements in the array in parallel fashion. 
     * The result should be placed in sorted_array_mt. */
    printf ("\nSorting array using pthreads\n");
    sorted_array_d = (int *) malloc (num_elements * sizeof (int));
    if (sorted_array_d == NULL) {
        perror ("Malloc");
        exit (EXIT_FAILURE);
    }
    memset (sorted_array_d, 0, num_elements);
    gettimeofday (&start1, NULL);
    compute_using_pthreads (input_array, sorted_array_d, num_elements, range, num_threads);
    gettimeofday (&stop1, NULL);
    // print_array (sorted_array_d, num_elements);

    /* Check the two results for correctness. */
    printf ("\nComparing reference and pthread results\n");
    status = compare_results (sorted_array_reference, sorted_array_d, num_elements);
    if (status == 1)
        printf ("Test passed\n");
    else
        printf ("Test failed\n");
    
    printf ("\n");
    printf ("Ref Execution time = %fs\n", (float) (stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float) 1000000));
    printf ("Thread Execution time = %fs\n", (float) (stop1.tv_sec - start1.tv_sec + (stop1.tv_usec - start1.tv_usec)/(float) 1000000));
	printf ("\n");

    exit (EXIT_SUCCESS);
}

/* Reference implementation of counting sort. */
int 
compute_gold (int *input_array, int *sorted_array, int num_elements, int range)
{
    /* Compute histogram. Generate bin for each element within
     * the range. 
     * */
    int i, j;
    int num_bins = range + 1;
    int *bin = (int *) malloc (num_bins * sizeof (int));    
    if (bin == NULL) {
        perror ("Malloc");
        return 0;
    }

    memset(bin, 0, num_bins); /* Initialize histogram bins to zero */ 
    for (i = 0; i < num_elements; i++)
        bin[input_array[i]]++;

#ifdef DEBUG_MORE_VERBOSE
    print_histogram (bin, num_bins, num_elements);
#endif

    /* Generate the sorted array. */
    int idx = 0;
    for (i = 0; i < num_bins; i++) {
        for (j = 0; j < bin[i]; j++) {
            sorted_array[idx++] = i;
        }
    }

    return 1;
}

/* FIXME: Write multi-threaded implementation of counting sort. */
void 
compute_using_pthreads (int *input_array, int *sorted_array, int num_elements, int range, int num_threads)
{
    int i;
    pthread_t *tid = (pthread_t *) malloc (sizeof (pthread_t) * num_threads); /* Data structure to store the thread IDs */
    if (tid == NULL) {
        perror ("malloc");
        exit (EXIT_FAILURE);
    }

    pthread_attr_t attributes;                  /* Thread attributes */
    pthread_attr_init (&attributes);            /* Initialize the thread attributes to the default values */

    pthread_mutex_t *mutex_for_sum = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t) * num_threads);   /* Lock for the shared variable sum */
    for (i = 0; i < MAX_VALUE; i++)
        pthread_mutex_init (&mutex_for_sum[i], NULL);
    
    int num_bins = range + 1;
    int *bin = (int *) malloc (num_bins * sizeof (int));
    if (bin == NULL) {
        perror ("Malloc");
        return;
    }
    memset(bin, 0, num_bins); /* Initialize histogram bins to zero */

    int *thread_bin = (int *) malloc (num_threads * num_bins * sizeof (int));
    if (thread_bin == NULL) {
        perror ("Malloc");
        return;
    }
    memset(thread_bin, 0, num_threads * num_bins); /* Initialize histogram bins to zero */

    pthread_barrier_t *barrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t *));
    pthread_barrier_init(barrier,NULL,num_threads);


    pthread_barrier_t *barrier2 = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t *));
    pthread_barrier_init(barrier2,NULL,num_threads);

    ARGS_FOR_THREAD **args_for_thread;
    args_for_thread = malloc (sizeof (ARGS_FOR_THREAD) * num_threads);
    int chunk_size = (int) floor ((float) num_bins/(float) num_threads); // Compute the chunk size
    for (i = 0; i < num_threads; i++) {
        args_for_thread[i] = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD));
        args_for_thread[i]->tid = i;
        args_for_thread[i]->num_threads = num_threads;
        args_for_thread[i]->num_elements = num_elements;
        args_for_thread[i]->input = input_array;
        args_for_thread[i]->sorted = sorted_array;
        args_for_thread[i]->bin = bin;
        args_for_thread[i]->thread_bin = thread_bin;
        args_for_thread[i]->sorted = sorted_array;
        args_for_thread[i]->range = range;
        args_for_thread[i]->barrier = barrier;
        args_for_thread[i]->barrier2 = barrier2;
        args_for_thread[i]->offset = i * chunk_size;
        args_for_thread[i]->chunk_size = chunk_size;
        pthread_create (&tid[i], &attributes, counting_sort, (void *) args_for_thread[i]);
    }

    /* Wait for the workers to finish */
    for(i = 0; i < num_threads; i++) {
        pthread_join (tid[i], NULL);
    }

    /* Free data structures */
    for(i = 0; i < num_threads; i++)
        free ((void *) args_for_thread[i]);
    
    return;
}

void *
counting_sort (void *args)
{
    ARGS_FOR_THREAD *args_for_me = (ARGS_FOR_THREAD *) args; /* Typecast the argument to a pointer the the ARGS_FOR_THREAD structure */
    int num_bins = args_for_me->range + 1;
    args_for_me->local_bin = (int *) malloc (num_bins * sizeof (int));
    memset(args_for_me->local_bin, 0, num_bins);
    int idx = 0;

    if (args_for_me->tid < (args_for_me->num_threads - 1)) {
        for (int i = args_for_me->offset; i < (args_for_me->offset + args_for_me->chunk_size); i++)
            args_for_me->thread_bin[args_for_me->tid * num_bins + args_for_me->input[i]]++;
    }
    else { /* This takes care of the number of elements that the final thread must process */
        for (int i = args_for_me->offset; i < args_for_me->num_elements; i++)
            args_for_me->thread_bin[args_for_me->tid * num_bins + args_for_me->input[i]]++;
    }

    pthread_barrier_wait(args_for_me->barrier);
    if (args_for_me->tid < (args_for_me->num_threads - 1)) 
    {
        for (int i = args_for_me->offset; i < (args_for_me->offset + args_for_me->chunk_size); i++)
            for (int j = 0; j < args_for_me->num_threads; j++)
                args_for_me->bin[i] += args_for_me->thread_bin[j * num_bins + i];
    }
    else
    {
        for (int i = args_for_me->offset; i < num_bins; i++)
            for (int j = 0; j < args_for_me->num_threads; j++)
                args_for_me->bin[i] += args_for_me->thread_bin[j * num_bins + i];
    }
    

    pthread_barrier_wait(args_for_me->barrier2);
    for (int i = 0; i < args_for_me->offset;i++)
        idx += args_for_me->bin[i];
    /* Generate the sorted array. */
    if (args_for_me->tid < (args_for_me->num_threads - 1)) {
        for (int i = args_for_me->offset; i < (args_for_me->offset + args_for_me->chunk_size); i++)
            for (int j = 0; j < args_for_me->bin[i]; j++)
                args_for_me->sorted[idx++] = i;
    }
    else { /* This takes care of the number of elements that the final thread must process */
        for (int i = args_for_me->offset; i < num_bins; i++)
            for (int j = 0; j < args_for_me->bin[i]; j++)
                args_for_me->sorted[idx++] = i;
    }
    
    pthread_exit ((void *)0);
}

/* Check if the array is sorted. */
int
check_if_sorted (int *array, int num_elements)
{
    int status = 1;
    for (int i = 1; i < num_elements; i++) {
        if (array[i - 1] > array[i]) {
            status = 0;
            break;
        }
    }

    return status;
}

/* Check if the arrays elements are identical. */ 
int 
compare_results (int *array_1, int *array_2, int num_elements)
{
    int status = 1;
    for (int i = 0; i < num_elements; i++) {
        if (array_1[i] != array_2[i]) {
            status = 0;
            break;
        }
    }

    return status;
}


/* Returns a random integer between [min, max]. */ 
int
rand_int (int min, int max)
{
    float r = rand ()/(float) RAND_MAX;
    return (int) floorf (min + (max - min) * r);
}

/* Helper function to print the given array. */
void
print_array (int *this_array, int num_elements)
{
    printf ("Array: ");
    for (int i = 0; i < num_elements; i++)
        printf ("%d ", this_array[i]);
    printf ("\n");
    return;
}

/* Helper function to return the min and max values in the given array. */
void 
print_min_and_max_in_array (int *this_array, int num_elements)
{
    int i;

    int current_min = INT_MAX;
    for (i = 0; i < num_elements; i++)
        if (this_array[i] < current_min)
            current_min = this_array[i];

    int current_max = INT_MIN;
    for (i = 0; i < num_elements; i++)
        if (this_array[i] > current_max)
            current_max = this_array[i];

    printf ("Minimum value in the array = %d\n", current_min);
    printf ("Maximum value in the array = %d\n", current_max);
    return;
}

/* Helper function to print the contents of the histogram. */
void 
print_histogram (int *bin, int num_bins, int num_elements)
{
    int num_histogram_entries = 0;
    int i;

    for (i = 0; i < num_bins; i++) {
        printf ("Bin %d: %d\n", i, bin[i]);
        num_histogram_entries += bin[i];
    }

    printf ("Number of elements in the input array = %d \n", num_elements);
    printf ("Number of histogram elements = %d \n", num_histogram_entries);

    return;
}


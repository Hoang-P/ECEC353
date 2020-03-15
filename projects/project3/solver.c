/* Code for the Jacobi equation solver. 
 * Author: Naga Kandasamy
 * Date created: April 19, 2019
 * Date modified: February 21, 2020
 *
 * Compile as follows:
 * gcc -o solver solver.c solver_gold.c -O3 -Wall -std=c99 -lm -lpthread
 *
 * If you wish to see debug info, add the -D DEBUG option when compiling the code.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include "grid.h" 

/* Shared data structure used by the threads */
typedef struct args_for_thread_t {
    int tid;                          /* The thread ID */
    int num_threads;                  /* Number of worker threads */
    int num_elements;                 /* Number of elements in the vectors */
    float old;                        /* Old grid value */
    float new;                        /* New grid value */
    double diff;                      /* Grid value difference */
    grid_t *grid;                     /* Grid */
    grid_t *grid2;                     /* Grid */
    pthread_barrier_t *barrier;
    double *global_diff;
    int done;
    int *num_iter;
} ARGS_FOR_THREAD;

extern int compute_gold (grid_t *);
int compute_using_pthreads_jacobi (grid_t *, int);
void compute_grid_differences(grid_t *, grid_t *);
grid_t *create_grid (int, float, float);
grid_t *copy_grid (grid_t *);
void print_grid (grid_t *);
void print_stats (grid_t *);
double grid_mse (grid_t *, grid_t *);
void * jacobi (void *args);


int 
main (int argc, char **argv)
{	
	if (argc < 5) {
        printf ("Usage: %s grid-dimension num-threads min-temp max-temp\n", argv[0]);
        printf ("grid-dimension: The dimension of the grid\n");
        printf ("num-threads: Number of threads\n"); 
        printf ("min-temp, max-temp: Heat applied to the north side of the plate is uniformly distributed between min-temp and max-temp\n");
        exit (EXIT_FAILURE);
    }
    
    /* Parse command-line arguments. */
    int dim = atoi (argv[1]);
    int num_threads = atoi (argv[2]);
    float min_temp = atof (argv[3]);
    float max_temp = atof (argv[4]);
    
    /* Generate the grids and populate them with initial conditions. */
 	grid_t *grid_1 = create_grid (dim, min_temp, max_temp);
    /* Grid 2 should have the same initial conditions as Grid 1. */
    grid_t *grid_2 = copy_grid (grid_1); 

    /* Compute time */
	struct timeval start, stop, start1, stop1;	

	/* Compute the reference solution using the single-threaded version. */
	printf ("\nUsing the single threaded version to solve the grid\n");
    gettimeofday (&start, NULL);
	int num_iter = compute_gold (grid_1);
    gettimeofday (&stop, NULL);
	printf ("Convergence achieved after %d iterations\n", num_iter);
    /* Print key statistics for the converged values. */
	printf ("Printing statistics for the interior grid points\n");
    print_stats (grid_1);
#ifdef DEBUG
    print_grid (grid_1);
#endif

	/* Use pthreads to solve the equation using the jacobi method. */
	printf ("\nUsing pthreads to solve the grid using the jacobi method\n");
    gettimeofday (&start1, NULL);
	num_iter = compute_using_pthreads_jacobi (grid_2, num_threads);
    gettimeofday (&stop1, NULL);
	printf ("Convergence achieved after %d iterations\n", num_iter);			
    printf ("Printing statistics for the interior grid points\n");
	print_stats (grid_2);
#ifdef DEBUG
    print_grid (grid_2);
#endif
    
    /* Compute grid differences. */
    double mse = grid_mse (grid_1, grid_2);
    printf ("MSE between the two grids: %f\n", mse);

	/* Free up the grid data structures. */
	free ((void *) grid_1->element);	
	free ((void *) grid_1); 
	free ((void *) grid_2->element);	
	free ((void *) grid_2);

    printf ("\n");
    printf ("Ref Execution time = %fs\n", (float) (stop.tv_sec - start.tv_sec + (stop.tv_usec - start.tv_usec)/(float) 1000000));
    printf ("Thread Execution time = %fs\n", (float) (stop1.tv_sec - start1.tv_sec + (stop1.tv_usec - start1.tv_usec)/(float) 1000000));
	printf ("\n");

	exit (EXIT_SUCCESS);
}

/* FIXME: Edit this function to use the jacobi method of solving the equation. The final result should be placed in the grid data structure. */
int 
compute_using_pthreads_jacobi (grid_t *grid, int num_threads)
{	
    pthread_t *tid = (pthread_t *) malloc (sizeof (pthread_t) * num_threads); /* Data structure to store the thread IDs */
    if (tid == NULL) {
        perror ("malloc");
        exit (EXIT_FAILURE);
    }

    pthread_attr_t attributes;                  /* Thread attributes */    
    pthread_attr_init (&attributes);            /* Initialize the thread attributes to the default values */

    ARGS_FOR_THREAD **args_for_thread;
    args_for_thread = malloc (sizeof (ARGS_FOR_THREAD) * num_threads);
    // int chunk_size = (int) floor ((float) (grid->dim - 1)/(float) num_threads); // Compute the chunk size

    int *num_iter = (int *) malloc (num_threads * sizeof (int));
    if (num_iter == NULL) {
        perror ("Malloc");
        return 1;
    }
    memset(num_iter, 0, num_threads);
    int i, j;
    grid_t *grid2 = copy_grid(grid);
    pthread_barrier_t *barrier = (pthread_barrier_t *)malloc(sizeof(pthread_barrier_t *));
    pthread_barrier_init(barrier,NULL,num_threads);
    double *diff = (double *) malloc (num_threads * sizeof (double));
    if (diff == NULL) {
        perror ("Malloc");
        return 1;
    }
    
    // print_grid(grid);

    for (i = 0; i < num_threads; i++) {
        args_for_thread[i] = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD));
        args_for_thread[i]->tid = i;
        args_for_thread[i]->num_threads = num_threads;
        args_for_thread[i]->num_elements = (grid->dim - 1)*(grid->dim - 1);
        args_for_thread[i]->diff = 0.0;
        args_for_thread[i]->grid = grid;
        args_for_thread[i]->grid2 = grid2;
        args_for_thread[i]->barrier = barrier;
        // args_for_thread[i]->barrier2 = barrier2;
        args_for_thread[i]->global_diff = diff;
        args_for_thread[i]->done = 0;
        args_for_thread[i]->num_iter = num_iter;
        pthread_create (&tid[i], &attributes, jacobi, (void *) args_for_thread[i]);
    }

    for (i = 0; i < num_threads; i++)
        pthread_join (tid[i], NULL);

    /* Free data structures */
    for(j = 0; j < num_threads; j++)
        free ((void *) args_for_thread[j]);

    int final_iter = num_iter[0];
    // for(i = 0; i < num_threads; i++)
    // {
    //     final_iter += num_iter[i];
    // }
        
    return final_iter;
}

void *
jacobi (void *args)
{
    ARGS_FOR_THREAD *args_for_me = (ARGS_FOR_THREAD *) args; /* Typecast the argument to a pointer the the ARGS_FOR_THREAD structure */
    grid_t *grid = args_for_me->grid;
    grid_t *grid2 = args_for_me->grid2;
    float eps = 1e-4;
    double total;
    int pingpong = 1;

    while(!args_for_me->done)
    {
        total = 0.0;
        args_for_me->diff = 0.0;
        for (int i = (args_for_me->tid) + 1; i < (grid->dim - 1); i += args_for_me->num_threads) {
            for (int j = 1; j < (grid->dim - 1); j++) {
                args_for_me->old = pingpong == 1 ? (grid->element[i * grid->dim + j]) : (grid2->element[i * grid2->dim + j]);
                /* Apply the update rule. */
                args_for_me->new = pingpong == 1 ? ( 0.25 * (grid->element[(i - 1) * grid->dim + j] +\
                                                        grid->element[(i + 1) * grid->dim + j] +\
                                                        grid->element[i * grid->dim + (j + 1)] +\
                                                        grid->element[i * grid->dim + (j - 1)] )):\
                                              ( 0.25 * (grid2->element[(i - 1) * grid2->dim + j] +\
                                                        grid2->element[(i + 1) * grid2->dim + j] +\
                                                        grid2->element[i * grid2->dim + (j + 1)] +\
                                                        grid2->element[i * grid2->dim + (j - 1)] ));
                
                /* Update the grid-point value. */
                if (pingpong)
                    grid2->element[i * grid2->dim + j] = args_for_me->new;
                else
                    grid->element[i * grid->dim + j] = args_for_me->new;

                args_for_me->diff += fabs(args_for_me->new - args_for_me->old); /* Calculate the difference in values. */
            }
        }

        args_for_me->global_diff[args_for_me->tid] = args_for_me->diff;
        // printf("%f\n", args_for_me->global_diff[args_for_me->tid]);
        args_for_me->num_iter[args_for_me->tid] += 1;
        pingpong = !pingpong;
        pthread_barrier_wait(args_for_me->barrier);
        for(int i = 0; i < args_for_me->num_threads; i++)
            total += args_for_me->global_diff[i];
        total = total/args_for_me->num_elements;
        if (total < eps)
            args_for_me->done = 1;
    }

    pthread_exit ((void *)0);
}



/* Create a grid with the specified initial conditions. */
grid_t * 
create_grid (int dim, float min, float max)
{
    grid_t *grid = (grid_t *) malloc (sizeof (grid_t));
    if (grid == NULL)
        return NULL;

    grid->dim = dim;
	printf("Creating a grid of dimension %d x %d\n", grid->dim, grid->dim);
	grid->element = (float *) malloc (sizeof (float) * grid->dim * grid->dim);
    if (grid->element == NULL)
        return NULL;

    int i, j;
	for (i = 0; i < grid->dim; i++) {
		for (j = 0; j < grid->dim; j++) {
            grid->element[i * grid->dim + j] = 0.0; 			
		}
    }

    /* Initialize the north side, that is row 0, with temperature values. */ 
    srand ((unsigned) time (NULL));
	float val;		
    for (j = 1; j < (grid->dim - 1); j++) {
        val =  min + (max - min) * rand ()/(float)RAND_MAX;
        grid->element[j] = val; 	
    }

    return grid;
}

/* Creates a new grid and copies over the contents of an existing grid into it. */
grid_t *
copy_grid (grid_t *grid) 
{
    grid_t *new_grid = (grid_t *) malloc (sizeof (grid_t));
    if (new_grid == NULL)
        return NULL;

    new_grid->dim = grid->dim;
	new_grid->element = (float *) malloc (sizeof (float) * new_grid->dim * new_grid->dim);
    if (new_grid->element == NULL)
        return NULL;

    int i, j;
	for (i = 0; i < new_grid->dim; i++) {
		for (j = 0; j < new_grid->dim; j++) {
            new_grid->element[i * new_grid->dim + j] = grid->element[i * new_grid->dim + j] ; 			
		}
    }

    return new_grid;
}

/* This function prints the grid on the screen. */
void 
print_grid (grid_t *grid)
{
    int i, j;
    for (i = 0; i < grid->dim; i++) {
        for (j = 0; j < grid->dim; j++) {
            printf ("%f\t", grid->element[i * grid->dim + j]);
        }
        printf ("\n");
    }
    printf ("\n");
}


/* Print out statistics for the converged values of the interior grid points, including min, max, and average. */
void 
print_stats (grid_t *grid)
{
    float min = INFINITY;
    float max = 0.0;
    double sum = 0.0;
    int num_elem = 0;
    int i, j;

    for (i = 1; i < (grid->dim - 1); i++) {
        for (j = 1; j < (grid->dim - 1); j++) {
            sum += grid->element[i * grid->dim + j];

            if (grid->element[i * grid->dim + j] > max) 
                max = grid->element[i * grid->dim + j];

             if(grid->element[i * grid->dim + j] < min) 
                min = grid->element[i * grid->dim + j];
             
             num_elem++;
        }
    }
                    
    printf("AVG: %f\n", sum/num_elem);
	printf("MIN: %f\n", min);
	printf("MAX: %f\n", max);
	printf("\n");
}

/* Calculate the mean squared error between elements of two grids. */
double
grid_mse (grid_t *grid_1, grid_t *grid_2)
{
    double mse = 0.0;
    int num_elem = grid_1->dim * grid_1->dim;
    int i;

    for (i = 0; i < num_elem; i++) 
        mse += (grid_1->element[i] - grid_2->element[i]) * (grid_1->element[i] - grid_2->element[i]);
                   
    return mse/num_elem; 
}
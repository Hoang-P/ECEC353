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

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "grid.h" 

/* Shared data structure used by the threads */
typedef struct args_for_thread_t {
    int tid;                          /* The thread ID */
    int num_threads;                  /* Number of worker threads */
    int *num_elements;                /* Number of elements in the vectors */
    float old;                        /* Old grid value */
    float new;                        /* New grid value */
    double *diff;                     /* Grid value difference */
    grid_t *grid;                     /* Grid */
    int *row;                         /* Current row */
    int offset;                       /* Starting offset for thread within the vectors */
    int chunk_size;                   /* Chunk size */
    pthread_mutex_t *mutex_for_sum;   /* Location of the lock variable protecting sum */
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

	/* Compute the reference solution using the single-threaded version. */
	printf ("\nUsing the single threaded version to solve the grid\n");
	int num_iter = compute_gold (grid_1);
	printf ("Convergence achieved after %d iterations\n", num_iter);
    /* Print key statistics for the converged values. */
	printf ("Printing statistics for the interior grid points\n");
    print_stats (grid_1);
#ifdef DEBUG
    print_grid (grid_1);
#endif
	
	/* Use pthreads to solve the equation using the jacobi method. */
	printf ("\nUsing pthreads to solve the grid using the jacobi method\n");
	num_iter = compute_using_pthreads_jacobi (grid_2, num_threads);
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
    pthread_mutex_t mutex_for_sum;              /* Lock for the shared variable sum */
    
    pthread_attr_init (&attributes);            /* Initialize the thread attributes to the default values */
    pthread_mutex_init (&mutex_for_sum, NULL);  /* Initialize the mutex */

    ARGS_FOR_THREAD **args_for_thread;
    args_for_thread = malloc (sizeof (ARGS_FOR_THREAD) * num_threads);
    int chunk_size = (int) floor ((float) (grid->dim - 1)/(float) num_threads); // Compute the chunk size

    int num_iter = 0;
	int done = 0;
    int i, j, row;
	double diff;
	// float old = 0.0, new = 0.0;
    float eps = 1e-2; /* Convergence criteria. */
    int num_elements;

    for (i = 0; i < num_threads; i++) {
        args_for_thread[i] = (ARGS_FOR_THREAD *) malloc (sizeof (ARGS_FOR_THREAD));
        args_for_thread[i]->tid = i;
        args_for_thread[i]->num_threads = num_threads;
        args_for_thread[i]->num_elements = &num_elements;
        args_for_thread[i]->diff = &diff;
        args_for_thread[i]->grid = grid;
        args_for_thread[i]->row = &row;
        args_for_thread[i]->offset = i * chunk_size;
        args_for_thread[i]->chunk_size = chunk_size;
        args_for_thread[i]->mutex_for_sum = &mutex_for_sum;
    }

    while (!done) {
        diff = 0.0;
        num_elements = 0;

        for (i = 1; i < (grid->dim - 1); i++) {
            row = i;
            for (j = 0; j < num_threads; j++)
                pthread_create (&tid[j], &attributes, jacobi, (void *) args_for_thread[j]);

            /* Wait for the workers to finish */
            for(j = 0; j < num_threads; j++)
                pthread_join (tid[j], NULL);
        }
        
        /* End of an iteration. Check for convergence. */
        diff = diff/num_elements;
        printf ("Iteration %d. DIFF: %f.\n", num_iter, diff);
        num_iter++;
			  
        if (diff < eps) 
            done = 1;
    }

    /* Free data structures */
    for(j = 0; j < num_threads; j++)
        free ((void *) args_for_thread[j]);

    return num_iter;
}

void *
jacobi (void *args)
{
    ARGS_FOR_THREAD *args_for_me = (ARGS_FOR_THREAD *) args; /* Typecast the argument to a pointer the the ARGS_FOR_THREAD structure */
    grid_t *grid = args_for_me->grid;

    if (args_for_me->tid < (args_for_me->num_threads - 1)) {
        for (int j = (args_for_me->offset + 1); j < (args_for_me->offset + args_for_me->chunk_size + 1); j++) {
            args_for_me->old = grid->element[*(args_for_me->row) * grid->dim + j];
            /* Apply the update rule. */
            args_for_me->new = 0.25 * ( grid->element[(*(args_for_me->row) - 1) * grid->dim + j] +\
                                        grid->element[(*(args_for_me->row) + 1) * grid->dim + j] +\
                                        grid->element[*(args_for_me->row) * grid->dim + (j + 1)] +\
                                        grid->element[*(args_for_me->row) * grid->dim + (j - 1)] );
            
            grid->element[*(args_for_me->row) * grid->dim + j] = args_for_me->new; /* Update the grid-point value. */
            pthread_mutex_lock(args_for_me->mutex_for_sum);
            *(args_for_me->diff) += fabs(args_for_me->new - args_for_me->old); /* Calculate the difference in values. */
            *(args_for_me->num_elements) += 1;
            pthread_mutex_unlock(args_for_me->mutex_for_sum);
        }
    }

    else { /* This takes care of the number of elements that the final thread must process */
        for (int j = (args_for_me->offset + 1); j < (grid->dim - 1); j++) {
            args_for_me->old = grid->element[*(args_for_me->row) * grid->dim + j];
            /* Apply the update rule. */
            args_for_me->new = 0.25 * ( grid->element[(*(args_for_me->row) - 1) * grid->dim + j] +\
                                        grid->element[(*(args_for_me->row) + 1) * grid->dim + j] +\
                                        grid->element[*(args_for_me->row) * grid->dim + (j + 1)] +\
                                        grid->element[*(args_for_me->row) * grid->dim + (j - 1)] );
            
            grid->element[*(args_for_me->row) * grid->dim + j] = args_for_me->new; /* Update the grid-point value. */
            pthread_mutex_lock(args_for_me->mutex_for_sum);
            *(args_for_me->diff) += fabs(args_for_me->new - args_for_me->old); /* Calculate the difference in values. */
            *(args_for_me->num_elements) += 1;
            pthread_mutex_unlock(args_for_me->mutex_for_sum);
        }
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



		


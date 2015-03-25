/*
 * main.c - This simulates a barbershop where customers arrive
 * and get seated at a waiting room until a barber is ready to
 * cut their hair. If the waiting room is full the customer is
 * turned away.
 *
 * We implement the process using a ring buffer where a chair in
 * the waiting room is represented by a slot in the ring buffer
 * and a customer is represented by an item.
 * We use semaphores to limit thread access to resources to
 * prevent race conditions.
 *
 * As soon as a customer arrives we check if there is room in 
 * the buffer using the slots semaphore and if there is room 
 * we increment the items semaphore so the barber can begin 
 * cutting their hair. We use the customer mutex semaphore
 * to ensure that the customer doesn't leave the barbershop
 * until the barber has finished cutting his hair.
 *
 * We fetch the customer from the waiting room i.e. buffer
 * and decrement the items semaphore.
 * Once the barber is done cutting the customer's hair 
 * we increment the slots semaphore i.e. freeing up a seat in 
 * the waiting room. 
 * 
 */


#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "help.h"
#include <errno.h>
#include <string.h>

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in below.
 *
 * === User information ===
 * User 1: carl13
 * SSN: 2110872939
 * User 2: johannob01
 * SSN: 0812815059
 * === End User Information ===
 ********************************************************/

void unix_error(char *msg);
int Sem_post(sem_t *semaphore);
int Sem_wait(sem_t *semaphore);
int Sem_trywait(sem_t *semaphore);
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
					void *(*start_routine) (void *), void *arg);
int Pthread_detach(pthread_t thread);

struct chairs
{
    struct customer **customer; /* Array of customers */
    /* Variables for the ring buffer */
	int max;
	int front; 
	int rear;

	/* Semaphores */
	sem_t mutex;
	sem_t slots;
	sem_t items;    
};

struct barber
{
    int room;
    struct simulator *simulator;
};

struct simulator
{
    struct chairs chairs;
    
    pthread_t *barberThread;
    struct barber **barber;
};

/*
 * barber_work - Loops constantly with treads waiting for an item to
 * be placed in the buffer. Once an item is available the thread can
 * start working on cutting the customer's hair. When the work is 
 * done a slot is freed.
 */
static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
	
    struct customer *customer = 0; 
	
    /* Main barber loop */
    while (true) {
		/* Blocks thread if no items are available */
		Sem_wait(&chairs->items);
		/* Makes sure only one thread at a time accesses the buffer */
		Sem_wait(&chairs->mutex);
		/* Removes the customer from the buffer */
		customer = chairs->customer[(++chairs->front) % (chairs->max)];
		thrlab_prepare_customer(customer, barber->room);
        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
		thrlab_dismiss_customer(customer, barber->room);
		/* Unblocks the customer thread */
		Sem_post(&customer->mutex);
		/* Unlock the buffer */
		Sem_post(&chairs->mutex);
		/* Free up a slot */
		Sem_post(&chairs->slots);		
    }
    return NULL;
}

/*
 * Initialize data structures and create waiting barber threads.
 */
static void setup(struct simulator *simulator)
{
    struct chairs *chairs = &simulator->chairs;
   
    /* Initializing variables  */
    chairs->max = thrlab_get_num_chairs();
	chairs->front = 0;
	chairs->rear = 0;
	
    /* Set up semaphores*/
	sem_init(&chairs->mutex, 0, 1);
	sem_init(&chairs->slots, 0, chairs->max);
	sem_init(&chairs->items, 0, 0);
	
    /* Create chairs*/
    chairs->customer = malloc(sizeof(struct customer *) * thrlab_get_num_chairs());
    
    /* Create barber thread data */
    simulator->barberThread = malloc(sizeof(pthread_t) * thrlab_get_num_barbers());
    simulator->barber = malloc(sizeof(struct barber*) * thrlab_get_num_barbers());

    /* Start barber threads */
    struct barber *barber;
    for (unsigned int i = 0; i < thrlab_get_num_barbers(); i++) {
		barber = calloc(sizeof(struct barber), 1);
		barber->room = i;
		barber->simulator = simulator;
		simulator->barber[i] = barber;
		Pthread_create(&simulator->barberThread[i], 0, barber_work, barber);
		Pthread_detach(simulator->barberThread[i]);
    }
}

/**
 * Free all used resources and end the barber threads.
 */
static void cleanup(struct simulator *simulator)
{
    /* Free chairs */
    free(simulator->chairs.customer);

    /* Free barber thread data */
    free(simulator->barber);
    free(simulator->barberThread);
	
	/* Destroy semaphores*/
	sem_destroy(&simulator->chairs.mutex);
	sem_destroy(&simulator->chairs.items);
	sem_destroy(&simulator->chairs.slots);	
}

/*
 * customer_arrived - When a customer arrives we check if there is
 * an available chair if not the customer is turned away. If there
 * is an empty chair we place the customer in the chair where he 
 * waits until a barber can service him.
 */
static void customer_arrived(struct customer *customer, void *arg)
{
    struct simulator *simulator = arg;
    struct chairs *chairs = &simulator->chairs;
    sem_init(&customer->mutex, 0, 0);
	
	/* Accepts a customer if there is an available chair */
    if(Sem_trywait(&chairs->slots) == 0){
		/* Makes sure only one thread at a time accesses the buffer*/
		Sem_wait(&chairs->mutex);
		thrlab_accept_customer(customer);
		/* Inserts a customer to the buffer */
		chairs->customer[(++chairs->rear)%(chairs->max)] = customer;
		/* Unlocks the buffer */
		Sem_post(&chairs->mutex);
		/* Fills up a slot */
		Sem_post(&chairs->items);
		/* Blocks the customer thread */
		Sem_wait(&customer->mutex);
		
	} else {
		/* Reject a customer if there are no available chairs */
		thrlab_reject_customer(customer);
	}
}

/*
 * Unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * Sem_post - wrapper for the sem_post function
 */
int Sem_post(sem_t *semaphore)
{
	int val;
	if((val = sem_post(semaphore)) < 0){
		unix_error("Post error");
	}
	return val;
}

/*
 * Sem_wait - wrapper for the sem_wait function
 */
int Sem_wait(sem_t *semaphore)
{
	int val;
	if((val = sem_wait(semaphore)) < 0){
		unix_error("Wait error");
	}
	return val;
}

/*
 * Sem_trywait - wrapper for the sem_trywait function
 */
int Sem_trywait(sem_t *semaphore)
{
	int val;
	if((val = sem_trywait(semaphore)) < 0){
		unix_error("Try wait error");
	}
	return val;
}

/*
 * Pthread_create - wrapper for the pthread_create function
 */
int Pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
					void *(*start_routine) (void *), void *arg)
{
	int val;
	if((val = pthread_create(thread, attr, start_routine, arg)) != 0)
	{
		unix_error("Pthread create error");
	}
	return val;
}

/*
 * Pthread_detach - wrapper for the pthread_detach function
 */
int Pthread_detach(pthread_t thread)
{
	int val;
	if((val = pthread_detach(thread)) != 0)
	{
		unix_error("Pthread detach error");
	}
	return val;
}

int main (int argc, char **argv)
{
    struct simulator simulator;

    thrlab_setup(&argc, &argv);
    setup(&simulator);

    thrlab_wait_for_customers(customer_arrived, &simulator);

    thrlab_cleanup();
    cleanup(&simulator);

    return EXIT_SUCCESS;
}

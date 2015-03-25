/*
 * main.c - This simulates a barbershop where customers arrive
 * and get seated at a waiting room until a barber is ready to
 * cut their hair. If the waiting room is full the customer is
 * turned away.
 * We implement the process using a ring buffer where a chair in
 * the waiting room is represented by a slot in the ring buffer
 * and a customer is represented by an item.
 * We use semaphores to limit thread access to resources to
 * prevent race conditions.
 * As soon as a customer arrives we check if there is room in 
 * the buffer using the slots semaphore and if there is room 
 * we increment the items semaphore so the barber can begin 
 * cutting their hair. We use the customer mutex semaphore
 * to ensure that the customer doesn't leave the barbershop
 * until the barber has finished cutting his hair.
 * When a barber receives a customer we increment the slots 
 * semaphore and decrement the items semaphore in other words 
 * freeing up a seat in the waiting room. 
 * Once the barber is done cutting the customer's hair we 
 * decrement 
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

struct chairs
{
    struct customer **customer; /* Array of customers */
    int max;
	int front;
	int rear;
	sem_t mutex;
	sem_t slots;
	sem_t items;
    /* TODO: Add more variables related to threads */
    /* Hint: Think of the consumer producer thread problem */
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

static void *barber_work(void *arg)
{
    struct barber *barber = arg;
    struct chairs *chairs = &barber->simulator->chairs;
	
    struct customer *customer = 0; 
	
    /* Main barber loop */
    while (true) {
		/* TODO: Here you must add you semaphores and locking logic */
		
		
		Sem_wait(&chairs->items);
		Sem_wait(&chairs->mutex);
		
		
		customer = chairs->customer[(++chairs->front) % (chairs->max)]; /* TODO: You must choose the customer */
		
		
		
		//Sem_wait(&customer->mutex);
		
		thrlab_prepare_customer(customer, barber->room);
        thrlab_sleep(5 * (customer->hair_length - customer->hair_goal));
		
		
        
		thrlab_dismiss_customer(customer, barber->room);
		Sem_post(&customer->mutex);
		
		Sem_post(&chairs->mutex);
		Sem_post(&chairs->slots);
		
		
    }
    return NULL;
}

/**
 * Initialize data structures and create waiting barber threads.
 */
static void setup(struct simulator *simulator)
{
    struct chairs *chairs = &simulator->chairs;
    
	
    chairs->max = thrlab_get_num_chairs();
	chairs->front = 0;
	chairs->rear = 0;
	
    /* Setup semaphores*/
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
		pthread_create(&simulator->barberThread[i], 0, barber_work, barber);
		pthread_detach(simulator->barberThread[i]);
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
	
	/* Destory semaphores*/
	
	sem_destroy(&simulator->chairs.mutex);
	sem_destroy(&simulator->chairs.items);
	sem_destroy(&simulator->chairs.slots);
	
}

/**
 * Called in a new thread each time a customer has arrived.
 */
static void customer_arrived(struct customer *customer, void *arg)
{
    struct simulator *simulator = arg;
    struct chairs *chairs = &simulator->chairs;

    sem_init(&customer->mutex, 0, 0);
	
	/* Reject if there are no available chairs */
    if(Sem_trywait(&chairs->slots) == 0){
		 /* Accept if there is an available chair */
		Sem_wait(&chairs->mutex);
		thrlab_accept_customer(customer);
		chairs->customer[(++chairs->rear)%(chairs->max)] = customer;
		Sem_post(&chairs->mutex);
		Sem_post(&chairs->items);
		Sem_wait(&customer->mutex);
		
	} else {
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

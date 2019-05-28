#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h>
#include <math.h>
#include <time.h>
#include "mythreads.h"

int main_done=0,number_of_threads,jobs_left,available_threads=0;
short int terminate = 0;
sem_t waiting_on_threads,all_available,mtx;

typedef struct job {				//job struct containing the number and the result
	int number;
	int is_prime;
}job;


typedef struct thread_info {
	thr_t thread_id;
	int thread_index;				//Thread_array index
	short int available;			//available for job,also used by main to synchronize 
	job* thread_job;				//pointer used by threads,to write the result.
	sem_t waiting_for_job;
}thread_info;


job* job_array;
thread_info* thread_array;





int prime_test(int number) {
    int i;
	
    for (i=2; i<=sqrt(number); i++) {
        if (number % i == 0)
		return 0;
    }
    return 1;
}





void worker_thread(int* id) {
	while (1) {
		
		mythreads_sem_down(&mtx);
		if (terminate ==1){ 						//Terminate flag
			mythreads_sem_up(&mtx);
			break;
		}
		available_threads++;
		
		if (available_threads==1)
			mythreads_sem_up(&waiting_on_threads);
		if (available_threads==number_of_threads && main_done == 1)
			mythreads_sem_up(&all_available);
		if(thread_array[*id].waiting_for_job.val==0)
			mythreads_sem_up(&mtx);
		mythreads_sem_down(&thread_array[*id].waiting_for_job);

		if (terminate ==1){ 						//Terminate flag
			mythreads_sem_up(&mtx);
			break;
		}
		printf("I am thread %d and my job is %d\n", *id , thread_array[*id].thread_job->number);
			
		thread_array[*id].thread_job->is_prime = prime_test(thread_array[*id].thread_job->number); //writing result
		printf("Worker no: %d finished his job.\n", *id);
		thread_array[*id].available = 1;			//Job done,waiting for new
		mythreads_sem_up(&mtx);
	}
	
	mythreads_sem_down(&mtx);
	available_threads--;
	
	if (available_threads==0);
		mythreads_sem_up(&waiting_on_threads);
	
	mythreads_sem_up(&mtx);
	return;
	
	
	
	
	
	
	
}

int main(int argc,char* argv[]) {
	int i;
	
	if (argc<3) {
		printf("Please enter as first argument the number of threads, and at least one number to check if it is prime\n");
		return 0;
	}	
	mythreads_sem_init(&waiting_on_threads,0);
	mythreads_sem_init(&all_available,0);
	mythreads_sem_init(&mtx,1);
	
	mythreads_init();

	number_of_threads = atoi(argv[1]);	//Number of threads given as arguemnt.
	clock_t begin = clock();
	
	thread_array = (thread_info*)calloc(number_of_threads, sizeof(thread_info));
	if (thread_array == NULL) {
		printf("Error allocating memory for the thread_array.\n");
		return 0;
	}
	jobs_left= argc-2 ;
	job_array = (job*)calloc(jobs_left, sizeof(job));
	if (job_array == NULL) {
		printf("Error allocating memory for the job_array.\n");
		return 0;
	}
	
	
	for(i=0; i<jobs_left; i++){			//job array init
		job_array[i].number = atoi(argv[i+2]);
		job_array[i].is_prime = -1; 			//no calculations have been made
	}

	for (i=0; i<number_of_threads; i++) {		//thread array init
		
		thread_array[i].available = 1;
		thread_array[i].thread_index = i;
		mythreads_sem_init(&thread_array[i].waiting_for_job,0);

	}
	
	for (i=0; i<number_of_threads; i++) {			//creating threads
		if(mythreads_create(&thread_array[i].thread_id,(void*)worker_thread,(void*)&thread_array[i].thread_index))
			printf("Error creating thread!!\n");
	}
	
	while(jobs_left>0){					//As long as there are jobs
		mythreads_sem_down(&mtx);
		if (available_threads == 0){
			mythreads_sem_up(&mtx);
			mythreads_sem_down(&waiting_on_threads);
		}
		
		for (i=0; i<number_of_threads; i++) {
			if(thread_array[i].available)				//Search for available thread
				break;
		}
		thread_array[i].thread_job = &job_array[jobs_left-1];		//assign job
		jobs_left--;
		thread_array[i].available = 0;								//thread no longer available
		
		available_threads--;			//global variable needs to be atomically handled

		mythreads_sem_up(&thread_array[i].waiting_for_job);
		mythreads_sem_up(&mtx);
	}
	main_done =1;
	mythreads_sem_down(&all_available);

	
	terminate = 1;							//notify workers to terminate
	for (i=0; i<number_of_threads; i++)
		mythreads_sem_up(&thread_array[i].waiting_for_job);
	
	for (i=0; i<number_of_threads; i++)
		mythreads_join(&thread_array[i].thread_id);

	
	for(i=0; i<argc-2; i++){
		if(job_array[i].is_prime){
			printf("Number %d is a prime.\n",job_array[i].number);
		}
		else{
			printf("Number %d is not a prime.\n",job_array[i].number);
		}
	}
	
	mythreads_sem_destroy(&waiting_on_threads);
	mythreads_sem_destroy(&all_available);
	mythreads_sem_destroy(&mtx);
	for (i=0; i<number_of_threads; i++)
		mythreads_sem_destroy(&thread_array[i].waiting_for_job);

	free(thread_array);
	free(job_array);
	
	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Time was: %lf\n",time_spent);
	return(0);
}




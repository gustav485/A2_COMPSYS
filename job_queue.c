#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

#include "job_queue.h"


int job_queue_init(struct job_queue *job_queue, int capacity) {
  if (capacity <= 0){
    return -1;
  }

  job_queue->jobs = malloc(capacity* sizeof(void *));
  job_queue->front = 0;
  job_queue->rear = 0;
  job_queue->count = 0;
  job_queue->capacity = capacity;
  job_queue->destroyed = 0;
  pthread_cond_init(&job_queue->not_empty, NULL);
  pthread_cond_init(&job_queue->not_full, NULL);

  if (pthread_mutex_init(&job_queue->mutex, NULL) != 0) {
    free(job_queue->jobs);
    return -1;
  }

  return 0;
}

int job_queue_destroy(struct job_queue *job_queue) {
  pthread_mutex_lock(&job_queue->mutex);
  job_queue->destroyed = 1;
  while (job_queue->count > 0) {
      pthread_cond_wait(&job_queue->not_full, &job_queue->mutex);
  } 

  pthread_cond_broadcast(&job_queue->not_empty);
  pthread_mutex_unlock(&job_queue->mutex);
  
  free(job_queue->jobs);
  pthread_mutex_destroy(&job_queue->mutex);
  pthread_cond_destroy(&job_queue->not_empty);
  pthread_cond_destroy(&job_queue->not_full);
  return 0;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  pthread_mutex_lock(&job_queue->mutex);
  while (job_queue->count == job_queue->capacity && job_queue-> destroyed != 1){
    pthread_cond_wait(&job_queue->not_full, &job_queue->mutex);
  }
  if (job_queue->destroyed) {
    pthread_mutex_unlock(&job_queue->mutex);
    return -1;
  }
  job_queue->jobs[job_queue->rear] = data;
  job_queue->rear = (job_queue->rear + 1) % job_queue->capacity;
  job_queue->count++;

  pthread_cond_signal(&job_queue->not_empty);
  pthread_mutex_unlock(&job_queue->mutex);

  return 0;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  pthread_mutex_lock(&job_queue->mutex);

  while (job_queue->count == 0 && job_queue->destroyed != 1){
    pthread_cond_wait(&job_queue->not_empty, &job_queue->mutex);
  }
  
  if (job_queue->count == 0 && job_queue->destroyed){
    pthread_mutex_unlock(&job_queue->mutex);
    return -1;
  }
  *data = job_queue->jobs[job_queue->front];
  job_queue->front = (job_queue->front + 1) % job_queue->capacity;
  job_queue->count--;

  pthread_cond_signal(&job_queue->not_full); 
  pthread_mutex_unlock(&job_queue->mutex);
  return 0;
}

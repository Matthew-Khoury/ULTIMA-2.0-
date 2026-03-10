/* Begining was taken from Lab_10-Semaphores writted by Dr. Hakimzadeh*/
#include <iostream>
#inlcude <pthread.h>
#include <unistd.h>
#include <queue.h>
#include "Semaphores.cpp"

void* worker(void* arg);

Semaphore sem(1);
//needs a print function 

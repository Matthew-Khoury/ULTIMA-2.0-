#include <iostream>
#inlcude <pthread.h>
#include <unistd.h>
#include <queue.h>

class Semaphore{
    private:
        int value;  // this will be the id number ** Check with the group*
        pthread_mutex_t lock; // the on/off lock 
        pthread cond_t cond;
        Queue<int> waitQueue;

    public:
        Semaphore(int initalValue) : value(initalValue){
            pthread_mutex_init(&LOCK, nullptr);
            pthread_cond_init(&cond, nullptr);

        }
        
        ~Semaphore(){
            pthread_mutex_destroy(&lock);
            pthread_cond_destory(&cond);

        }
//Implement the up, down, and wait functions needed for semaphores
        void Down(int thread_id){

            pthread_mutex_lock(&lock);
            std:: cout << "\tThread = " << thread_id<< "aqcuiring mutex lock" << std :: endl;

            if(value >=1){
                value --;

            }
            else{
                pthread_t self = pthread_self();
                std:: cout <<"\tThread = " << thread_id<< " is being placed on queue (Internal Thread_No = )"<<self<<")'std::endl;
                
                //waitQueue.En_Q(self);

                waitQueue.En_Q(thread_id)
                waitQueue.Print();

                do {
                    std:: cout<< "\tThread = " << thread_id << "waiting to be released from the queue" << std::endl;
                    pthread_cond_wait)&cond, &lock);
                                        
                } while (value < 0);

                //std::cout << self << " just got released from queue " << std::endl;
                std:: cout <<" \tThread = " << thread_id << " just got released from the queue and re-aquired mutex lock" << std :: endl;
            }
            
            
        
             
            //Above is the critical region is located

            pthead_mutex_unlock(&lock);
            }


         void Up (int thread_id){

            pthread_mutex_lock(&lock);
            std:: cout << "\tThread = " << thread_id<< "aqcuiring mutex lock" << std :: endl;

            if (value <= 1){
                int id;
                if(!watQueue.isEmpty()){
                std::cout<<"\tBefore ReleasingThread from Queue\n:";

                waitQueue.Print();
                id = waitQueue.De_Q();
                std:: cout << "\tAfter Releasing Thread " << id << " from Queue\n";
                waitQueue.Print();
                }
            std:: cout <<"\tSignal Blocked Thread " << id << " to be released\n";

            pthread_cond_signal(&cond);
            }
            else
                value ++;
                
            pthread_mutex_unlock(&lock);

            }
              
            
    int main(){
    const int THREAD_COUNT =3;
    pthread_t thread[THREAD_COUNT];

    std :: cout << "Creating " << THREAD_COUNT << "Child Threads\n";
    for (long i = 0 ; i < THREAD_COUNT; i++){
        pthread_create(&threads[i], nullptr, worker, (void*)i);
        }
        std ::cout << "Parent waiting for Child Threads to end.\n";
        for(int i =0; i< THRESD_COUNT, i++){
        pthread_join*threads[i], nullptr);
        }
        return 0;
        }


        
                

                


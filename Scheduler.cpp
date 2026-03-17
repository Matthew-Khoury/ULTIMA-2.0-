#include "Queue.h"

const string READY = "Ready";
const string RUNNING = "Running";
const string BLOCKED = "Blocked"; 
const string DEAD = "Dead";

const int MAX_TASKS = 3; //how do we determine the number of tasks allowed?

struct tcb {
    int task_id;
    string state;
    clock_t start_time;
    tcb *next; //this line creates a pointer to the next node in the linked list

};

class scheduler{
    // the orginal line from Lab 12: int current_task; 
    //the replacement lines to implement the linked list
    tcb* current;
    tcb* tail;
    long current_quantum;
    int next_available_task_id;
    tcb task_table[MAX_TASKS]; // this is an Array

public:
scheduler(){
    // current_task= -1;
    current = nullptr;
    tail = nullptr;
    next_available_task_id =0;//start the system start at 0
    current_quantum=300; 
}
~scheduler(){
    cout<<"Existing Scheduler...."<<endl;
}

void set_quantum(long quantum){
    current_quantum = quantum;
}
long get_quantum(){
    return(current_quantum);
}
void set_state(int the_taskid, string the_state){
    task_table[the_taskid].state = the_state;
}
string get_state(int the_taskid){
    return(task_table[the_taskid].state);
}
int get_task_id(){
    return (current!= nullptr) ? current-> task_id: -1; // will return the current task
}

int create_task(){
    if(next_available_task_id<MAX_TASKS){
        cout<< "Creating task # " << next_available_task_id<<endl;
        task_table[next_available_task_id].task_id = next_available_task_id << endl;
        task_table[next_available_task_id].state = READY;
        task_table[next_available_task_id].next = NULL; 

        next_available_task_id++;
        return(next_available_task_id -1);
    }
    else{
        cout << "Create_task() FAILED: Available tasks exceeded. MAX_TASKS = " << MAX_TASKS << endl;
        return(-1); //return error
    }
}
void start(){
    cout<< "................." << endl;
    cout<< ".................SCHEDULING STARTED" << endl;
    cout<< ".................\n" << endl;
    //task_table[0].start_time = clock();
    //task_table[0].state = RUNNING;
    current = tail->next; //the head node for the linked list 
    current ->start_time = clock();
    current-> state = RUNNING;
    set_quantum(1000/MAX_TASKS); //setting the fixed amount of time the CPU will allocate for the process

    sleep(1);

}
void yield (){

    int counter = 0;
    cout << "Current Task # "<< current->task_id << " is trying to Yield" << endl;
    
    clock_t elaspsed_time = clock() - current->start_time;
    cout << "Task: " << current->task_id << ", Elapsed time: "<< elaspsed_time << endl;

    if(elaspsed_time >= current_quantum){
        cout << "Yielding...(Switching from task #" << current->task_id << " to next ready task)" <<endl;

        //if running, we should change it to ready since the quantime itme has run out
        if(current->state == RUNNING)
            current->state = READY;
       // current_task = (current_task +1) % MAX_TASKS; from lab 12 replaced by the following line
       current = current->next;

        while(current->state != READY && counter < MAX_TASKS-1){ //we are subtracting one because we can not surpase the max tasks
            //current_task = (current_task + 1) % MAX_TASKS;
            current = current->next;
            counter ++;

        }
        if (counter < MAX_TASKS -1 && current->state == READY){
            current->start_time = clock();
            current->state = RUNNING;
            cout <<"Started Running task # "<<current->task_id <<endl;
        }else{
        cout << "POSSIBLE DEAD LOCK"<< endl;
    }
    
    }
    else
    cout<<"NO YIELD!  (task: " << current->task_id << " Still have some quantum left)" << endl;
}

void dump(){
    cout << "------------------------------PROCESS TABLE------------------------------------"<<endl;
    cout <<"Quantum = " << current_quantum<< endl;

    cout << "Task ID\t Elapsed Time\tState"<<endl;
    for(int i = 0; i<next_available_task_id; i++){
        clock_t elapsed_time = clock() - task_table[i].start_time;
        printf("%6d\t%8d\t%s",task_table[i].task_id, elapsed_time, task_table[i].state.c_str() );

        if(i == current->task_id)// ** ask the group to 
            cout <<" <---CURRENT PROCESS";
        cout << endl; 
    }
    cout<<"----------------------------------------------------------------------------------\n"<<endl;
}


};

int main(){
    scheduler swapper;
    int t_id;

    for(int i = 0; i< 4; i++)
    t_id = swapper.create_task();

    swapper.dump();
    swapper.start();
    swapper.dump();

    for(int i = 0; i<3; i++){
        waste_time(3);
        swapper.yield();
        swapper.dump();
    }
}

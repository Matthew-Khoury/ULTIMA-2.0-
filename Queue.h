//Primary Author: Matthew Khoury

#ifndef QUEUE_H
#define QUEUE_H

#include <iostream>
#include <sstream>

// ------------------ DECLARATION ------------------
template <class TYPE>
class Queue {
public:
    Queue();
    ~Queue();

    void En_Q(TYPE value);
    TYPE De_Q();
    bool isEmpty();
    void Print();
    std::string Get_Q_String();

private:
    TYPE* data;     // dynamic array
    int front;      // first element index
    int back;       // last element index
    int capacity;   // size of the array (queue)
    int count;      // number of total elements in the array (queue)
};

// ------------------ IMPLEMENTATION ------------------

// Queue constructor
template <class TYPE>
Queue<TYPE>::Queue() {
    capacity = 10;              // TODO: discuss the initial capacity
    data = new TYPE[capacity];
    front = 0;
    back = 0;
    count = 0;
}

// Queue deconstructor
template <class TYPE>
Queue<TYPE>::~Queue() {
    delete[] data;
}

template <class TYPE>
void Queue<TYPE>::En_Q(TYPE value) {
    if (count == capacity) {
        return;  // TODO: Discuss whether to implement resizing logic for the capacity of the queue
    }
    data[back] = value;             // assign the back of the queue as enqueued value
    back = (back + 1) % capacity;   // move the back index forward
    count++;                        // increment element count by 1
}

template <class TYPE>
TYPE Queue<TYPE>::De_Q() {
    if (isEmpty()) {
        throw std::runtime_error("Queue is empty");
    }
    TYPE value = data[front];        // get the first element in the queue
    front = (front + 1) % capacity;  // move the front index forward
    count--;                         // decrement element count by 1
    return value;                    // return the removed element
}

template <class TYPE>
bool Queue<TYPE>::isEmpty() {
    return (count == 0);
}

template <class TYPE>
void Queue<TYPE>::Print() {
    for (int i = 0; i < count; i++) {      // loop through array
        std::cout << data[(front + i) % capacity] << " ";
    }
    std::cout << std::endl;        // print out the elements in order(front -> back):capacity
}

template <class TYPE>
std::string Queue<TYPE>::Get_Q_String() {
    std::stringstream ss;
    for (int i = 0; i < count; i++) {      // loop through array
        ss << data[(front + i) % capacity] << " ";
    }
    return ss.str();     // return contents of the array as a string
}

#endif

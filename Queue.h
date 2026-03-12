#include <iostream>
#include <sstream>

using namespace std;

// This Queue is based off of the template from Lab 10
#define TRUE 1
#define FALSE 0

// ------------------ DECLARATION ------------------
template <class TYPE>
class Queue {
public:
	Queue();
	~Queue();

	void En_Q(TYPE& value);
	TYPE De_Q();
	int isEmpty();
	void Print();
	string Get_Q_String();

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
void Queue<TYPE>::En_Q(TYPE& value) {
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
		throw runtime_error("Queue is empty");
	}
	TYPE value = data[front];        // get the first element in the queue
	front = (front + 1) % capacity;  // move the front index forward
	count--;                         // decrement element count by 1
	return value;                    // return the removed element
}

template <class TYPE>
int Queue<TYPE>::isEmpty() {
	return (count == 0) ? TRUE : FALSE;   // return 1 (TRUE) or 0 (FALSE)
}

template <class TYPE>
void Queue<TYPE>::Print() {
	for (int i = 0; i < count; i++) {      // loop through array
		cout << data[(front + i) % capacity] << " ";
	}
	cout << endl;        // print out the elements in order(front -> back):capacity
}

template <class TYPE>
string Queue<TYPE>::Get_Q_String() {
	stringstream ss;
	for (int i = 0; i < count; i++) {      // loop through array
		ss << data[(front + i) % capacity] << " ";
	}
	return ss.str();     // return contents of the array as a string
}

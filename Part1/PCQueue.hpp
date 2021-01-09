#ifndef _QUEUEL_H
#define _QUEUEL_H
#include "Headers.hpp"
#include "Semaphore.hpp"
// Single Producer - Multiple Consumer queue
template <typename T>class PCQueue
{

public:
	// Blocks while queue is empty. When queue holds items, allows for a single
	// thread to enter and remove an item from the front of the queue and return it. 
	// Assumes multiple consumers.
	T pop(); 

	// Allows for producer to enter with *minimal delay* and push items to back of the queue.
	// Hint for *minimal delay* - Allow the consumers to delay the producer as little as possible.  
	// Assumes single producer 
	void push(const T& item); 

	PCQueue();

	~PCQueue();

private:
	// Add your class memebers here
    queue<T> items_queue;
    Semaphore sem;
    pthread_mutex_t mutex;
    int producers;
};
// Recommendation: Use the implementation of the std::queue for this exercise

template <typename T>
void PCQueue<T>::push(const T &item) {
    producers++;
    pthread_mutex_lock(&mutex);
    items_queue.push(item);
    producers--;
    pthread_mutex_unlock(&mutex);
    sem.up();
}

template <typename T>
T PCQueue<T>::pop() {
    while(producers){
        sched_yield();
        // gives up cpu in case there is a producer inside
        // should effectively give some priority to producers
    }
    sem.down();
    pthread_mutex_lock(&mutex);

    T item = items_queue.front();
    items_queue.pop();

    pthread_mutex_unlock(&mutex);

    return item;
}

template <typename T>
PCQueue<T>::PCQueue(): items_queue(), sem(), producers(0){
    pthread_mutex_init(&mutex, nullptr);
}

template <typename T>
PCQueue<T>::~PCQueue() {
    pthread_mutex_destroy(&mutex);
}
#endif

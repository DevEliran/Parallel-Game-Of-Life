#ifndef _QUEUEL_H
#define _QUEUEL_H
#include <cassert>
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
	queue<T> items_queue;
	pthread_cond_t cond;
    pthread_mutex_t mutex;
    int queue_size;
};
// Recommendation: Use the implementation of the std::queue for this exercise

template <typename T>
void PCQueue<T>::push(const T &item) {
    pthread_mutex_lock(&mutex);
    items_queue.push(item);
    queue_size++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

template <typename T>
T PCQueue<T>::pop() {
    pthread_mutex_lock(&mutex);
    while(queue_size == 0){
		pthread_cond_wait(&cond, &mutex);
	}
	T item = items_queue.front();
	items_queue.pop();
	queue_size--;
	pthread_mutex_unlock(&mutex);
	return item;
}

template <typename T>
PCQueue<T>::PCQueue(): items_queue(), queue_size(0){
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
}

template <typename T>
PCQueue<T>::~PCQueue() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

#endif

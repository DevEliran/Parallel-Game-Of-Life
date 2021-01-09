#include "Headers.hpp"
#include "Semaphore.hpp"

Semaphore::Semaphore(): total_locks(0), available_locks(0){
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
}

Semaphore::Semaphore(unsigned val): total_locks(val), available_locks(val) {
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
}

void Semaphore::up() {
    pthread_mutex_lock(&m);
    available_locks++;
    pthread_cond_signal(&c);
    pthread_mutex_unlock(&m);
}

void Semaphore::down() {
    pthread_mutex_lock(&m);
    while(available_locks == 0){
        pthread_cond_wait(&c, &m);
    }
    available_locks--;
    pthread_mutex_unlock(&m);
}

Semaphore::~Semaphore() {
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&c);
}

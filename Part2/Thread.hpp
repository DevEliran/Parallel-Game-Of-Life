#ifndef __THREAD_H
#define __THREAD_H

#include "../Part1/Headers.hpp"
#include "Game.hpp"
#include "Job.h"
class Thread
{
public:
	Thread(uint thread_id) 
	{
		// Only places thread_id
		m_thread_id = thread_id;
	} 
	virtual ~Thread() {} // Does nothing

	/** Returns true if the thread was successfully started, false if there was an error starting the thread */
	// Creates the internal thread via pthread_create 
	bool start()
	{
        return (pthread_create(&m_thread, nullptr, entry_func, this) == 0);
	}

	/** Will not return until the internal thread has exited. */
	void join()
	{
        pthread_join(m_thread, nullptr);
	}

	/** Returns the thread_id **/
	uint thread_id()
	{
		return m_thread_id;
	}
protected:
	// Implement this method in your subclass with the code you want your thread to run. 
	virtual void thread_workload() = 0;
	uint m_thread_id; // A number from 0 -> Number of threads initialized, providing a simple numbering for you to use

private:
	static void * entry_func(void * thread) { ((Thread *)thread)->thread_workload(); return NULL; }
	pthread_t m_thread;
};


class GameThread: public Thread{
public:
    Game* game;
    GameThread(uint id, Game* g): Thread(id), game(g){}
    virtual ~GameThread() = default;
    
    void thread_workload() override {
        while(game->jobs_left > 0){
            pthread_mutex_lock(&game->job_mtx);
            Job curr_job = game->jobs_queue->pop(); //blocks if empty (pcq)
            game->jobs_left--;
            pthread_mutex_unlock(&game->job_mtx);
            curr_job.execute_job();
        }
    }
};
#endif

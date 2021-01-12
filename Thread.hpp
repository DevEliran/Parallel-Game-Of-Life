#ifndef __THREAD_H
#define __THREAD_H
#include "Headers.hpp"
#include "PCQueue.hpp"
#include "Job.h"

class Thread
{
public:
	Thread(uint thread_id, PCQueue<Job*>* jobs_q, vector<double>* hist,
	        pthread_mutex_t* m, vector<vector<uint>>* curr,
	        vector<vector<uint>>* next, Semaphore* completed){
	    m_thread_id = thread_id;
	    jobs_queue = jobs_q;
	    completed_jobs = completed;
	    m_tile_hist = hist;
	    mutex = m;
	    game_matrix_curr = curr;
	    game_matrix_next = next;
	}

	virtual ~Thread() {} // Does nothing 

	/** Returns true if the thread was successfully started, false if there was an error starting the thread */
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
        return this->m_thread_id;
	}
protected:
	/** Implement this method in your subclass with the code you want your thread to run. */
	virtual void thread_workload() = 0;
	uint m_thread_id; // A number from 0 -> Number of threads initialized, providing a simple numbering for you to use
    PCQueue<Job*>* jobs_queue;
    //int* completed_jobs;
    vector<double>* m_tile_hist;
    pthread_mutex_t* mutex;
    vector<vector<uint>>* game_matrix_curr;
    vector<vector<uint>>* game_matrix_next;
    Semaphore* completed_jobs;

private:
	static void * entry_func(void * thread) { ((Thread *)thread)->thread_workload(); return NULL; }
	pthread_t m_thread;
};


class GameThread: public Thread{
public:
    GameThread(uint thread_id, PCQueue<Job*>* jobs_queue, vector<double>* hist,
				pthread_mutex_t* m, vector<vector<uint>>* curr,
				vector<vector<uint>>* next, Semaphore* completed):
               Thread(thread_id, jobs_queue, hist, m, curr, next, completed){}
    ~GameThread() = default;

    void thread_workload() {
        while (true) {
            Job* job = jobs_queue->pop();
            int range_start = get<0>(job->thread_range_coverage);
            int range_end = get<1>(job->thread_range_coverage);
            
			auto start = std::chrono::system_clock::now();
			
            if (!job->phase) {//Starting phase 1
                for (int i = range_start; i < range_end; ++i) {
					//Bounds for neighbors searching
					int low_bound = i-1 >= 0 ? i-1: 0;
                    int high_bound = i+1 < (int)game_matrix_curr->size() ? i+1 : (int)game_matrix_curr->size()-1;

                    for (int j = 0; j < (int)(*game_matrix_curr)[i].size(); ++j) {
						//Bounds for neighbors searching
						int left_bound = j-1 >= 0 ? j-1: 0;
                        int right_bound = (int)(*game_matrix_curr)[i].size()-1 < j+1 ? (int)(*game_matrix_curr)[i].size()-1 : j+1;

                        int alive_neighbors = 0;
                        vector<int> specie_histogram(8, 0);// counting the appearances of each specie in the neighborhood

                        for (int k = low_bound; k <= high_bound; ++k) {
                            for (int l = left_bound; l <= right_bound; ++l) {
                                if (k == i && l == j) {
                                    continue;
                                } else if ((*game_matrix_curr)[k][l] != 0) {
									alive_neighbors++;
                                    if (alive_neighbors <= 3) {
                                        specie_histogram[((*game_matrix_curr)[k][l])]++;
                                    }
                                }
                            }
                        }

                        if ((*game_matrix_curr)[i][j] == 0 && alive_neighbors == 3) {
                            int dominant = 0;
                            int max = 0;
                            for (int k = 0; k < 8; ++k) {
                                if (specie_histogram[k] * k > max) {
                                    max = specie_histogram[k] * k;
                                    dominant = k;
                                }
                                (*game_matrix_next)[i][j] = dominant;
                            }
                        } else if ((*game_matrix_curr)[i][j] > 0 && alive_neighbors != 2 && alive_neighbors != 3) {
                            (*game_matrix_next)[i][j] = 0;
                        } else {
                            (*game_matrix_next)[i][j] = (*game_matrix_curr)[i][j];
                        }
                    }
                }
            }
            else {//Starting phase 2
                for (int i = range_start; i < range_end; ++i) {
					//Bounds for neighbors searching
                    int low_bound = i-1 >= 0 ? i-1: 0;
                    int high_bound = i+1 < (int)game_matrix_curr->size() ? i+1 : (int)game_matrix_curr->size()-1;

                    for (int j = 0; j < (int) (*game_matrix_next)[i].size(); ++j) {
						//If a cell is dead in phase 2 he will remain dead
                        if ((*game_matrix_next)[i][j] == 0) {
                            (*game_matrix_curr)[i][j] = 0;
                            continue;
                        }
						//Bounds for neighbors searching
                        int left_bound = j-1 >= 0 ? j-1: 0;
                        int right_bound = (int)(*game_matrix_curr)[i].size()-1 < j+1 ? (int)(*game_matrix_curr)[i].size()-1 : j+1;

                        double alive_neighbors = 0;
                        double sum = 0;
                        /*-----------------------------------------------
                         * Searching for dominant specie in neighborhood
                         ------------------------------------------------*/
                        for (int k = low_bound; k <= high_bound; ++k) {
                            for (int l = left_bound; l <= right_bound; ++l) {
                                if ((*game_matrix_next)[k][l] > 0) {
                                    alive_neighbors++;
                                    sum += (*game_matrix_next)[k][l];
                                }
                            }
                        }
                        (*game_matrix_curr)[i][j] = round(sum / alive_neighbors);
                    }
                }
            }
			auto end = std::chrono::system_clock::now();
            pthread_mutex_lock(mutex);
            m_tile_hist->push_back((double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
            pthread_mutex_unlock(mutex);
			bool exit = job->is_last_gen_phase_two;
			delete job;
			completed_jobs->up();
		
            if(exit) {
                return;
            }
        }
    }
};
#endif

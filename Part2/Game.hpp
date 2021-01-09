#ifndef __GAMERUN_H
#define __GAMERUN_H

#include "../Part1/Headers.hpp"
#include "../Part1/PCQueue.hpp"
#include "Thread.hpp"

/*--------------------------------------------------------------------------------
								  Species colors
--------------------------------------------------------------------------------*/
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black - 7 */
#define RED     "\033[31m"      /* Red - 1*/
#define GREEN   "\033[32m"      /* Green - 2*/
#define YELLOW  "\033[33m"      /* Yellow - 3*/
#define BLUE    "\033[34m"      /* Blue - 4*/
#define MAGENTA "\033[35m"      /* Magenta - 5*/
#define CYAN    "\033[36m"      /* Cyan - 6*/


/*--------------------------------------------------------------------------------
								  Auxiliary Structures
--------------------------------------------------------------------------------*/
struct game_params {
	// All here are derived from ARGV, the program's input parameters. 
	uint n_gen;
	uint n_thread;
	string filename;
	bool interactive_on; 
	bool print_on; 
};
/*--------------------------------------------------------------------------------
									Class Declaration
--------------------------------------------------------------------------------*/
class Game {
public:
/*--------------------------------------------------------------------------------
									Job Class
--------------------------------------------------------------------------------*/
	class Job{
	public:
		Game* game;
		tuple<int, int> thread_range_coverage;
	//    uint matrix_height;
	//    uint matrix_width;
	//    bool phase;
	//    bool is_last_gen_phase_two;

		Job(Game* g, tuple<int, int> range): game(g), thread_range_coverage(range){}
		~Job() = default;
		void execute_job(){
			auto start = std::chrono::system_clock::now();
			if(!game->phase){
				game->execute_phase_one(this->thread_range_coverage);
			} else if(game->phase){
				game->execute_phase_two(this->thread_range_coverage);
			}
			pthread_mutex_lock(&game->mtx);
			game->completed_jobs++;
			if(game->completed_jobs == game->m_thread_num){
				pthread_cond_signal(&game->cnd);
			}
			auto end = std::chrono::system_clock::now();
			game->m_tile_hist.push_back((double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
			pthread_mutex_unlock(&game->mtx);
		}
	};
/*--------------------------------------------------------------------------------
									Game Class
--------------------------------------------------------------------------------*/
	Game(game_params);
	~Game();
	void run(); // Runs the game
	const vector<double> gen_hist() const; // Returns the generation timing histogram  
	const vector<double> tile_hist() const; // Returns the tile timing histogram
	uint thread_num() const; //Returns the effective number of running threads = min(thread_num, field_height)


protected: // All members here are protected, instead of private for testing purposes

	// See Game.cpp for details on these three functions
	void _init_game(); 
	void _step(uint curr_gen); 
	void _destroy_game(); 
	inline void print_board(const char* header);

	uint m_gen_num; 			 // The number of generations to run
	uint m_thread_num; 			 // Effective number of threads = min(thread_num, field_height)
	vector<double> m_tile_hist; 	 // Shared Timing history for tiles: First (2 * m_gen_num) cells are the calculation durations for tiles in generation 1 and so on. 
							   	 // Note: In your implementation, all m_thread_num threads must write to this structure. 
	vector<double> m_gen_hist;  	 // Timing history for generations: x=m_gen_hist[t] iff generation t was calculated in x microseconds
	vector<Thread*> m_threadpool; // A storage container for your threads. This acts as the threadpool. 

	bool interactive_on; // Controls interactive mode - that means, prints the board as an animation instead of a simple dump to STDOUT 
	bool print_on; // Allows the printing of the board. Turn this off when you are checking performance (Dry 3, last question)
	
	// TODO: Add in your variables and synchronization primitives
    uint non_effective_thread_num;
    string filename;
    int_mat* game_matrix_curr;
    int_mat* game_matrix_next;
    uint matrix_height;
    uint matrix_width;

    int completed_jobs;

    pthread_mutex_t mtx;
    pthread_cond_t cnd;
    

    void initialize_game_matrix();
    void fill_jobs_queue();

    void execute_phase_one(tuple<int, int> range);
    void execute_phase_two(tuple<int, int> range);
    bool is_in_bounds(int r, int c);
    
public:
	int jobs_left;
    bool phase;
    PCQueue<Job>* jobs_queue;
    pthread_mutex_t job_mtx;
};


class GameThread: public Thread{
public:
    Game* game;
    GameThread(uint id, Game* g): Thread(id), game(g){}
    virtual ~GameThread() = default;
    
    void thread_workload() override {
        while(game->jobs_left > 0){
            pthread_mutex_lock(&game->job_mtx);
            Game::Job curr_job = game->jobs_queue->pop(); //blocks if empty (pcq)
            game->jobs_left--;
            pthread_mutex_unlock(&game->job_mtx);
            curr_job.execute_job();
        }
    }
};
#endif

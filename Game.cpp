#include "Game.hpp"
#include "utils.hpp"

static const char *colors[7] = {BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN};
/*--------------------------------------------------------------------------------

--------------------------------------------------------------------------------*/
void Game::run() {

	_init_game(); // Starts the threads and all other variables you need
	print_board("Initial Board");
	for (uint i = 0; i < m_gen_num; ++i) {
		auto gen_start = std::chrono::system_clock::now();
		_step(i); // Iterates a single generation 
		auto gen_end = std::chrono::system_clock::now();
		m_gen_hist.push_back((float)std::chrono::duration_cast<std::chrono::microseconds>(gen_end - gen_start).count());
		print_board(nullptr);
	} // generation loop
	print_board("Final Board");
	_destroy_game();
}

void Game::_init_game() {
    initialize_game_matrix();
    m_thread_num = non_effective_thread_num > matrix_height ? matrix_height: non_effective_thread_num;
    //jobs_queue = new PCQueue<Job>;
    //completed_jobs = 0;
    pthread_mutex_init(&mtx, nullptr);
    
    for(uint i = 0; i < m_thread_num; i++){
		GameThread* gh = new GameThread(i, &jobs_queue, &m_tile_hist,
			&mtx, game_matrix_curr, game_matrix_next, &completed_jobs); 
        m_threadpool.push_back(gh);
        gh->start();
    }
    //cout << matrix_height << "," << matrix_width << endl;
}

void Game::_step(uint curr_gen) {
    //completed_jobs = 0;
    fill_jobs_queue(0, false); // push jobs for phase one
	
    for(int i = 0; i < m_thread_num; i++){
		completed_jobs.down();
	}// waiting for phase one to complete

    //completed_jobs = 0;
    bool end = false;
    if(curr_gen == m_gen_num - 1){
        end = true;
    }
    
    fill_jobs_queue(1, end); // push jobs for phase two
    for(int i = 0; i < m_thread_num; i++){
		completed_jobs.down();
	}// waiting for phase two to complete
    
    /* Instead of swapping between matrices for two times at each _step call
     * I implemented thread_workload to work on the current board in phase 1
     * and updated the next board.
     * While on phase 2, I work on the next board and update the current board.
     * That way I save up to 2*num_of_generations swaps.
     */
}

void Game::_destroy_game(){
    for (uint i = 0; i < m_thread_num; ++i) {
        m_threadpool[i]->join();
    }

    for(auto &thread: this->m_threadpool){
        delete thread;
    }

    delete game_matrix_curr;
    delete game_matrix_next;
    pthread_mutex_destroy(&mtx);
}

/*--------------------------------------------------------------------------------

--------------------------------------------------------------------------------*/
inline void Game::print_board(const char* header) {

	if(print_on){ 

		// Clear the screen, to create a running animation 
		if(interactive_on)
			system("clear");

		// Print small header if needed
		if (header != NULL)
			cout << "<------------" << header << "------------>" << endl;

        cout << u8"╔" << string(u8"═") * matrix_width << u8"╗" << endl;
        for (uint i = 0; i < matrix_height; ++i) {
            cout << u8"║";
            for (uint j = 0; j < matrix_width; ++j) {
                if ((*game_matrix_curr)[i][j] > 0)
                    cout << colors[(*game_matrix_curr)[i][j] % 7] << u8"█" << RESET;
                else
                    cout << u8"░";
            }
            cout << u8"║" << endl;
        }
        cout << u8"╚" << string(u8"═") * matrix_width << u8"╝" << endl;

		// Display for GEN_SLEEP_USEC micro-seconds on screen 
		if(interactive_on)
			usleep(GEN_SLEEP_USEC);
	}

}


Game::Game(game_params params): m_gen_num(params.n_gen), m_thread_num(params.n_thread),
                interactive_on(params.interactive_on),
                print_on(params.print_on), non_effective_thread_num(params.n_thread),
                filename(params.filename), completed_jobs(){
    game_matrix_curr = new vector<vector<uint>>;
    game_matrix_next = new vector<vector<uint>>;
}

Game::~Game() {}

uint Game::thread_num() const {
    return this->m_thread_num;
}

const vector<double> Game::gen_hist() const {
    return m_gen_hist;
}

const vector<double> Game::tile_hist() const {
    return m_tile_hist;
}

void Game::initialize_game_matrix() {
    vector<string> lines = utils::read_lines(filename);
    for(auto& it: lines){
        vector<string> rows = utils::split(it, DEF_MAT_DELIMITER);
        vector<uint> elements = vector<uint>();
        for(uint i = 0; i < rows.size(); i++){
            elements.push_back(std::stoi(rows[i]));
        }
        game_matrix_curr->push_back(elements);
        game_matrix_next->push_back(elements);
        matrix_width = elements.size();
    }
    matrix_height = game_matrix_curr->size();
    //matrix_width = game_matrix_curr[0].size();
}


void Game::fill_jobs_queue(bool phase, bool end) {
    assert(m_thread_num != 0);
    int rows_per_thread = matrix_height / m_thread_num;
    int remainder = matrix_height % m_thread_num;

    for(uint i = 0; i < m_thread_num - 1; i++){
        tuple<int, int> range{rows_per_thread * i, rows_per_thread * (i+1)};
        Job* new_job = new Job(range, matrix_height, matrix_width, phase, end);
        jobs_queue.push(new_job);
    }
	tuple<int, int> remainder_range{rows_per_thread * (m_thread_num-1),
									(rows_per_thread * (m_thread_num)) + remainder};
	jobs_queue.push(new Job(remainder_range, matrix_height, matrix_width, phase, end));
}



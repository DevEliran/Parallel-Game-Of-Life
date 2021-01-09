#include "Game.hpp"
#include "utils.hpp"
#include "Thread.hpp"

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
		m_gen_hist.push_back((double)std::chrono::duration_cast<std::chrono::microseconds>(gen_end - gen_start).count());
		print_board(nullptr);
	} // generation loop
	print_board("Final Board");
	_destroy_game();
}

void Game::_init_game() {

	// Create game fields - Consider using utils:read_file, utils::split
	// Create & Start threads
	// Testing of your implementation will presume all threads are started here
    initialize_game_matrix();
    m_thread_num = non_effective_thread_num > matrix_height ? matrix_height: non_effective_thread_num;
    jobs_queue = new PCQueue<Job>;
    completed_jobs = 0;
    pthread_mutex_init(&mtx, nullptr);
    pthread_cond_init(&cnd, nullptr);
    pthread_mutex_init(&job_mtx, nullptr);

    for(uint i = 0; i < m_thread_num; i++){
        m_threadpool.push_back(new GameThread(i, this));
    }
    for(auto& thread: this->m_threadpool){
        thread->start();
    }

    jobs_left = 2 * m_thread_num * m_gen_num;
}

void Game::_step(uint curr_gen) {
	// Push jobs to queue
	// Wait for the workers to finish calculating 
	// Swap pointers between current and next field 
	// NOTE: Threads must not be started here - doing so will lead to a heavy penalty in your grade
	completed_jobs = 0;
	fill_jobs_queue();

	pthread_mutex_lock(&mtx);
	while(completed_jobs < m_thread_num){
	    pthread_cond_wait(&cnd, &mtx);
	}

	phase = 1;
	completed_jobs = 0;
	pthread_mutex_unlock(&mtx);

	game_matrix_curr = game_matrix_next; //There is no purpose "fully" swap I guess

	fill_jobs_queue();

    pthread_mutex_lock(&mtx);
    while(completed_jobs < m_thread_num){
        pthread_cond_wait(&cnd, &mtx);
    }

    phase = 0;
    completed_jobs = 0;
    pthread_mutex_unlock(&mtx);

    game_matrix_curr = game_matrix_next;
}

void Game::_destroy_game(){
	// Destroys board and frees all threads and resources 
	// Not implemented in the Game's destructor for testing purposes. 
	// All threads must be joined here
	for (uint i = 0; i < m_thread_num; ++i) {
        m_threadpool[i]->join();
    }
	pthread_mutex_destroy(&mtx);
	pthread_cond_destroy(&cnd);
	pthread_mutex_destroy(&job_mtx);
	delete game_matrix_curr;
	delete game_matrix_next;
	delete jobs_queue;
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
    }
    matrix_height = game_matrix_curr->size();
    matrix_width = game_matrix_curr[0].size();
}

void Game::fill_jobs_queue() {
    assert(m_thread_num != 0);
    int rows_per_thread = matrix_height / m_thread_num;
    int remainder = matrix_height % m_thread_num;

    for(uint i = 0; i < m_thread_num - 1; i++){
        tuple<int, int> range{rows_per_thread * i, rows_per_thread * (i+1)};
        Job new_job = Job(this, range);
        jobs_queue->push(new_job);
    }
    if(remainder > 0){
        tuple<int, int> remainder_range{rows_per_thread * (m_thread_num-1),
                                        (rows_per_thread * (m_thread_num)) + remainder};
        jobs_queue->push(Job(this, remainder_range));
    } else if (remainder == 0){
        tuple<int, int> range{rows_per_thread * m_thread_num - 1, rows_per_thread * m_thread_num};
        jobs_queue->push(Job(this, range));
    }
}

Game::Game(game_params params): m_gen_num(params.n_gen), m_thread_num(params.n_thread),
                                interactive_on(params.interactive_on),
                                print_on(params.print_on), non_effective_thread_num(params.n_thread),
                                filename(params.filename), completed_jobs(0), jobs_left(0), phase(0){
    game_matrix_curr = new int_mat;
    game_matrix_next = new int_mat;
}


void Game::execute_phase_one(tuple<int, int> range) {
    const int y[] = { -1, -1, -1,  1, 1, 1,  0, 0 };
    const int x[] = { -1,  0,  1, -1, 0, 1, -1, 1 };

    for(int i = get<0>(range); i < get<1>(range); i++){
        for(int ii = 0; ii < matrix_width; ii++){
            vector<uint> neighbors_specie_hist(8, 0);
            int alive_neighbors = 0;
            int dominant = 0;
            int max = 0;
            for(int k = 0; k < 8; k++){
                if(is_in_bounds(i+y[k], ii+x[k])){
                    if((*game_matrix_curr)[i+y[k]][ii+x[k]] > 0){
                        alive_neighbors++;
                    }
                    if(alive_neighbors <=  3){
                        neighbors_specie_hist[(*game_matrix_curr)[i+y[k]][ii+x[k]]]++;
                    }
                    if(alive_neighbors == 3){
                        for(uint j = 0; j < 8; j++){
                            if(max < j * neighbors_specie_hist[j]){
                                max = j * neighbors_specie_hist[j];
                                dominant = j;
                            }
                        }
                    }
                }
            }
            if((*game_matrix_curr)[i][ii] > 0 && (alive_neighbors == 2 || alive_neighbors == 3)){
                (*game_matrix_next)[i][ii] = (*game_matrix_curr)[i][ii];
            } else if((*game_matrix_curr)[i][ii] == 0 && alive_neighbors == 3){
                (*game_matrix_next)[i][ii] = dominant;
            } else{
                (*game_matrix_next)[i][ii] = 0;
            }
        }
    }
}


void Game::execute_phase_two(tuple<int, int> range) {
    const int y[] = { -1, -1, -1,  1, 1, 1,  0, 0 };
    const int x[] = { -1,  0,  1, -1, 0, 1, -1, 1 };

    for(int i = get<0>(range); i < get<1>(range); i++) {
        for (int ii = 0; ii<matrix_width; ii++) {
            if((*game_matrix_curr)[i][ii] == 0){
                (*game_matrix_next)[i][ii] = 0;
            } else{
                int alive_neighbors = 0;
                int sum = (*game_matrix_curr)[i][ii];
                for(int j = 0; j < 8; j++){
                    if(is_in_bounds(i+y[j], ii+x[j])){
                        if((*game_matrix_curr)[i+y[j]][ii+x[j]] > 0){
                            alive_neighbors++;
                            sum += (*game_matrix_curr)[i+y[j]][ii+x[j]];
                        }
                    }
                }
                int new_specie = round(sum / alive_neighbors);
                (*game_matrix_next)[i][ii] = new_specie;
            }
        }
    }
}


bool Game::is_in_bounds(int row, int column) {
        if(row < 0 || row >= matrix_height) return false;
        if(column < 0 || column >= matrix_width) return false;
        return true;
}

const vector<double> Game::gen_hist() const{
	return m_gen_hist;
}

const vector<double> Game::tile_hist() const{
	return m_tile_hist;
}

uint Game::thread_num() const{
	return m_thread_num;
}

Game::~Game() = default;



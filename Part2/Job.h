#ifndef CODE_SKELETON_JOB_H
#define CODE_SKELETON_JOB_H

#include "Game.hpp"
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
        pthread_mutex_lock(&game->mutex);
        game->completed_jobs++;
        if(game->completed_jobs == game->m_thread_num){
            pthread_cond_signal(&game->cond);
        }
        auto end = std::chrono::system_clock::now();
        m_tile_hist->push_back((double) std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());
        pthread_mutex_unlock(&game->mutex);
    }
};
#endif //CODE_SKELETON_JOB_H

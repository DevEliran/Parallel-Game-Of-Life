//
// Created by Eliran on 1/4/2021.
//

#ifndef CODE_SKELETON_JOB_H
#define CODE_SKELETON_JOB_H
class Job{
public:
    tuple<int, int> thread_range_coverage;
    uint matrix_height;
    uint matrix_width;
    bool phase;
    bool is_last_gen_phase_two;

    Job(tuple<int, int> range, uint h, uint w, bool phase, bool end):
            thread_range_coverage(range), matrix_height(h),matrix_width(w),
            phase(phase), is_last_gen_phase_two(end){}

    ~Job() = default;
};
#endif //CODE_SKELETON_JOB_H

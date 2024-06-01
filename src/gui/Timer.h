#include <ctime>
#include <iostream>

class Timer {
public:
    Timer() { clock_gettime(CLOCK_MONOTONIC, &beg_); }

    double elapsed() {
        clock_gettime(CLOCK_MONOTONIC, &end_);
        return end_.tv_sec - beg_.tv_sec +
               (end_.tv_nsec - beg_.tv_nsec) / 1000000000.;
    }

    void reset() { clock_gettime(CLOCK_MONOTONIC, &beg_); }

private:
    timespec beg_, end_;
};

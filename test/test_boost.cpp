#include <iostream>
#include <vector>
#include <boost/lockfree/queue.hpp>
#include <thread>

int main(int argc, char** argv) {
    int thread_num = 2;
    if (argc == 2) {
        thread_num = atoi(argv[1]);
    }

    std::vector<std::thread> threads;
    boost::lockfree::queue<int> queue(1024);
    auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < thread_num; ++i) {
        threads.emplace_back(std::thread([&]() {
            for (size_t i = 0; i < 1000000; ++i) {
                queue.push(i);
            }
        }));
    }

    for (int i = 0; i < thread_num; ++i) {
        threads.emplace_back(std::thread([&]() {
            int a;
            for (size_t i = 0; i < 1000000; ++i) {
                queue.pop(a);
            } 
        }));
    }
    
    for (auto & t : threads) {
        t.join();
    }

    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - begin).count();
    std::cout << cost << std::endl;

    return 0;
}

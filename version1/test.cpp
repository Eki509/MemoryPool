#include <iostream>
#include <thread>
#include <vector>
#include "../include/MemoryPool.h"

using namespace MyMemoryPool;

class Test1 { int a; public: Test1(int x) : a(x) {} };
class Test2 { int a[5]; public: Test2(int x) : a{x} {} };
class Test3 { int a[10]; public: Test3(int x) : a{x} {} };
class Test4 { int a[20]; public: Test4(int x) : a{x} {} };
class Test5 { int a[50]; public: Test5(int x) : a{x} {} };

void testMemoryPool(size_t works, size_t iterations, size_t rounds) {
    std::vector<std::thread> threads(works);
    size_t total_time = 0;
    for (size_t i = 0; i < works; ++i) {
        threads[i] = std::thread([&]() {
            for (int j = 0; j < rounds; ++j) {
                size_t start = clock();
                for(size_t k = 0; k < iterations; ++k) {
                    Test1* obj1 = newElement<Test1>(1);
                    deleteElement<Test1>(obj1);
                    Test2* obj2 = newElement<Test2>(2);
                    deleteElement<Test2>(obj2);
                    Test3* obj3 = newElement<Test3>(3);
                    deleteElement<Test3>(obj3);
                    Test4* obj4 = newElement<Test4>(4);
                    deleteElement<Test4>(obj4);
                    Test5* obj5 = newElement<Test5>(5);
                    deleteElement<Test5>(obj5);
                }
                size_t end = clock();
                total_time += (end - start);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Thread count: " << works << ". Round count: " << rounds << ". Iterations per round: " << iterations << std::endl;
    std::cout << "Average time per round: " << (total_time / (works * rounds)) << " ms" << std::endl;
    std::cout << "Total time: " << total_time << " ms" << std::endl;
}

void testNew(size_t works, size_t iterations, size_t rounds) {
    std::vector<std::thread> threads(works);
    size_t total_time = 0;
    for (size_t i = 0; i < works; ++i) {
        threads[i] = std::thread([&]() {
            for (int j = 0; j < rounds; ++j) {
                size_t start = clock();
                for(size_t k = 0; k < iterations; ++k) {
                    Test1* obj1 = new Test1(1);
                    delete obj1;
                    Test2* obj2 = new Test2(2);
                    delete obj2;
                    Test3* obj3 = new Test3(3);
                    delete obj3;
                    Test4* obj4 = new Test4(4);
                    delete obj4;
                    Test5* obj5 = new Test5(5);
                    delete obj5;
                }
                size_t end = clock();
                total_time += (end - start);
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::cout << "Thread count: " << works << ". Round count: " << rounds << ". Iterations per round: " << iterations << std::endl;
    std::cout << "Average time per round: " << (total_time / (works * rounds)) << " ms" << std::endl;
    std::cout << "Total time: " << total_time << " ms" << std::endl;
}

int main() {
    BlockToHash::initMemoryBlock();
    size_t works = 4; // Number of threads
    size_t iterations = 100; // Number of iterations per thread
    size_t rounds = 10; // Number of rounds

    std::cout << "----------------------Testing Memory Pool------------------------" << std::endl;
    testMemoryPool(works, iterations, rounds);

    std::cout << "=================================================================" << std::endl;

    std::cout << "----------------------Testing new/delete-------------------------" << std::endl;
    testNew(works, iterations, rounds);

    return 0;
}
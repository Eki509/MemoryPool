#include "./include/MemoryPool.h"
#include "./include/UseMemoryPool.h"
#include <iostream>
#include <thread>
#include <vector>

using namespace MyMemoryPool;

void testMemoryPool(size_t works, size_t rounds, size_t iterations){
    std::vector<std::thread> threads(works);
    size_t alloc_time = 0;
    size_t dealloc_time = 0;
    for(size_t i = 0; i < works; ++i){
        threads[i] = std::thread([&](){
            std::vector<void*> ptrVec;
            ptrVec.reserve(iterations);
            for(size_t j = 0; j < rounds; ++j){
                size_t start_alloc = clock();
                for(size_t k = 0; k < iterations; ++k){
                    ptrVec.push_back(localAllocate(std::min(((k + 1) * 64) % (512 * 1024) + 1, (size_t)MAX_BYTES)));
                }
                size_t end_alloc = clock();
                size_t start_dealloc = clock();
                for(size_t k = 0; k < iterations; ++k){
                    localDeallocate(ptrVec[k]);
                }
                size_t end_dealloc = clock();
                ptrVec.clear();
                alloc_time += (end_alloc - start_alloc);
                dealloc_time += (end_dealloc - start_dealloc);
            }
        });
    }
    for(auto& t : threads){
        t.join();
    }
    printf("%lu个线程并发执行%lu轮,每轮调用内存池alloc内存%lu次：花费%ld ms\n",
        works, rounds, iterations, alloc_time);
    printf("平均每轮alloc内存耗时：%ld ms\n", alloc_time / (works * rounds));
    printf("%lu个线程并发执行%lu轮,每轮调用内存池dealloc内存%lu次：花费%ld ms\n",
        works, rounds, iterations, dealloc_time);
    printf("平均每轮dealloc内存耗时：%ld ms\n", dealloc_time / (works * rounds));
    printf("%lu个线程并发执行内存池alloc/dealloc操作%lu次，总计花费%ld ms\n",
        works, works * iterations * rounds, alloc_time + dealloc_time);
}

void testMalloc(size_t works, size_t rounds, size_t iterations){
    std::vector<std::thread> threads(works);
    size_t malloc_time = 0;
    size_t free_time = 0;
    for(size_t i = 0; i < works; ++i){
        threads[i] = std::thread([&](){
            std::vector<void*> ptrVec;
            ptrVec.reserve(iterations);
            for(size_t j = 0; j < rounds; ++j){
                size_t start_malloc = clock();
                for(size_t k = 0; k < iterations; ++k){
                    ptrVec.push_back(malloc(std::min(((k + 1) * 64) % (512 * 1024) + 1, (size_t)MAX_BYTES)));
                }
                size_t end_malloc = clock();
                size_t start_free = clock();
                for(size_t k = 0; k < iterations; ++k){
                    free(ptrVec[k]);
                }
                size_t end_free = clock();
                ptrVec.clear();
                malloc_time += (end_malloc - start_malloc);
                free_time += (end_free - start_free);
            }
        });
    }
    for(auto& t : threads){
        t.join();
    }
    printf("%lu个线程并发执行%lu轮,每轮调用malloc分配内存%lu次：花费%ld ms\n",
        works, rounds, iterations, malloc_time);
    printf("平均每轮malloc内存耗时：%ld ms\n", malloc_time / (works * rounds));
    printf("%lu个线程并发执行%lu轮,每轮调用free释放内存%lu次：花费%ld ms\n",
        works, rounds, iterations, free_time);
    printf("平均每轮free内存耗时：%ld ms\n", free_time / (works * rounds));
    printf("%lu个线程并发执行malloc/free操作%lu次，总计花费%ld ms\n",
        works, works * iterations * rounds, malloc_time + free_time);
}

int main(){
    size_t works = 4; // 线程数
    size_t rounds = 10; // 每个线程执行的轮数
    size_t iterations = 1000; // 每轮分配/释放的次数

    std::cout << "=================Test Memory Pool=================" << std::endl;
    testMemoryPool(works, rounds, iterations);
    std::cout << std::endl;
    std::cout << "==================================================";
    std::cout << std::endl;
    std::cout << "===================Test Malloc====================" << std::endl;
    testMalloc(works, rounds, iterations);
    std::cout << std::endl;
    return 0;  
}
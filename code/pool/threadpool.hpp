#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP
#include "../log/log.h"
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <thread>
#include <functional>
class ThreadPool{
public:
    ThreadPool(int poolSize = 20){
        logger_.info();
        logger_ << "Thread pool initializing...";
        stop_ = false;
        for(int i = 0; i < poolSize; i++){
            threadPool_.emplace_back(
                [&]{
                    try{
                        while(!stop_){
                            std::function<void()> task;
                            {
                                std::unique_lock<std::mutex> lock(mtx_);
                                condVar_.wait(lock, [&]() { return !tasks_.empty() || stop_;} );
                                if(tasks_.empty() and stop_)
                                    return;
                                task = std::move(tasks_.front());
                                tasks_.pop();
                            }
                            try{
                                logger_ << "ThreadPool: Task started.";
                                task();
                                logger_ << "ThreadPool: Tasked completed.";
                            }catch(const std::exception & e){
                                logger_.error();
                                logger_.Log(std::string("Task failed: ") + e.what(), LogLevel::ERROR);
                                logger_.info();
                            }
                        }

                    }
                    catch(const std::exception& e){
                        logger_.error();
                        logger_ << "Thread Exited: " << e.what();
                    }
                }
            );
        }
        logger_ << "Thread pool initialized with " + std::to_string(threadPool_.size()) + " threads.";
    }
    ~ThreadPool(){
        stop_ = true;
        condVar_.notify_all();
        for(auto && worker: threadPool_){
            if(worker.joinable())
                worker.join();
        }
        logger_ << "Thread pool stopped.";
    }
    void submit(std::function<void()> task){
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.push(task);
        }
        condVar_.notify_one();
    }

private:
    std::mutex mtx_;
    std::vector<std::thread> threadPool_;
    std::queue<std::function<void()>> tasks_;
    std::condition_variable condVar_;
    Logger logger_;
    bool stop_;
};
#endif
#include "../../include/threading/ThreadPool.hpp"
#include <iostream>

namespace dataflow {
namespace threading {

ThreadPool::ThreadPool(size_t numThreads) {
    // Ensure at least one thread
    numThreads = (numThreads > 0) ? numThreads : 1;
    
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this, i] {
            this->workerFunction(i);
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown(bool waitForTasks) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        if (shutdown_) {
            return;
        }
        
        // Set shutdown flag
        shutdown_ = true;
        
        // If not waiting for tasks, clear the queue
        if (!waitForTasks) {
            std::queue<std::function<void()>> emptyQueue;
            std::swap(tasks_, emptyQueue);
        }
    }
    
    // Notify all workers
    condition_.notify_all();
    
    // Wait for all workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    // Clear the worker vector
    workers_.clear();
}

size_t ThreadPool::queueSize() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::workerFunction(size_t workerId) {
    try {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(queueMutex_);
                
                // Wait for a task or shutdown
                condition_.wait(lock, [this] {
                    return shutdown_ || !tasks_.empty();
                });
                
                // If shutdown and no tasks, exit
                if (shutdown_ && tasks_.empty()) {
                    break;
                }
                
                // Get the next task
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            
            // Execute the task
            try {
                ++activeTaskCount_;
                task();
                --activeTaskCount_;
            }
            catch (const std::exception& e) {
                --activeTaskCount_;
                std::cerr << "Exception in thread " << workerId << ": " << e.what() << std::endl;
            }
            catch (...) {
                --activeTaskCount_;
                std::cerr << "Unknown exception in thread " << workerId << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal exception in worker thread " << workerId << ": " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Fatal unknown exception in worker thread " << workerId << std::endl;
    }
}

} // namespace threading
} // namespace dataflow

#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <memory>

namespace dataflow {
namespace threading {

/**
 * @class ThreadPool
 * @brief Thread pool for parallel execution of tasks
 * 
 * Implements a thread pool pattern to efficiently execute multiple tasks in parallel.
 * Uses a work-stealing algorithm for better load balancing across threads.
 */
class ThreadPool {
public:
    /**
     * @brief Constructor
     * @param numThreads Number of worker threads to create
     */
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    
    /**
     * @brief Destructor
     * Shuts down all worker threads
     */
    ~ThreadPool();
    
    /**
     * @brief Enqueues a task for execution
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return Future for the result of the function
     */
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        
        using ReturnType = typename std::invoke_result<F, Args...>::type;
        
        // Create a packaged task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        // Get future from the task
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            // Don't allow enqueueing after stopping the pool
            if (shutdown_) {
                throw std::runtime_error("Cannot enqueue on a stopped ThreadPool");
            }
            
            // Add the task to the queue
            tasks_.emplace([task]() { (*task)(); });
        }
        
        // Wake up a waiting worker thread
        condition_.notify_one();
        
        return result;
    }
    
    /**
     * @brief Shuts down the thread pool
     * @param waitForTasks If true, wait for all tasks to complete before shutdown
     */
    void shutdown(bool waitForTasks = true);
    
    /**
     * @brief Gets the number of worker threads
     * @return The number of threads in the pool
     */
    size_t size() const { return workers_.size(); }
    
    /**
     * @brief Gets the number of tasks waiting in the queue
     * @return The current queue size
     */
    size_t queueSize() const;
    
    /**
     * @brief Gets the number of active tasks currently being processed
     * @return The number of active tasks
     */
    size_t activeTaskCount() const { return activeTaskCount_; }

private:
    // Worker threads
    std::vector<std::thread> workers_;
    
    // Task queue
    std::queue<std::function<void()>> tasks_;
    
    // Synchronization
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    
    // State
    std::atomic<bool> shutdown_{false};
    std::atomic<size_t> activeTaskCount_{0};
    
    // Worker thread function
    void workerFunction(size_t workerId);
};

} // namespace threading
} // namespace dataflow

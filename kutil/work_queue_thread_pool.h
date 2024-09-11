#ifndef KUTIL_WORK_QUEUE_THREAD_POOL
#define KUTIL_WORK_QUEUE_THREAD_POOL

#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <random>
#include <algorithm>

namespace kutil {
/**
 * Thread pool each thread owns a task queue
 * 
 */
class WorkQueueThreadPool {
public:
    using Task = std::function<void()>;

    class TaskQueue {
    public:
        TaskQueue() : _hungry(true) {}
        
        ~TaskQueue() {}

        inline bool is_hungry() const {
            return _hungry;
        }

        inline bool empty() const {
            return _tasks.empty();
        }

        inline size_t size() const {
            return _tasks.size();
        }

        void push(const Task& task) {
            _tasks.push(std::move(task));
            _hungry = false;
        }

        Task pop() {
            Task task;
            if (_tasks.empty()) {
                _hungry = true;
            } else {
                task = std::move(_tasks.front());
                _tasks.pop();
            }
            return task;
        }
    
    private:
        std::queue<Task> _tasks;
        bool _hungry;
        
    };

    WorkQueueThreadPool(size_t num_threads) : _stop(false) {
        for (size_t i = 0; i < num_threads; i++) {
            _task_queues.emplace_back(std::make_unique<TaskQueue>());
            _queue_mutexes.emplace_back(std::make_unique<std::mutex>());
            _queue_cv.emplace_back(std::make_unique<std::condition_variable>());
        }
        for (size_t i = 0; i < num_threads; i++) {
            _workers.emplace_back(&WorkQueueThreadPool::do_work, this, i);
        }
    }

    ~WorkQueueThreadPool() {
        if (!_stop) {
            shut_down();
        }
    }

    /**
     * shutdown thread pool, tasks that are not executed will be ignored
     */
    void shut_down() {
        _stop = true;
        for (size_t i = 0; i < _workers.size(); i++) {
            _queue_cv[i]->notify_all();
        }
        for (std::thread &worker : _workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    template <typename FunctionType, typename... Args>
    std::future<typename std::result_of<FunctionType(Args...)>::type>
    submit(FunctionType&& func, Args&&... args) {
        typedef typename std::result_of<FunctionType(Args...)>::type result_type;
        if (_stop) {
            return std::future<result_type>();
        }
        auto task = std::make_shared<std::packaged_task<result_type()>>(
            std::bind(std::forward<FunctionType>(func), std::forward<Args>(args)...)
        );
        std::future<result_type> future = task->get_future();
        size_t q_size = UINT32_MAX;
        size_t idx = 0;
        for (size_t i = 0; i < _task_queues.size(); i++) {
            if (_task_queues[i]->is_hungry()) {
                idx = i;
                break;
            }
            size_t t = _task_queues[i]->size();
            if (t < q_size) {
                idx = i;
                q_size = t;
            }
        }
        {
            std::unique_lock<std::mutex> lock(*_queue_mutexes[idx]);
            if (_stop) {
                return std::future<result_type>();
            }
            _task_queues[idx]->push([task](){
                (*task)();
            });
        }
        _queue_cv[idx]->notify_one();
        return future;
    }
    /**
     * check if all tasks are finished
     */
    bool all_task_done() {
        for (size_t i = 0; i < _task_queues.size(); i++) {
            std::unique_lock<std::mutex> lock(*_queue_mutexes[i]);
            if (!_task_queues[i]->empty()) {
                return false;
            }
        }
        return true;
    }

private:
    void do_work(size_t worker_id) {
        while (!_stop) {   
            Task task;
            {
                std::unique_lock<std::mutex> lock(*_queue_mutexes[worker_id]);
                _queue_cv[worker_id]->wait(lock, [this, worker_id] { return _stop || !_task_queues[worker_id]->is_hungry(); });
                if (_stop && _task_queues[worker_id]->empty()) {
                    return;
                }
                auto& tqueue = _task_queues[worker_id];
                task = tqueue->pop();
            }
            if (task) {
                task();
            }
        }
    }

private:
    // worker threads
    std::vector<std::thread> _workers;
    // task queue for each thread
    std::vector<std::unique_ptr<TaskQueue>> _task_queues;
    // mutex for each task queue
    std::vector<std::unique_ptr<std::mutex>> _queue_mutexes;
    // condition variable
    std::vector<std::unique_ptr<std::condition_variable>> _queue_cv;
    // stop flag
    bool _stop;
};
    
} // namespace kutil

#endif
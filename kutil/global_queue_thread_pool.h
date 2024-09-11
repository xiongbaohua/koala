#ifndef KUTIL_SIMPLE_THREAD_POOL_H
#define KUTIL_SIMPLE_THREAD_POOL_H

#include <vector>
#include <functional>
#include <condition_variable>
#include <future>
#include <thread>
#include <mutex>
#include <queue>

namespace kutil {

/**
 * A simple thread pool, all threads share from a global task queue, 
 * and fetch task from it.
 */
class SimpleThreadPool {
public:
    using Task = std::function<void()>;

    SimpleThreadPool(size_t num_threads) : _stop(false) {
        for (int i = 0; i < num_threads; i++) {
            _workers.emplace_back(&SimpleThreadPool::do_work, this);
        }
    }

    ~SimpleThreadPool() {
        if (!_stop) {
            shut_down();
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
        {
            std::lock_guard<std::mutex> lk(_mutex);
            if (_stop) {
                return std::future<result_type>();
            }
            _tasks.push([task]() {
                (*task)();
            });
        }
        _cv.notify_one();
        return future;
    }

    /**
     * shutdown thread pool, tasks that are not executed will be ignored
     */
    void shut_down() {
        _stop = true;
        _cv.notify_all();
        for (auto& t : _workers) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    bool all_task_done() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _tasks.empty();
    }

private:
    void do_work() {
        while (!_stop) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _cv.wait(lock, [this]{
                    // wakeup when _stop == true or _tasks.size() > 0
                    return this->_stop || !this->_tasks.empty(); 
                });
                if (_stop && _tasks.empty()) {
                    return;
                }
                task = std::move(_tasks.front());
                _tasks.pop();
            }
            if (task) {
                task();
            }
        }
    }

private:
    // thread workers
    std::vector<std::thread> _workers;
    // global task queue
    std::queue<Task> _tasks;
    // global task queue mutex
    std::mutex _mutex;
    // stop flag
    bool _stop;
    // condition variable
    std::condition_variable _cv;

};

} // namespace kutil

#endif
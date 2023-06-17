﻿#ifndef QUQICOROR_HPP
#define QUQICOROR_HPP

#define QUQI_NAME_SPACE_START namespace qcoro {
#define QUQI_NAME_SPACE_END }

#include <coroutine>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <functional>
#include <vector>
#include <condition_variable>
#include <stdexcept>

QUQI_NAME_SPACE_START

// 执行器
class executor
{
public:
    executor() = default;
    executor(const executor&) = delete;
    executor(executor&&) noexcept = default;
    virtual ~executor() = default;

    executor& operator=(const executor&) = delete;
    executor& operator=(executor&&) = default;

    template<typename Func, typename... Args>
    void post(Func&& func, Args&&... args)
    {
        _post(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    }

protected:
    virtual void _post(std::function<void()> func)
    {
        func();
    }
};

//线程池
class thread_pool : public executor
{
public:
    explicit thread_pool(int num = 12) :
        can_join_(true),
        is_running_(true)
    {
        auto work_func = [this]()
            {
                while (true)
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cv_.wait(lock, [&]() {return !funcs_.empty() || !is_running_; });
                    if (!is_running_ && funcs_.empty())
                        return;
                    else if (funcs_.empty())
                        continue;

                    auto task = std::move(funcs_.front());
                    funcs_.pop();

                    lock.unlock();

                    task();
                }
            };

        for (int i = 0; i < num; i++)
        {
            threads_.emplace_back(work_func);
        }
    }
    virtual ~thread_pool()
    {
        if (!can_join_)
            return;
        is_running_ = false;
        cv_.notify_all();
        detach();
    }

    bool joinable() const
    {
        return can_join_;
    }

    void join()
    {
        if (!can_join_)
            throw std::runtime_error("can't join");
        can_join_ = false;
        is_running_ = false;
        cv_.notify_all();
        for (auto i = threads_.begin(); i != threads_.end(); i++)
        {
            if (i->joinable())
                i->join();
        }
    }

    void detach()
    {
        if (!can_join_)
            throw std::runtime_error("can't detach");
        can_join_ = false;
        for (auto i = threads_.begin(); i != threads_.end(); i++)
        {
            i->detach();
        }
    }

    template<typename Func, typename... Args>
    void post(Func&& func, Args&&... args)
    {
        _post(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    }

protected:
    virtual void _post(std::function<void()> func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        funcs_.push(func);
        cv_.notify_one();
    }

private:
    std::atomic<bool>                   is_running_;
    std::queue<std::function<void()>>   funcs_;
    std::vector<std::thread>            threads_;
    std::condition_variable             cv_;
    std::mutex                          mutex_;
    bool                                can_join_;
};

class completion
{
public:
    completion() = default;
    ~completion() = default;
};

// 非 void 协程
template<class T, class RT = T>
class coroutine final
{
public:
    class promise_type;

    class iterator
    {
    public:
        T& operator*()
        {
            return h.promise().result_;
        }

        const T& operator*() const
        {
            return h.promise().result_;
        }

        iterator operator++()
        {
            if (!h.done())
                h.resume();
            return *this;
        }

        bool operator==(const iterator&) const
        {
            return h.done();
        }

        bool operator!=(const iterator&) const
        {
            return !h.done();
        }

        std::coroutine_handle<promise_type>& h;
    };

    class promise_type
    {
    public:
        T result_;
        RT revalue_;

        coroutine get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T value)
        {
            result_ = value;
            return {};
        }
        void unhandled_exception() { std::terminate(); }
        void return_value(RT value)
        {
            revalue_ = value;
        }
    };

    T& operator ()()
    {
        if (!handle_.done())
        {
            handle_.resume();
            return handle_.promise().result_;
        }
        else
        {
            return handle_.promise().revalue_;
        }
    }

    const T& operator ()() const
    {
        if (!handle_.done())
        {
            handle_.resume();
            return handle_.promise().result_;
        }
        else
        {
            return handle_.promise().revalue_;
        }
    }

    iterator begin() { return { handle_ }; }
    iterator end() { return { handle_ }; }

    std::coroutine_handle<promise_type> handle_;
};

// void协程
template<class T>
class coroutine<T, void> final
{
public:
    class promise_type;

    class iterator
    {
    public:
        T& operator*()
        {
            return handle_.promise().result_;
        }

        const T& operator*() const
        {
            return handle_.promise().result_;
        }

        iterator operator++()
        {
            if (!handle_.done())
                handle_.resume();
            return *this;
        }

        bool operator==(const iterator&) const
        {
            return handle_.done();
        }

        bool operator!=(const iterator&) const
        {
            return !handle_.done();
        }

        std::coroutine_handle<promise_type>& handle_;
    };

    class promise_type
    {
    public:
        T result_;

        coroutine get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T value)
        {
            result_ = value;
            return {};
        }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    iterator begin() { return { handle_ }; }
    iterator end() { return { handle_ }; }

    T& operator ()()
    {
        if (!handle_.done())
        {
            handle_.resume();
            return handle_.promise().result_;
        }
        else
        {
            throw std::runtime_error("end");
        }
    }

    const T& operator ()() const
    {
        if (!handle_.done())
        {
            handle_.resume();
            return handle_.promise().result_;
        }
        else
        {
            throw std::runtime_error("end");
        }
    }

    std::coroutine_handle<promise_type> handle_;
};

// 双 void 协程
template<>
class coroutine<void, void> final
{
public:
    class promise_type;

    class iterator
    {
    public:
        iterator operator++()
        {
            if (!handle_.done())
                handle_.resume();
            return *this;
        }

        bool operator==(const iterator&) const
        {
            return handle_.done();
        }

        bool operator!=(const iterator&) const
        {
            return !handle_.done();
        }

        std::coroutine_handle<promise_type>& handle_;
    };

    class promise_type
    {
    public:
        coroutine get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { /*std::terminate();*/ }
        void return_void() {}
    };

    iterator begin() { return { handle_ }; }
    iterator end() { return { handle_ }; }

    void operator ()()
    {
        if (!handle_.done())
        {
            handle_.resume();
        }
        else
        {
            throw std::runtime_error("end");
        }
    }

    std::coroutine_handle<promise_type> handle_;
};

template<class T>
class awaiter final
{
public:
    class promise_type;
    using function_type = std::function<void(std::function<void(T)>, executor&)>;

    awaiter(std::coroutine_handle<promise_type> h)
        : executor_(local_executor_)
    {
        handle_ = h;
    }
    awaiter(function_type func, executor& e)
        : executor_(e)
    {
        func_ = func;
    }
    ~awaiter() = default;

    class promise_type
    {
    public:
        T result_;

        awaiter get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_value(T value)
        {
            result_ = value;
        }
    };

    bool    await_ready() const { return ready_; }
    T       await_resume() { return result_; }
    void    await_suspend(std::coroutine_handle<> handle)
    {
        auto f = [handle, this](T value) {
            result_ = std::move(value);
            ready_ = true;
            handle.resume();
            };
        func_(f, executor_);
    }
    void unhandled_exception() { std::terminate(); }

    std::coroutine_handle<promise_type> handle_;
    T               result_;
    function_type   func_;
    executor        &executor_;
    executor        local_executor_;
    bool            ready_ = false;
};

template<>
class awaiter<void> final
{
public:
    class promise_type;
    using function_type = std::function<void(std::function<void()>, executor&)>;

    awaiter(std::coroutine_handle<promise_type> h)
        : executor_(local_executor_)
    {
        handle_ = h;
    }
    awaiter(function_type func, executor& e)
        : executor_(e)
    {
        func_ = func;
    }
    ~awaiter() = default;

    class promise_type
    {
    public:
        awaiter get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this) }; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}
    };

    bool    await_ready() const { return ready_; }
    void    await_resume() {}
    void    await_suspend(std::coroutine_handle<> handle)
    {
        auto f = [handle, this]() {
            ready_ = true;
            handle.resume();
            };
        func_(f, executor_);
    }
    void unhandled_exception() { std::terminate(); }

    std::coroutine_handle<promise_type> handle_;
    function_type   func_;
    executor&       executor_;
    executor        local_executor_;
    bool            ready_ = false;
};

void co_spawn(std::function<qcoro::awaiter<void>(qcoro::executor&)> func, qcoro::executor& e)
{
    auto awaiter = func(e);
    while (!awaiter.handle_.done())
    {
    }
}

QUQI_NAME_SPACE_END

#endif // !QUQICOROR_HPP
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<thread>
#include<condition_variable>
#include<mutex>
#include<vector>
#include<queue>
#include<future>

class ThreadPool
{
private:
    bool m_stop;
    std::vector<std::thread> m_thread;
    std::queue<std::function<void()>> tasks;
    std::mutex m_mutex;
    std::condition_variable m_cv;

public:
    explicit ThreadPool(size_t threadNum):m_stop(false){
        for(int i=0;i<threadNum;i++){
            m_thread.emplace_back(//执行构造函数，在vector中原地构造thread对象，传入一个Lamda表达式函数作为参数
                [this](){//捕捉外部变量this，即此threadpool
                    for(;;){//循环等待
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lk(m_mutex);//condition variable使用的锁，使用全局锁m_mutex初始化
                            m_cv.wait(lk,[this](){return m_stop||!tasks.empty();});             //第一个参数为锁，第二个参数为当这个参数为false时，线程才会阻塞
                            if(m_stop&&tasks.empty()) return;                                  //当这个参数为true时，其他线程唤醒这个线程才会解除阻塞
                            task=std::move(tasks.front());//移动构造，因为tasks中任务即将被销毁     //这里的实际意义为，当m_stop为false时或者任务队列为空时等待
                            tasks.pop();
                        }//大括号括住，出这个范围后锁将自动销毁
                        task();//执行任务
                    }
                }
            );
        }
    }

    ThreadPool(const ThreadPool&)=delete;
    ThreadPool(ThreadPool&&)=delete;

    ThreadPool& operator=(const ThreadPool&)=delete;
    ThreadPool& operator=(ThreadPool&&)=delete;

    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_stop=true;
        }
        m_cv.notify_all();//唤醒所有等待线程
        for(auto& threads:m_thread){
            threads.join();
        }
    }
    template<typename F,typename... Args>
    auto submit(F&& f,Args&&... args)->std::future<decltype(f(args...))>{//这里的&&为万能引用，不是右值引用
        auto taskPtr=std::make_shared<std::packaged_task<decltype(f(args...))()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );
        {
            std::unique_lock<std::mutex>lk(m_mutex);
            if(m_stop) throw std::runtime_error("submit on stopped ThreadPool");
            tasks.emplace([taskPtr](){ (*taskPtr)(); });
        }
        m_cv.notify_one();
        return taskPtr->get_future();

    }

};

#endif

//1.std::future  future使用的时机是当你不需要立刻得到一个结果的时候，你可以开启一个线程帮你去做一项任务，并期待这个任务的返回
//2.std::packaged_task  可以通过std::packaged_task对象获取任务相关联的future，调用get_future()方法可以获得std::packaged_task对象
//绑定的函数的返回值类型的future。std::packaged_task的模板参数是函数签名   PS：例如int add(int a, intb)的函数签名就是int(int, int)  

//submit执行的流程：
//其整体为一个Lamda函数，参数1为我们需要执行的函数，参数2为这个函数的参数，->后为函数的返回值的类型，是一个future对象，
//函数体中，taskPtr为我们将要执行的函数的封装，由内到外的封装为：
//第一层：forward完美转发，作用为它接受一个参数，然后返回该参数本来所对应的类型的引用
//第二层：bind的封装，将函数和其所要使用的参数封装在一起
//第三层：packaged_task，将函数与future对象绑定
//第四层：make_shared，将其封装成一个共享ptr以便能够复制construct/assign
//接下来我们将这个函数的封装再用void函数包装起来，因为所有函数f可能具有不同的返回类型，将它们存储在容器（我们的队列）中的唯一方法是用通用 void 函数包装它们
//然后将这个函数封装加入到任务队列中，唤醒一个等待线程，返回一个future，此时阻塞在wait上的线程将执行刚刚加入队列的函数
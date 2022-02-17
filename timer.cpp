#include "timer.h"

void TimerManager::siftup_(size_t i){
    assert(i>=0&&i<heap_.size());
    size_t j=(i-1)/2;
    while(j>=0){
        if(heap_[j]<heap_[i]) break;
        swapNode_(i,j);
        i=j;
        j=(i-1)/2;
    }
}

void TimerManager::swapNode_(size_t i,size_t j){
    assert(i>=0&&i<heap_.size());
    assert(j>=0&&j<heap_.size());
    std::swap(heap_[i],heap_[j]);
    ref_[heap_[i].id]=i;//节点已经交换，此时heap_[i]中时heap_[j]的节点
    ref_[heap_[j].id]=j;
}

bool TimerManager::siftdown_(size_t u,size_t n){
    assert(u>=0&&u<heap_.size());
    assert(n>=0&&n<=heap_.size());
    size_t i=u;
    if(2*i+1<n&&heap_[2*i+1]<heap_[u]) i=2*u+1;//小顶堆
    if(2*i+2<n&&heap_[2*i+2]<heap_[u]) i=2*u+2;//堆的调整是通过重载<运算符比较expire时间来完成的，越往上过期时间越小
    if(u!=i){
        swapNode_(i,u);
        siftdown_(i,n);
    }
    return i>u;
}

void TimerManager::addTimer(int id,int timeout,const TimeoutCallBack& cb){
    assert(id>=0);
    size_t i;
    if(ref_.count(id)==0){//新节点，堆尾插入，调整堆
        i=heap_.size();
        ref_[id]=i;
        heap_.push_back({id,Clock::now()+MS(timeout),cb});
        siftup_(i);
    }else{
        //已有节点，调整堆
        i=ref_[id];
        heap_[i].expire=Clock::now()+MS(timeout);
        heap_[i].cb=cb;
        if(!siftdown_(i,heap_.size())){//向下调整失败则向上调整
            siftup_(i);
        }
    }
}

void TimerManager::CallbackWork(int id){
    //删除指定id节点，并触发回调函数
    if(heap_.empty()||ref_.count(id)==0) return;
    size_t i=ref_[id];
    TimerNode node=heap_[i];
    node.cb();
    del_(i);
}

void TimerManager::del_(size_t index){
    //删除指定位置的节点
    assert(!heap_.empty()&&index>=0&&index<heap_.size());
    //将要删除的节点换到队尾，对除队尾之外的堆进行调整
    size_t i=index;
    size_t n=heap_.size()-1;
    assert(i<=n);
    if(i<n){
        swapNode_(i,n);
        if(!siftdown_(i,n)){//队尾元素已经调整到i的位置，对这个原本再队尾的元素进行调整
            siftup_(i);
        }
    }
    //队尾元素删除
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void TimerManager::update(int id,int timeout){
    //调整指定id的节点
    assert(!heap_.empty()&&ref_.count(id)>0);
    heap_[ref_[id]].expire=Clock::now()+MS(timeout);
    siftdown_(ref_[id],heap_.size());
}

void TimerManager::handle_expired_event(){
    //清除超时节点
    if(heap_.empty()){
        return;
    }
    while(!heap_.empty()){
        TimerNode node=heap_.front();
        if(std::chrono::duration_cast<MS>(node.expire-Clock::now()).count()>0){//最小的节点的都没有过期，说明没有节点过期了
            break;
        }
        node.cb();
        pop();
    }
}

void TimerManager::pop(){
    assert(!heap_.empty());
    del_(0);
}

void TimerManager::clear(){
    ref_.clear();
    heap_.clear();
}

int TimerManager::getNextHandle(){
    handle_expired_event();
    size_t res=-1;
    if(!heap_.empty()){
        res=std::chrono::duration_cast<MS>(heap_.front().expire-Clock::now()).count();
        if(res<0) res=0;
    }
    return res;
}




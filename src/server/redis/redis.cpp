#include "redis.h"

#include <iostream>
#include <thread>

Redis::Redis() : publish_context_(nullptr), subscribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_)
    {
        redisFree(publish_context_);
    }
    if (subscribe_context_)
    {
        redisFree(subscribe_context_);
    }
}

// 连接redis
bool Redis::connect()
{
    // 负责publish发布消息的上下文连接,连接redis-server
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 负责subscribe
    subscribe_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr)
    {
        std::cerr << "connect redis failed!" << std::endl;
        return false;
    }

    // 在单独的线程中监听通道上的事件,有消息给业务层上报
    std::thread t([this]()
                  { observer_channel_message(); });
    t.detach();

    std::cout << "connect redis-server success!" << std::endl;

    return true;
}

// 向redis指定的通道发布消息
bool Redis::publish(int channel, std::string message)
{
    redisReply *reply = static_cast<redisReply *>(redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str()));
    if (reply == nullptr)
    {
        std::cerr << "publish command failed!" << std::endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息,这里只做订阅通道,不接收通道消息
    // 通道消息的接收专门在observe_channel_message函数中的独立线程中进行
    // 只负责发送命令,不阻塞接收redis_server响应消息,否则和notifyMsg线程抢占响应
    if (REDIS_ERR == redisAppendCommand(this->subscribe_context_, "SUBSCRIBE %d", channel))
    {
        std::cerr << "subscribe command failed!" << std::endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区,直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->subscribe_context_, &done))
        {
            std::cerr << "subscribe command failed!" << std::endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

// 向redis指定的通道取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if (REDIS_ERR == redisAppendCommand(this->subscribe_context_, "UNSUBSCRIBE %d", channel))
    {
        std::cerr << "unsubscribe command failed!" << std::endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区,直到缓冲区数据发送完毕(done被置为1)
    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(this->subscribe_context_, &done))
        {
            std::cerr << "unsubscribe command failed!" << std::endl;
            return false;
        }
    }
    // redisGetReply

    return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(this->subscribe_context_, (void **)&reply))
    {
        // 订阅收到的消息是一个带三元素的数组
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            // 给业务层上报通道上发生的消息
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    std::cerr << ">>>>>>>>>>>>>>>>>>>>>> observer_channel_message quit <<<<<<<<<<<<<<<<<<<<<<" << std::endl;
}

// 初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(std::function<void(int, std::string)> fn)
{
    this->notify_message_handler_ = std::move(fn);
}
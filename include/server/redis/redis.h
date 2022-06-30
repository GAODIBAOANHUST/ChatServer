#pragma once

#include <hiredis/hiredis.h>
#include <functional>
#include <string>

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis
    bool connect();
    // 向redis指定的通道发布消息
    bool publish(int channel, std::string message);
    // 向redis指定的通道订阅消息
    bool subscribe(int channel);
    // 向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);
    // 在独立线程中接收订阅通道中的消息
    void observer_channel_message();
    // 初始化向业务层上报通道消息的回调对象
    void init_notify_handler(std::function<void(int, std::string)> fn);
private:
    // hiredis同步上下文对象
    redisContext* publish_context_;
    redisContext* subscribe_context_;

    // 回调操作,收到订阅的消息,给service层上报
    std::function<void(int, std::string)> notify_message_handler_;
};
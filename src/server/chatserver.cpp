#include "chatserver.h"
#include "chatservice.h"

#include <nlohmann/json.hpp>
#include <functional>
#include <string>

// 初始化聊天服务器对象
ChatServer::ChatServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, const std::string &nameArg) : server_(loop, listenAddr, nameArg), loop_(loop)
{
    // 注册连接回调
    server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
    // 注册消息回调
    server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // 设置线程数量
    server_.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    server_.start();
}

// 上报链接相关信息的回调
void ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调
void ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer, muduo::Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    nlohmann::json js = nlohmann::json::parse(buf);
    // 打到的目的：完全解耦网络模块的代码和业务模块的代码
    // 通过js["msgid"] 获取->业务handler->conn js time
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器,来执行响应的业务处理
    msgHandler(conn, js, time);
}
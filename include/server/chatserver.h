#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

// 聊天服务器的主类
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(muduo::net::EventLoop* loop, const muduo::net::InetAddress& listenAddr, const std::string& nameArg);

    // 启动服务
    void start();
private:
    // 上报链接相关信息的回调
    void onConnection(const muduo::net::TcpConnectionPtr& conn);

    // 上报读写事件相关信息的回调
    void onMessage(const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buffer, muduo::Timestamp time);

    muduo::net::TcpServer server_; // 组合的muduo库,实现服务器功能的类对象
    muduo::net::EventLoop* loop_;   // 指向事件循环对象的指针
};
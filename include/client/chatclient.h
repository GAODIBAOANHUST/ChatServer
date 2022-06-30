#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>

class ChatClient
{
public:
    // 初始化客户端
    ChatClient(muduo::net::EventLoop* loop, const muduo::net::InetAddress& serverAddr, const std::string nameArg);

    // 客户端发起连接
    void connect();
    
private:
    muduo::net::TcpClient client_;
    muduo::net::EventLoop* loop_;
};
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace muduo;
using namespace muduo::net;

// 基于muduo网络库开发服务器程序
// 1.组合Tcpserver对象
// 2.创建EventLoop时间循环对象的指针
// 3.明确TcpServer构造函数的参数
// 4.在当前服务器类的构造函数当中，注册处理连接的回调函数和处理读写事件的回调函数
// 5.设置合适的服务端线程数量，muduo库会设置
class ChatServer
{
public:
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg):server_(loop, listenAddr, nameArg), loop_(loop) 
    {
        // 给服务器注册用户连接的创建和断开回调
        server_.setConnectionCallback(bind(&ChatServer::onConnection, this, placeholders::_1));
        // 给服务器注册用户读写事件回调
        server_.setMessageCallback(bind(&ChatServer::onMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
        // 设置服务器端的线程数量
        server_.setThreadNum(8);
    }

    void start()
    {
        server_.start();
    }

private:
    // 专门处理用户的连接和断开
    void onConnection(const TcpConnectionPtr& conn)
    {
        if(conn->connected())
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state : online" << endl;
        }
        else
        {
            cout << conn->peerAddress().toIpPort() << "->" << conn->localAddress().toIpPort() << "state : offline" << endl;
            conn->shutdown(); // close(fd)
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time)
    {
        string buf = buffer->retrieveAllAsString();
        cout << "recv data: " << buf << " time: " << time.toString() << endl;
        conn->send(buf);
    }

    TcpServer server_; // #1
    EventLoop* loop_; // 
};

int main()
{
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop(); // epoll_wait以阻塞方式等待新用户的连接和已连接用户的读写事件等

    return 0;
}
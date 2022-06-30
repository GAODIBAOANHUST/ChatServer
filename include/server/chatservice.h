# pragma once

#include <muduo/net/TcpConnection.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "public.h"
#include "usermodel.h"
#include "offlinemessagemodel.h"
#include "friendmodel.h"
#include "groupmodel.h"
#include "redis.h"

// 表示处理消息的时间回调方法类型
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr&, nlohmann::json&, muduo::Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登录业务
    void Login(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 处理注册业务
    void Register(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 一对一聊天业务
    void oneChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 添加好友
    void addFriend(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 创建群组
    void createGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 加入群组
    void addGroup(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 群组聊天
    void groupChat(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 处理注销业务
    void Loginout(const muduo::net::TcpConnectionPtr& conn, nlohmann::json& js, muduo::Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid, std::string msg);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const muduo::net::TcpConnectionPtr& conn);
    // 服务器异常,业务重置方法
    void reset();

private:
    ChatService();
    // 存储消息id和其对应的业务处理方法
    std::unordered_map<int, MsgHandler> msgHandlerMap_;

    // 存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> userConnMap_;

    // 定义互斥锁保证userConnMap的线程安全
    std::mutex mtx_;

    // 数据操作类对象
    UserModel userModel_;
    OfflineMessageModel offlinemessageModel_;
    FriendModel friendModel_;
    GroupModel groupModel_;

    // redis对象
    Redis redis_;
};
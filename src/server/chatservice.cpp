#include "chatservice.h"
#include <muduo/base/Logging.h>

#include <vector>

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的回调操作
ChatService::ChatService()
{
    msgHandlerMap_.emplace(LOGIN_MSG, std::bind(&ChatService::Login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(LOGINOUT_MSG, std::bind(&ChatService::Loginout, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(REG_MSG, std::bind(&ChatService::Register, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    msgHandlerMap_.emplace(GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 连接redis服务器
    if (redis_.connect())
    {
        // 设置上报消息的回调
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, std::placeholders::_1, std::placeholders::_2));
    }
}

// 处理登录业务
void ChatService::Login(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    std::string pwd = js["password"];

    User user = userModel_.query(id);
    if (user.getId() != -1 && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录,不允许重复登录
            nlohmann::json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            {
                // 记录用户连接信息
                std::lock_guard<std::mutex> lock(mtx_);
                userConnMap_.emplace(id, conn);
            }

            // id用户登录成功后,向redis订阅channel(id)
            redis_.subscribe(id);

            // 登录成功,更新用户状态
            user.setState("online");
            userModel_.updateState(user);

            nlohmann::json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 查询用户是否有离线消息
            std::vector<std::string> vec = offlinemessageModel_.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息之后,把该用户的所有离线消息删除掉
                offlinemessageModel_.remove(id);
            }
            // 查询该用户的好友信息并返回
            std::vector<User> userVec = friendModel_.query(id);
            if (!userVec.empty())
            {
                std::vector<std::string> vec2;
                for (auto &user : userVec)
                {
                    nlohmann::json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 查询用户的群组信息
            std::vector<Group> groupuserVec = groupModel_.queryGroups(id);
            if (!groupuserVec.empty())
            {
                std::vector<std::string> groupV;
                for (auto &group : groupuserVec)
                {
                    nlohmann::json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    std::vector<std::string> userV;
                    for (auto &user : group.getUsers())
                    {
                        nlohmann::json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在,用户存在但是密码错误,登录失败
        nlohmann::json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}

// 处理注册业务 name password
void ChatService::Register(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = userModel_.insert(user);
    if (state)
    {
        // 注册成功
        nlohmann::json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        nlohmann::json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 一对一聊天业务
void ChatService::oneChat(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto iter = userConnMap_.find(toid);
        if (iter != userConnMap_.end())
        {
            // toid在线,转发消息    服务器主动推送消息给toid用户
            iter->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = userModel_.query(toid);
    if (user.getState() == "online")
    {
        redis_.publish(toid, js.dump());
        return;
    }

    // toid不在线,存储离线消息
    offlinemessageModel_.insert(toid, js.dump());
}

// 添加好友
void ChatService::addFriend(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    friendModel_.insert(userid, friendid);
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志,msgtype没有对应的时间处理回调
    auto iter = msgHandlerMap_.find(msgid);
    if (iter == msgHandlerMap_.end())
    {
        return [=](const muduo::net::TcpConnectionPtr &, nlohmann::json &, muduo::Timestamp)
        {
            LOG_ERROR << "MsgType: " << static_cast<int>(msgid) << "can not find handler";
        };
    }
    else
    {
        return msgHandlerMap_[msgid];
    }
}

// 创建群组
void ChatService::createGroup(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    std::string name = js["groupname"];
    std::string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (groupModel_.createGroup(group))
    {
        // 存储群组创建人信息
        groupModel_.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组
void ChatService::addGroup(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupModel_.addGroup(userid, groupid, "normal");
}

// 群组聊天
void ChatService::groupChat(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    std::vector<int> useridVec = groupModel_.queryGroupUsers(userid, groupid);

    std::lock_guard<std::mutex> lock(mtx_);
    for (int id : useridVec)
    {
        auto iter = userConnMap_.find(id);
        if (iter != userConnMap_.end())
        {
            // 转发群消息
            iter->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线
            User user = userModel_.query(id);
            if (user.getState() == "online")
            {
                redis_.publish(id, js.dump());
            }
            else
            {
                // 存储离线消息
                offlinemessageModel_.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::Loginout(const muduo::net::TcpConnectionPtr &conn, nlohmann::json &js, muduo::Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto iter = userConnMap_.find(userid);
        if (iter != userConnMap_.end())
        {
            userConnMap_.erase(iter);
        }
    }

    // 用户注销,相当于就是下线,在redis中取消消息订阅
    redis_.unsubscribe(userid);

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    userModel_.updateState(user);
}

// 处理客户端异常退出
void ChatService::clientCloseException(const muduo::net::TcpConnectionPtr &conn)
{
    User user;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (auto iter = userConnMap_.begin(); iter != userConnMap_.end(); ++iter)
        {
            if (iter->second == conn)
            {
                // 从map表删除用户的连接信息
                user.setId(iter->first);
                userConnMap_.erase(iter);
                break;
            }
        }
    }

    // 用户注销,相当于就是下线,在redis中取消消息订阅
    redis_.unsubscribe(user.getId());

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        userModel_.updateState(user);
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, std::string msg)
{
    nlohmann::json js = nlohmann::json::parse(msg.c_str());

    std::lock_guard<std::mutex> lock(mtx_);
    auto iter = userConnMap_.find(userid);
    if(iter != userConnMap_.end())
    {
        iter->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    offlinemessageModel_.insert(userid, msg);
}

// 服务器异常,业务重置方法
void ChatService::reset()
{
    // 把所有online状态的用户,设置成offline
    userModel_.resetState();
}
#pragma once

#include <string>

// 匹配User表的ORM类
class User
{
public:
    User(int id = -1, std::string name = "", std::string password = "", std::string state = "offline"):id_(id), name_(name), password_(password), state_(state)
    {}

    void setId(int id) { id_ = id; }
    void setName(std::string name) { name_ = name; }
    void setPwd(std::string password) { password_ = password; }
    void setState(std::string state) { state_ = state; }

    int getId() { return id_; }
    std::string getName() { return name_; }
    std::string getPwd() { return password_; }
    std::string getState() { return state_; }
     
private:
    int id_;
    std::string name_;
    std::string password_;
    std::string state_;
};
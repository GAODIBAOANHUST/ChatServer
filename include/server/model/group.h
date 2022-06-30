#pragma once

#include "groupuser.h"

#include <string>
#include <vector>

class Group
{
public:
    Group(int id = -1, std::string name = "", std::string desc = ""):id_(id), name_(name), desc_(desc) {}

    void setId(int id) { this->id_ = id; }
    void setName(std::string name) { this->name_ = name; }
    void setDesc(std::string desc) { this->desc_ = desc; }
    
    int getId() const { return this->id_; }
    std::string getName() const { return this->name_; }
    std::string getDesc() const { return this->desc_; }
    std::vector<GroupUser> getUsers() const { return this->users_; }

private:
    int id_;
    std::string name_;
    std::string desc_;
    std::vector<GroupUser> users_;
};
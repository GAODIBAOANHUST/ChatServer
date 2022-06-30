#pragma once

#include "user.h"

class GroupUser : public User
{
public:
    void setRole(std::string role) { role_ = role; }
    std::string getRole() { return role_; }
    
private:
    std::string role_;
};
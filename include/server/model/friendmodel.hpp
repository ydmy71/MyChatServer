#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include<vector>
#include"user.hpp"
using namespace std;
class FriendModel
{
    public:
    void insert(int userid,int friendid);
    vector<User> query(int userid);
    private:
};
#endif
#ifndef OFFLINEMESSAGEMODE_H
#define OFFLINEMESSAGEMODE_H
#include<string>
#include<iostream>
#include<vector>
using namespace std;


class OfflineMsgModel
{
    public:
    void insert(int userid,string msg);
    void remove(int userid);
    vector<string> query(int userid);
};
#endif
#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<unordered_map>
#include<functional>
#include<muduo/net/TcpConnection.h>
#include"usermodel.hpp"
#include"json.hpp"
#include<mutex>
#include"offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include "groupmodel.hpp"
#include"redis.hpp"
using json=nlohmann::json;
using namespace std;
using namespace muduo;
using namespace muduo::net;
using MsgHandler=std::function<void(const TcpConnectionPtr &conn,json&js,Timestamp)>;
class Chatservice
{
public:
    static Chatservice* instance();
    void login(const TcpConnectionPtr &conn,json&js,Timestamp);
    void reg(const TcpConnectionPtr &conn,json&js,Timestamp);
    void ClientCloseException(const TcpConnectionPtr &conn);
    void oneChat(const TcpConnectionPtr &conn,json&js,Timestamp);
    void addfriend(const TcpConnectionPtr &conn,json&js,Timestamp);
     // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
     void handleRedisSubscribeMessage(int, string);
    void reset();
    //获取消息对应的处理器
    MsgHandler gethandler(int msgid);
private:
    Chatservice();
    unordered_map<int,MsgHandler>_msgHandermap;
    UserModel  _userModel;
    mutex _connMutex;
     unordered_map<int,TcpConnectionPtr>_userconnmap;
     OfflineMsgModel _offlineMsgModel;
     FriendModel _friendmodel;
     GroupModel _groupmodel;
     Redis _redis;
    
};


#endif
#include "chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h>
#include<vector>
#include<iostream>
#include<map>
using namespace std;
Chatservice *Chatservice::instance()
{
    static Chatservice service;
    return &service;
}
Chatservice::Chatservice()
{
   _msgHandermap.insert({LOGIN_MSG,bind(&Chatservice::login,this,_1,_2,_3)});
   _msgHandermap.insert({REG_MSG,bind(&Chatservice::reg,this,_1,_2,_3)});
   _msgHandermap.insert({ONE_CHAT_MSG,bind(&Chatservice::oneChat,this,_1,_2,_3)});
   _msgHandermap.insert({ADD_FRIEND_MSG,bind(&Chatservice::addfriend,this,_1,_2,_3)});
   _msgHandermap.insert({CREATE_GROUP_MSG, std::bind(&Chatservice::createGroup, this, _1, _2, _3)});
   _msgHandermap.insert({ADD_GROUP_MSG, std::bind(&Chatservice::addGroup, this, _1, _2, _3)});
   _msgHandermap.insert({GROUP_CHAT_MSG, std::bind(&Chatservice::groupChat, this, _1, _2, _3)});
   _msgHandermap.insert({LOGINOUT_MSG, std::bind(&Chatservice::loginout, this, _1, _2, _3)});
      if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&Chatservice::handleRedisSubscribeMessage, this, _1, _2));
    }
}
void Chatservice::login(const TcpConnectionPtr &conn,json&js,Timestamp)
{
    int id=js["id"].get<int>();
    string pwd=js["password"];
    User user=_userModel.query(id);
    if(user.getId()==id&&user.getPwd()==pwd)
    {
        if(user.getState()=="online")
        {
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=2;
            response["errmsg"]="该账号已经在线";
            conn->send(response.dump());
        }
        else
        {
            {
            lock_guard<mutex> lock(_connMutex);
            _userconnmap.insert({id,conn});
            }
              _redis.subscribe(id); 

            user.setState("online");
            _userModel.updatestate(user);

            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();
            vector<string> vec=_offlineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlinemsg"]=vec;
                _offlineMsgModel.remove(id);
            }
            vector<User> userVec=_friendmodel.query(id);
            if(!userVec.empty())
            {
                vector<string> vec2;
                for(User &user:userVec)
                {
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"]=vec2;
            }
             vector<Group> groupuserVec = _groupmodel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="用户名或者密码错误";
        conn->send(response.dump());
    }

}
void Chatservice::reg(const TcpConnectionPtr &conn,json&js,Timestamp)
{
    string name=js["name"];
    string pwd=js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state=_userModel.insert(user);
    if(state)
    {
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());

    }
    else
    {
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }
}
//获取消息对应的处理器
MsgHandler Chatservice::gethandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it=_msgHandermap.find(msgid);
    if(it==_msgHandermap.end())
    {
         return [=](const TcpConnectionPtr &conn,json&js,Timestamp){
        LOG_ERROR<<"MSGID:"<<msgid<<"can not find handle";
         };
    }
    else
    {
         return _msgHandermap[msgid];
    }
}
 void Chatservice::ClientCloseException(const TcpConnectionPtr &conn)
 {
        User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it=_userconnmap.begin();it!=_userconnmap.end();++it)
        {
            if(it->second==conn)
            {
                user.setId(it->first);
                _userconnmap.erase(it);
                break;
            }
        }
    }
    _redis.unsubscribe(user.getId()); 
        if(user.getId()!=-1)
        {
            user.setState("offline");
            _userModel.updatestate(user);
        }
 }
  void Chatservice::oneChat(const TcpConnectionPtr &conn,json&js,Timestamp)
  {
      int toid=js["toid"].get<int>();
      {
        lock_guard<mutex>lock(_connMutex);
        auto it=_userconnmap.find(toid);
        if(it!=_userconnmap.end())
        {
           it->second->send(js.dump()); 
           return;
        }
      }
       User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }
      _offlineMsgModel.insert(toid,js.dump());
  }
  void Chatservice ::reset()
  {
    _userModel.resetState();
  }
  void Chatservice::addfriend(const TcpConnectionPtr &conn,json&js,Timestamp)
  {
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();
    _friendmodel.insert(userid,friendid);
  }
  void Chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupmodel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupmodel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组业务
void Chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupmodel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void Chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupmodel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userconnmap.find(id);
        if (it != _userconnmap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
             else
        {
            // 查询toid是否在线 
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}
void Chatservice::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userconnmap.find(userid);
        if (it != _userconnmap.end())
        {
            _userconnmap.erase(it);
        }
    }
    _redis.unsubscribe(userid); 
    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updatestate(user);
}
void Chatservice::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userconnmap.find(userid);
    if (it != _userconnmap.end())
    {
        it->second->send(msg);
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}
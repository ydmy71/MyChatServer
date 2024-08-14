#include"chatserver.hpp"
#include"chatservice.hpp"
#include"json.hpp"
using json=nlohmann::json;
#include<string>
#include<iostream>
#include<functional>
using namespace std;
using namespace placeholders;
 // 初始化TcpServer
ChatServer::ChatServer(EventLoop *loop,
 const InetAddress &listenAddr,
 const string &nameArg)
 :_server(loop, listenAddr, nameArg),_loop(loop)
 {
 // 给服务器注册用户连接的创建和断开回调
 _server.setConnectionCallback(bind(&ChatServer::onConnection,
 this, _1));
 //给服务器注册用户读写事件回调
 _server.setMessageCallback(bind(&ChatServer::onMessage,
 this, _1, _2, _3));
 // 设置EventLoop的线程个数
 _server.setThreadNum(4);
 }
 // 启动ChatServer服务 开始事件循环
 void ChatServer:: start()
 {
    _server.start();
 }
 // TcpServer绑定的回调函数，当有新连接或连接中断时调用
 void ChatServer::onConnection(const muduo::net::TcpConnectionPtr &conn)
 {
    if(!conn->connected())
    {
      Chatservice::instance()->ClientCloseException(conn);
      conn->shutdown();
    }
 }
 // TcpServer绑定的回调函数，当有新数据时调用 专门处理用户的读写事件
 void ChatServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
Buffer *buffer,
Timestamp time )
 {
   string buf=buffer->retrieveAllAsString();
   //数据的反序列化
   json js=json::parse(buf);

   auto msgHandler=Chatservice::instance()->gethandler(js["msgid"].get<int>());
   msgHandler(conn,js,time);
 }
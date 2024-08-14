#ifndef CHATSERVER_H 
#define CHATSERVER_H
#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;
class ChatServer
{
public:
 // 初始化TcpServer
 ChatServer(EventLoop *loop,
 const InetAddress &listenAddr,
 const string &nameArg);
 // 启动ChatServer服务 开始事件循环
 void start();
private:
 // TcpServer绑定的回调函数，当有新连接或连接中断时调用
 void onConnection(const muduo::net::TcpConnectionPtr &);
 // TcpServer绑定的回调函数，当有新数据时调用 专门处理用户的读写事件
 void onMessage(const muduo::net::TcpConnectionPtr &,
 muduo::net::Buffer *buffer,
 muduo::Timestamp time);
 muduo::net::TcpServer _server;
 EventLoop *_loop;
};
#endif
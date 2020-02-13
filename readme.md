# httpd

高性能C++ HTTP服务器

## 背景

这是我在学习网络编程的过程中为加深理解写的一个项目, 用C++实现了一个非阻塞IO网络库, 并在此基础上实现HTTP服务器.

## 开发环境

使用Ubuntu 19.10开发. 安装依赖:

```
apt install cmake make g++ gdb
```

编译

```
./release.sh
```

运行

```
build/bin/httpd data/config/default
```

## 功能

* 实现了对 HTTP 协议的解析, 能够正确响应对静态资源的请求.

* 支持 FastCGI 协议, 能够与 php-fpm 等 FastCGI 进程管理器协同工作, 可以支持 WordPress 等网站后台.

* 实现了一个网络库, 使用Reactor模式, 每个工作线程拥有一个事件循环用于处理各种事件, 使用epoll进行IO复用. 每个连接在主线程accept之后分配到工作线程中.

* 用timerfd计时, 线程间使用eventfd通信. 这两种事件均用epoll统一管理.

* 在用静态资源测试及开启Keep-Alive时, 4线程服务器能够每秒处理27万请求.

## 并发模型

网络库采用Reactor模式, 会启动一个监听线程和若干个工作线程, 以及一些辅助线程(如日志线程). 监听线程用于accept新连接, 把连接分配到工作线程. 

每个工作线程都是一个Reactor, 其核心部分是一个调度器, 库的其他部分都会向这个调度器注册某些事件以及这个事件的回调函数. 调度器通过epoll来得知哪些事件已被触发, 然后调用对应的回调函数.

调度器支持五类事件, 每类事件都由一个类单独管理, 分别是:

* ListenerPool: 负责管理所有处于listen状态的socket. 对外开放的接口有:

```
// 添加一个处于listen状态的socket, 在有连接到来时执行accept得到对应的connection socket, 
// 然后调用指定的onConnection函数.
void add_listener(shared_ptr<Listener> listener, 
    function<void(shared_ptr<Connection>)> onConnection); 
// 移除监听socket.
void remove_listener(shared_ptr<Listener> listener);
```

* ConnectionPool: 负责管理所有活跃的连接. 对外开放的接口有:

```
// 添加一个活跃连接, 并且在发生事件时调用对应的回调函数
void add_connection(shared_ptr<Connection> conn, shared_ptr<ConnectionEvent> event); 
// 其中可用的事件包含
class ConnectionEvent {
public:
    // 连接建立时调用
    function<void(shared_ptr<Connection> conn)> onConnection = 0;
    // 消息到达时调用
    function<void(shared_ptr<Connection> conn, string &message)> onMessage = 0;
    // 发送完成时调用
    function<void(shared_ptr<Connection> conn)> onSendComplete = 0; 
    // 连接关闭时调用
    function<void(shared_ptr<Connection> conn)> onDisconnect = 0; 
};
```

* ConnectorPool: 负责管理主动发起的所有连接, 对外开放的接口有:

```
// 发起对ip:port的连接, 在成功时执行onSuccess函数, 在失败时执行onError函数.
void connect(const string &ip, uint16_t port, 
    function<void(shared_ptr<Connection> conn)> onSuccess, 
    function<void(shared_ptr<Connection> conn)> onError);
```

* EventPool: 负责管理所有需要监听可读事件的文件描述符.

```
// 监听fd, 在其可读时调用op
void add_event(int fd, function<void()> op);
// 不再监听fd
void del_event(int fd);
```

* TimerPool: 负责管理计时.

```
// 在一段时间后执行op, 返回计时器ID
int set_timeout(function<void(void)> op, double seconds);
// 设置某个连接在一段时间中没有活动则断开连接
void set_deactivation(shared_ptr<Connection> conn, int seconds);
// 取消计时器
void cancel(int id);
```

每个工作线程都有一个待处理队列, 用于存放还没有注册到调度器的连接. 监听线程在accept连接后把连接放到一个工作线程的待处理队列中, 然后通过eventfd的形式通知工作线程. 每个工作线程都会默认监听一个eventfd的可读事件用于接收通知.

此外, 网络库还可以工作在两种其他模式下, 分别是单线程模式, 一个Reactor同时处理listen socket和connection socket; 以及多进程模式, 监听进程通过unix domain socket把连接传递给工作进程.

## HTTP

### FastCGI

此服务器实现了FastCGI协议, 在URI符合一定要求时, 建立一个到指定FastCGI server的连接, 然后把HTTP的header和body转换为FastCGI对应的模式发送给过去, 等待FastCGI的响应. 在响应结束后, 把数据转发到客户端. 

在php-fpm以及mysql的配合下, 服务器可以支持像WordPress这样的网站. 

### 不活跃连接的管理

定时器采用`map<time_t, vector<weak_ptr<Connection>>>`这一结构来管理连接的失效时间, 其中key是时间, value是在此时间应该失效的连接. 每秒钟检查一次此数据结构, 如果`map`中的某个key小于当前时间, 且对应的某个连接的最后活跃时间等于这个key, 则断开此连接. (每个连接都记录最后的读写时间).

### 性能测试

环境: 阿里云ECS, Ubuntu 18.04, 四线程, CPU为Intel Xeon(Cascade Lake) Platinum 8269CY.

工具: 使用[Webbench](https://github.com/linyacool/WebBench), 启动1000个客户端进程，持续60s, 并分别测试开启Keep-Alive和不开启时的性能.

其他设置: 关闭log.

性能对比:

|(QPS)|Muduo Echo|HTTPD Echo|HTTPD|
|---|---|---|---|
|短连接|76378|94507|93586|
|长连接|338349|310367|272060|

其中, Muduo Echo是指使用Muduo网络库自带的Echo测试得到的结果. HTTPD Echo是指关闭服务器对HTTP的解析, 直接返回客户端发送的结果. HTTPD是指解析HTTP请求, 然后返回一个固定的字符串. 

长连接性能略低于Muduo网络库, 短连接性能略高于Muduo网络库.

## 日志


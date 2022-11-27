[![C++20](https://img.shields.io/badge/language-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)
![compiler](https://img.shields.io/badge/compiler-gcc-red)
[![license](https://img.shields.io/badge/license-GPL--3.0-green)](https://github.com/wythers/ioCoro/blob/main/LICENSE)
![platform Linux 64-bit](https://img.shields.io/badge/platform-Linux%2064--bit-yellow)
![Latest release](https://img.shields.io/badge/latest%20release-2.o-lightgrey)
![Io](https://img.shields.io/badge/io-async-yellow)
![Event](https://img.shields.io/badge/event%20processing%20mode-proactor-orange)  
![This is an image](./images/hello-ioCoro.png)  

---

 
# ioCoro - be elegant, be efficient :stuck_out_tongue_winking_eye:  

> Making things simple but simpler is hard.

## 导言  
**当引入一种新的编程语言或框架时，以`Hello，world`程序作为第一个示例已成为广泛接受的计算机编程传统，我很高兴遵循这种传统，来介绍`ioCoro`--异步IO服务框架。在本节中，你将跟随我一起来编写一个简单的ECHO网络服务程序。**  
  
### 1. 明确ECHO服务需求
 首先，我们必须解释我们的ECHO服务的运行机制:  
 - echo客户端将大写字符串（“HELLO，IOCORO！\n”）发送到echo服务器。  
 - 然后服务器将字符串转换为小写（“hello，iocoro！\n”），并将其发回。  
 - 最后，客户端在终端上显示刚刚从服务器获得的小写字符串。  
 
嗯，当我们明确了我们的需求后，开始实现我们的ECHO服务。  
  
### 2. 定义服务 
 我们可以定义我们的ECHO服务，像这样:  
 ```c++
 struct Echo
 {
 
 };
 ```  
有一个问题是，如何让ioCoro知道我们的服务，或者更直接地说如何让ioCoro调用我们的服务代码。答案是特殊的接口，无论是以静态（ioCoro属于）还是动态方式实现的接口。 
  
ioCoro有两个接口，`Active`用于客户端，`Passive`用于服务端，那么现在我们的服务类变成了这样:  
```c++
struct Echo
{
    // the ioCoro entry of client end
    static IoCoro Active(Stream stream, char const* host, int id)
    {
    ...
    }

    // the ioCoro entry of server end
    static IoCoro Passive(Stream streaming)
    {
    ...
    }
};
```  
到这里，我知道你有一些困惑，比如`Stream` 和 `IoCoro`是什么东西？ 嗯。。耐心点。  
为了回答你的困惑，我必须引入一个ioCoro的设计理念:

> Every Stream(socket) is distributed from the ioCoro, the user only uses the Stream instead of establishing or possess.  
> 每一个Stream都只由ioCoro分配，且用户(你)只能使用它，而不能拥有它(释放)或者创建它。

因此，参数`Stream`是留给ioCoro的插槽。如果你要求一个`Stream(Socket)`，那么就留下一个插槽、两个、两个插槽等等，像下面这样:
```c++
  static IoCoro Active(Stream s1, Stream s2, Stream s3, ...);
  static IoCoro Passive(Stream s1ing, Stream s2, Stream s3, stream s4, ...);
```  
但唯一的限制是`Stream`必须位于用户定义的参数之前。另一个问题是`IoCoro`是什么东西？  
嗯。。`IoCoro`不是一个用户(你)直接面对的组件，作为一个普通用户，不用在意它是什么，只需要保留它，留下血淋淋的细节由ioCoro来实现。  
  
最后，如果你足够细心，你会发现两个接口中的参数名称不同，一个是`stream`，另一个是`streaming`，因为`stream`是`主动`的自由流，而`streaming`则是`被动`的完成流。自由流意味着它可以连接任何对等体。完成的流则是已经链接完毕的确定流。  
  
接下来，我们将使用`ioCoroSysCall`(ioCoro系统调用)来具体的实现这两个接口，让我们开始吧！   

### 3. 应用两个基础组件    
在实现基本逻辑之前，有必要介绍ioCoro提供的两个特殊组件，它们是:  
- `ioCoro::unique_stream`
- `ioCoro::Deadline`

如果你使用过`std::unique_ptr`或`std::unique_lock`，你应该会熟悉`ioCoro::unique_stream`。正如你所猜测的，他的角色是自动执行用户定义的尾部任务，并通知ioCoro准备回收一个旧的`Stream`。      
  
而`ioCoro::Deadline`则是为了满足这样一种需求: 有时我们需要一种机制，来确保某一段代码在确定的时间内被执行完毕，否则将触发用户定义的守护任务。   
  
不用多谈，让我们直观的看一看:  
```c++
struct Echo
{

static constexpr auto DefualtMaxResponseTime = 2s;
         
    static IoCoro Active(Stream stream, char const* host, int id)
    {
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        // 保证本地的Stream流会被回收
        unique_stream cleanup([&]{
              printf("ECHO-REQUEST #%d has completed.\n", id);
        }, stream);

        // ensure the block will be passed within the maximum time frame
        // 确保下面的代码块在默认的最大响应时间(DefualtMaxResponseTime)内被执行完毕,否则关闭该Stream流
        {
            DeadLine line([&]{
                    stream.Close();
            }, stream, DefualtMaxResponseTime);

            // logic implementation
            // 逻辑实现
            ...
        }
    }
    
    static IoCoro Passive(Stream streaming)
    {
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        // 保证本地的Stream流会被回收
        unique_stream cleanup([]{
              printf("An ECHO-request just completed\n");
        }, streaming);

        // ensure the block will be passed within the maximum time frame
        // 确保下面的代码块在默认的最大响应时间(DefualtMaxResponseTime)内被执行完毕,否则关闭该Stream流
        {
            DeadLine line([&]{
                    streaming.Close();
            }, streaming, DefualtMaxResponseTime);

            // logic implementation
            // 逻辑实现
            ...
        }
    }
};
```  
好，终于到了完成我们的ECHO服务的最后一步，继续前进。 
  
### 4. 使用ioCoroSyscall实现基本逻辑

ioCoro是一个异步IO框架，它当然提供一些与IO相关的操作。最基本的三个是： 
- `ioCoroConnect(stream, host)`  
- `ioCoroRead(stream, buf, num)`  
- `ioCoroWrite(stream, buf, num)`  
  

这并不奇怪，但他们的调用方法有点不寻常。如果你想使用它们，你必须这样做:   
```c++
    ssize_t ret = co_await ioCoroRead(stream, buf, num);
    ssize_t ret = co_await ioCoroWrite(stream, buf, num);  
```  
  
我知道你又有了一些困惑。好吧，我简单地解释一下为什么要这么做。ioCoro提供的IO相关操作被设计为`ioCoroSyscall`(ioCoro系统调用)，因此每个IO相关操作都需要从用户上下文切换到ioCoro上下文，而`co_await`扮演一把特殊的钥匙。正如我之前所说，你只需要在使用中保留它，不必知道它是什么。  

在完成我们的ECHO服务类的剩余部分之前，同样地，我们需要介绍一个ioCoro设计思想:  

> ioCoro does not handle errors for users, but only reflects errors.  
> ioCoro不为用户(你)解决错误，而仅仅反映错误。
  
反映而非解决意味着，作为用户，必须在每个IO相关操作完成后检查流状态，然后提供错误的处理代码。 
  
一切就绪，我们现在可以完成我们的ECHO服务的剩余部分了。整个ECHO服务类如下:  
```c++
struct Echo
{

static constexpr auto DefualtMaxResponseTime = 2s;

    static IoCoro Active(Stream stream, char const* host, int id)
    {
        unique_stream cleanup([&]{
          if (!stream)
            printf("ECHO-REQUEST #%d has completed.\n", id);
          else
            fprintf(stderr, 
                    "ECHO-REQUEST #%d failed, ERROR CODE:%d, ERROR MESSAGE:%s.\n",
                    id,
                    stream.StateCode(),
                    stream.ErrorMessage().data());
        }, stream);

        {
            DeadLine line([&]{
                    stream.Close();
            }, stream, DefualtMaxResponseTime);

            // try to connect the server
            // 尝试链接服务器
            co_await ioCoroConnect(stream, host);
            if (stream)
              co_return;

            char const* uppercases = "HELLO, IOCORO!\n";

            // try to send the Uppercase string to the server 
            // and then shutdown the Write stream
            // 尝试发送大写字符串给服务器，而后关闭发送流
            co_await ioCoroCompletedWrite(stream, uppercases, strlen(uppercases));
            if (stream)
              co_return;

            char lowercase[32]{};

            // try to get the lowercase string sent back from the server 
            // and and then shutdown the Read stream
            // 尝试获取服务器处理后的小写字符串，而后关闭接受流
            co_await ioCoroCompletedRead(stream, lowercase, sizeof(lowercase));
            if (stream)
              co_return;

            // display the lowercase string on the terminal
            // 显示获取的小写字符串
            printf("%s", lowercase);
        }

        co_return;
    }
    
    static IoCoro Passive(Stream streaming)
    {
        unique_stream cleanup([]{
               printf("An ECHO-request just completed\n");
        }, streaming);

        {
            DeadLine line([&]{
                    streaming.Close();
            }, streaming, DefualtMaxResponseTime);

            char lowercases[32]{};

            // try to get the Uppercase string sent from the client
            // and then shutdown the Read stream
            // 尝试获取客服端发送的大写字符串，而后关闭接受流
            ssize_t ret = co_await ioCoroCompletedRead(streaming, lowercases, sizeof(lowercases));
            if (streaming)
              co_return;

            // translate the Uppercase string just obtained from the client to the lowercase
            // 转换大写字符串为小写字符串
            to_lowercases(lowercases, lowercases, ret);

            // try back to send the lowercase string just translated to the client
            // and then shutdown the Write stream
            // 尝试发送处理后的大写字符串给客服端，而后关闭发送流
            co_await ioCoroCompletedWrite(streaming, lowercases, ret);
            if (streaming)
              co_return;
        }
        
        co_return;
    }
};
``` 
  
到现在，作为用户，我们已经完美地履行了我们的责任--实现了我们的ECHO服务类。现在是时候驱动ioCoro履行它的职责了。  
  
### 5. 驱动ioCoro维护ECHO服务   
  
驱动ioCoro去维护我们的服务非常非常容易。你可以这样做:  
  
- For client,  
```c++
int main()
{
    // loads our service at client end
    // 装载我们的服务
    ioCoro::Client<Echo> client{};
    
    // 100'000 connections at a 10'000/sec rate, as a small case. 
    // On my computer, with 4 threads, a 30'000/sec rate,
    // and 2s of maximum response time, it is okay.
    // Why do you not try to run it on your computer?
    // 100'000 个链接，以每秒10'000个，作为一个小case。
    // 在我的电脑上，4线程，最大响应时间2秒，每秒30’000个链接，运行良好。
    // 为什么不在你的电脑上试一试呢？
    for (int i = 0; i < 100000;)
    {
        for (int j = 0; j < 10000; ++j)
        {
            // Does remember the two parameters we defined at Client Entry?
            // 还记得我们在客户端接口上定义的两个参数吗(host 和 id)， right？
            // static IoCoro Active(Stream stream, char const* host, int id)
            client.Submit("localhost:1024", ++i);
        }
        sleep(1);
    }   
    // similar to std::thread::join()
    // 类似std::thread::join()
    client.Join();
}
```  
- And server,  
```c++
int main()
{
    // loads our service at server end
    // 装载我们的服务
    ioCoro::Server<Echo> server{":1024"};
    
    // let ioCoro maintain our Echo service right now
    // 让ioCoro从现在开始维护我们的服务
    server.Run();
}
```  
无论是在现实世界还是计算机世界，如果每个人都像我们刚才那样承担起自己的责任，世界会变得更好，对吗？  

### 6. 总结 
在本节中，我们逐步实现了我们的ECHO服务。我相信你对ioCoro有了基本的了解，这正是我所希望的。出于可读性和空间限制的原因，上面只显示了源代码的关键部分。要查看完整的源代码，构建并运行它，请单击[此处](./example/echo)。   
  
## 进阶  
  
### 构建
1. 你必须在Linux环境中，然后你的`GCC`编译器必须支持c++20（-fcoroutines）和c++concept。 
2. 你可以按如下方式构建libiocoro:  
```bash
foo@bar:~$ git clone https://github.com/wythers/ioCoro.git
foo@bar:~$ cd ioCoro && ./install.sh
```  
3. include 和 linker
```c++
#include <iocoro/iocoro.hpp>
```
```shell
foo@bar:~$ g++ ... -liocoro
```  

### 定时器 
在这里，我想介绍ioCoro的第三个设计理念:  
  
> Provides requirements, but does not force use.  
> 提供可能需要的机制，但不强制使用

使用`Timer`将影响性能。有些用户需要这个机制，有些则不需要，所以ioCoro的选择是提供模块化的定时器。如果定义`NEED_IOCORO_TIMER`宏，则计时器模块将在不需要重新编译`libiocoro`的情况下发挥作用，反之亦然。 
  
同样，第四个设计理念:   
  
> Make user should do, but must do.  
> 使用户应该做，而非必须做
   
默认情况下，`ioCoro:：Timer`总是在用户上下文中耦合`Stream`，如下所示：
```c++
    ioCoro::Timer tm([]{
        ....
    }, stream);
```  
为什么？因为ioCoro不知道用户的`Timer`是否引用了协程局部变量，为了安全起见，它默认选择了耦合。如果ioCoro没有提供解耦的方法，那么就是must-do，而非should-do。方法是:
```c++
    ioCoro::Timer::Detach();
```  
一旦调用此成员方法，用户将承担起相应的责任。  
  
### 异常  

ioCoro仅在发生致命错误时抛出异常，通常是在ioCoro上下文的初始化阶段，或者当Linux运行环境发生变化时，例如防火墙启用禁止等。 

### 效率
效率是ioCoro的设计目标之一，很大程度取决于GCC的`cpp20coroutine`性能，如果遵循以下方法，ioCoro实现不会成为性能瓶颈:  

* 给出一个线索，让`ioCoro服务端`对即将到来的负载进行初步估计，你可以这样做：
```c++
    ioCoro::Server<service> server{...};
    // assume that more than 10000 streams will arrive
    // 假设将会有超过10000个链接需求到来
    server.Reserver(10000);
    server.Run();
```  
* 尽量在ioCoroEntry（协程上下文）中声明变量，即使它是大的数据块。由协程创建时一次性分配比在协程上下文中进行散碎分配要好的多。同时，也避免了频繁的`delete`。
  
* 如果你不需要`Timer`，则不应该定义`NEED_IOCORO_TIMER`，此时，ioCoro几乎是一个无锁程序，这非常令人兴奋。  
  
* 充分利用`ioCoroSyscall`的返回值来达到一次遍历而获得足够的数据信息。例如：  
```c++
    // the pos is where the delim("\r\n\r\n") first appeared in the buf
    // pos是字符串"\r\n\r\n"第一次在buf中出现的位置
    auto [num, pos] = co_await ioCoroReadUntil(stream, buf, num, "\r\n\r\n");
```  
  
* 正确的`Woker`数量也是关键，这需要用户根据服务逐步地调整，`ioCoro`提供`THREADS_NUM`宏，或传递一个uint参数来控制Worker数量，像这样：  
```c++
    // ...
    ioCoro::Server<service> server{host, threads_num};
    //...
    ioCoro::Client<service> client{threads_num};
```
默认值是平台CPU的核心数乘以2，这可能是一个不错的选择。  
  
### Flexibility  
  
作为一个用CPP实现的框架，当然不能绕过灵活性，ioCoro也有很好的灵活性。我相信你已经可以感受到这一点，此外，ioCoro允许你自己实现`ioCoroSyscall`，这是一个非常有趣的事，我迫不及待地想在另一个更高级的介绍中与你一起实现定制的`ioCoroSyscall`，该介绍将在[此处](./advance)。 
  
## Final
**Needless to say, please allow me to repeat:**  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;-*be elegant, be efficient.:love_you_gesture:*  
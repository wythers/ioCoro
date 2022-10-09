[![C++20](https://img.shields.io/badge/language-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)
![compiler](https://img.shields.io/badge/compiler-gcc-red)
[![license](https://img.shields.io/badge/license-GPL--3.0-green)](https://github.com/wythers/ioCoro/blob/main/LICENSE)
![platform Linux 64-bit](https://img.shields.io/badge/platform-Linux%2064--bit-yellow)
![Latest release](https://img.shields.io/badge/latest%20release-2.o-lightgrey)
![Io](https://img.shields.io/badge/io-async-yellow)
![Event](https://img.shields.io/badge/event%20processing%20mode-proactor-orange)  

---

![This is an image](./images/hello-ioCoro.png)  

---

 
# ioCoro - be elegant, be efficient :stuck_out_tongue_winking_eye:  

> Making things simple but simpler is hard.

## Tutorial  
**When introducing a new programming language or framework, it has become a widely accepted computer programming tradition to mimic the original
`Hello, world` program as the very first piece of code. I'm happy to follow this venerated tradition to introduce the powerful async-IO framework. In this section, you will learn the steps to code a simple IO-related ECHO-service. I explain the framework in advanced detail after the section.**  

 ### 1. Clarify the mechanism  
 First, we must explain the running mechanism of our ECHO-service, so there is the mechanism:  
 - The echo-client sends the Uppercase string("HELLO, IOCORO!\n") to the echo-server.
 - And then the server translates the string to the Lowercase("hello, iocoro!\n"), and sends it back.
 - At final the client displays the Lowercases just obtained from the server on the terminal.  
 
 Em..., It is time to implement our ECHO service when we clear on the design idea.  
 ### 2. Define our service  
 We can define our ECHO service like this:
 ```c++
 struct Echo
 {
 
 };
 ```  
Is that enough? No, no, no, if that's enough, it's magic, it's not a framework. Getting down to business, there is a question that how to make the ioCoro know our service or to more straightly say how to make the ioCoro invoke our codes. the answer is the special interface, the entry whatever it was implemented in a static(the ioCoro's entry be) or dynamic way.  
  
  
The ioCoro has two interfaces, the entries that the `Active` for Client end, `Passive` for Server, so now our service class is like this:
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
What does the `Stream` mean and `IoCoro`? Em... I know you have many questions, but be patient.  
  

For answering your questions, it must be known that one of the ioCoro design ideas:

> Every Stream(socket) is distributed from the ioCoro, the user only uses the Stream instead of establishing or possess.

So, the parameter `Stream` is a slot to be left for the ioCoro. If you ask for one Stream(Socket), just leave the one parameter, two, two-parameter and so on, like this:
```c++
  static IoCoro Active(Stream s1, Stream s2, Stream s3, ...);
  static IoCoro Passive(Stream s1ing, Stream s2, Stream s3, stream s4, ...);
```  
But the only restriction here is that the `Stream` parameters are in front of user-defined parameters. that's all for the question about `Stream`, and the other question is what does the `IoCoro` mean?  
Em... the story is that the `IoCoro` is not a user business or more straightly say not a normal user business. As a normal user, don't care what it is, just keep it,  leave the gory details to be implemented by the ioCoro, and then taste coffee, oh... a beautiful day!  
  
Finally, if you are careful enough, you will find the arg name is different in both interfaces, the one of `stream`, the other of `streaming` because the `stream` is a free stream in `Active`, and the `streaming` is the finished stream in `Passive`.  a free stream means it can connect any peer.  the finished streaming is fixed to the peer.
  
Next, we will implement both interfaces(entries) with ioCoroSysCall, let's go!  


### 3. Apply two basic components   
It is valuable to introduce two special and tricky components provided by ioCoro before implementing the basic logic, they are:
- `ioCoro::unique_stream`
- `ioCoro::Deadline`

If you have used `std::unique_ptr` or `std::unique_lock`, you should be familiar with `ioCoro::unique_stream`. As you guessed, his role is automating to perform the user-defined tail task and notify ioCoro that a guy will be back.  
  
And the `ioCoro::Deadline` is a little tricky, sometimes we need such a mechanism that draws a deadline to ensure some codes will be passed on time,  or the deadline is triggered to perform some tail tasks.  
  
Without further discussion, let's apply them to our service. our service class is right now like this:
```c++
struct Echo
{

static constexpr auto DefualtMaxResponseTime = 2s;
         
    static IoCoro Active(Stream stream, char const* host, int id)
    {
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        unique_stream cleanup([&]{
              printf("ECHO-REQUEST #%d has completed.\n", id);
        }, stream);

        // ensure the block will be passed within the maximum time frame
        {
            DeadLine line([&]{
                    stream.Close();
            }, stream, DefualtMaxResponseTime);

            // logic implementation
            ...
        }
    }
    
    static IoCoro Passive(Stream streaming)
    {
        // guarantees the stream(socket) reclaimed by the ioCoro-context
        unique_stream cleanup([]{
              printf("An ECHO-request just completed\n");
        }, streaming);

        // ensure the block will be passed within the maximum time frame
        {
            DeadLine line([&]{
                    streaming.Close();
            }, streaming, DefualtMaxResponseTime);

            // logic implementation
            ...
        }
    }
};
```  
Well, we are finally coming to the last step to completing our service, keep going guys.  
  
### 4. Implement the basic logic with ioCoroSyscall

ioCoro is an async-IO framework it provides some IO-related operations. The most basic three are:  
- `ioCoroConnect(stream, host)`  
- `ioCoroRead(stream, buf, num)`  
- `ioCoroWrite(stream, buf, num)`  
  

It is not surprising, but their calling method is a little unusual. If you want call them, you must do like this:  
```c++
    ssize_t ret = co_await ioCoroRead(stream, buf, num);
    ssize_t ret = co_await ioCoroWrite(stream, buf, num);  
```  
  
I know you raising a lot of questions. Ok... I am trying to simply explain why to do this. the story is that IO-related operations provided by ioCoro are designed to be ioCoro Syscall, so every IO-related Operation needs the Context-switch from User-context to ioCoro-context, and `co_await` roles the special key. As i said before, just keep it if you want to do, and don't care what it is.

Before complete the rest of our service class, similarly here we need to introduce one of the ioCoro design ideas:

> ioCoro does not handle errors for users, but only reflects errors.  
  
Reflection instead of handle means you, as a user, must check the stream status after every IO-related operation is completed, and then provide the error handle.  
  

Everything is ready，we can now complete the rest of our ECHO service. so the completed service class is like this:
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
            co_await ioCoroConnect(stream, host);
            if (stream)
              co_return;

            char const* uppercases = "HELLO, IOCORO!\n";

            // try to send the Uppercase string to the server 
            // and then shutdown the Write stream
            co_await ioCoroCompletedWrite(stream, uppercases, strlen(uppercases));
            if (stream)
              co_return;

            char lowercase[32]{};

            // try to get the lowercase string sent back from the server 
            // and and then shutdown the Read stream
            co_await ioCoroCompletedRead(stream, lowercase, sizeof(lowercase));
            if (stream)
              co_return;

            // display the lowercase string on the terminal
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
            ssize_t ret = co_await ioCoroCompletedRead(streaming, lowercases, sizeof(lowercases));
            if (streaming)
              co_return;

            // translate the Uppercase string just obtained from the client to the lowercase
            to_lowercases(lowercases, lowercases, ret);

            // try back to send the lowercase string just translated to the client
            // and then shutdown the Write stream
            co_await ioCoroCompletedWrite(streaming, lowercases, ret);
            if (streaming)
              co_return;
        }
        
        co_return;
    }
};
``` 
  
Until now, we, as users, have fulfilled our responsibilities perfectly to implement our ECHO-service class. It's time to drive ioCoro to fulfill its responsibilities.  
  
### 5. Load our service in the ioCoro framework  
  
Driving ioCoro to maintain our service is very, very easy. You can do like this:  
  
- For client,  
```c++
int main()
{
    // loads our service at client end
    ioCoro::Client<Echo> client{};
    
    // Does remember the two parameters we defined at Client Entry?
    client.Submit("localhost:1024", 1);
   
    // similar to std::thread::join(),
    client.Join();
}
```  
- And Server,  
```c++
int main()
{
    // loads our service at server end
    Server<Echo> server{":1024"};
    
    // let ioCoro maintain our Echo service right now
    server.Run();
}
```  
No matter in the real world or the computer world, the world will be better if everyone takes their own responsibility, as we just did, right? 

### 6. Summary  
In this section, we meet the target of implementing the simple ECHO service step by step. I believe you have a basic understanding of ioCoro, which is exactly what I hope. For reasons of readability and space constraints, only the key parts of the source code are displayed above. To view the complete source code, build it, and run it, please click [here](./example/echo).

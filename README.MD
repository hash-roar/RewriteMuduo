## NetMuduo

this library mainly derive from chenshuo's Muduo c++ library (which is graceful and inspirational in c++ network programing).

I strongly recommend hacking the library throughly and read the book-- . The core code is pithy and and the comments in the book make a pointed comment on what is good  about the philosophy of c++ concurrency programing and the subtle neglected  concepts about linux system call api,c++ concurrency library,software engineering,and so forth all of which sometimes should be recognized high importance  of.  



### TODO

Although with crisp fabric as well as well-designed structure,the library is not as user-friendly as Asio(another c++ network).If time permits,I will devote myself into the enhancement of this library.



#### IO

The library is canonical Reactor library. So all the io operations are synchronous including file io and signal.

One improvement  is io thread pool and task queue(with std::mutex or atomic variable) to work around the 

lack of asynchronous io in linux;

Another underlying choice is to use io_uring,which is an asynchronous io framework  introduced by kernel 5.4

#### Feature

A timer for every connection . The mechanism provided by chenshuo.We maitain a list with all connection pointer,check time validation every second

A signal handler 

Wrap it in a SEDA stage for a better development with system 

A more delicate Context State machine but not a simple std::any



#### Test

add more test with googletest especially  in  file io and self wrapped thread.


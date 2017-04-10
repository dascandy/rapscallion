# rapscallion

[![Linux and OSX build status](https://travis-ci.org/dascandy/rapscallion.svg)](https://travis-ci.org/dascandy/rapscallion)

C++ RPC using C++17 futures

# Why?
Most existing RPC mechanisms have two modes to operate in. Either you have no return path for replies (asynchronous), or you do have one and you block the receiving end until it has received everything (synchronous). Both of these have major downsides. 

Asynchronous means that you have to match up whatever data you get back yourselves. For one-way unreliable communication this is fine up to a point, but it becomes hard to manage for any kind of reliability.

Synchronous is better, but looks very close to in-language constructs and has the danger that people do not realize that such an RPC call can take a long time or may disconnect. Handling disconnections is an even bigger topic, where exceptions are probably the best solution but that again implies that you may get exceptions from functions that would otherwise not throw exceptions - a clear difference between having a local or a remote connection.

Rapscallion takes the technical solution of using futures, which is a way to run tasks "asynchronously", and applies it to remote execution of function calls - remote procedure calls. Any future can throw an exception, and the path for them is fully defined. Using the C++17 proposal for combining futures it is possible to program a distributed application fully asynchronously, but with continuations.

In other words, it's like the RPC you knew, except without the trouble.

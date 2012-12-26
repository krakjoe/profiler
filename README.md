profiler
========

An extension to allow a basic profile view of your PHP scripts, during or after runtime. The aim of the profiler is to have as little impact on your application as possible.
The callgrind it produces can help you to find bottlenecks in your code and identify functions or methods that could use optimization.

Features
========

* Profile all userland and internal function and method calls.
* Detect memory, and CPU usage for each and every call while the profiler is enabled.
* Enable/Disable profiler during runtime, allowing you to profile an entire request or just a block of code during a request.
* Profile production applications with little to no impact on speed and performance.

How ?
=====

profiler overrides the default execution functions of the Zend Engine (zend_execute and zend_execute_internal).
This means profiler is executed every time a userland or internal function is called, allowing profiler to trace every function call, recording memory and cpu usage. During execution of your script, NO allocations take place. Meaning your code runs at very close to full speed, while still retaining the ability to trace every call and not resort to sampling. By default, a maximum of 1000 frames will be recorded ( the first 1000 calls ), if you're application requires more than this you can configure a higher limit at build time if you configure with --with-profiler-max-frames=newlimit:

```./configure --enable-profiler --with-profiler-max-frames=2000```

would yield a profiler able to record for 2000 calls. Having a hard limit means that during execution we are just shifting pointers around and not allocating additional memory.

How To ?
========

```
void profiler_enable();
```

You must enable the profiler before it will start to record data. A call to profiler_enable while the profiler is enabled will result in an E_WARNING being raised.
You can enable the profiler by default with INI settings, see below.

```
void profiler_output(string $path);
```

You should set the output to something unique, if you are using pthreads, then the thread id should be included in the filename, else use a mixture of session/path/process id.
The profiler does not care if the file it writes to contains a profile, the contents will be truncated on every write. If the output is not set explicitly at some point during execution, the default path of /tmp/profile.callgrind will be written too.

```
void profiler_disable();
```

Will stop recording information, does not destroy any previously recorded information. A call to profiler_disable while the profiler is not enabled will result in an E_WARNING being raised.

INI Settings
============

```
profiler.enabled=0;
```

By default the profiler must be explicitly enabled in code with profiler_enable. If you wish to automatically profile every call during execution set profiler.enabled=1 in your system php.ini

```
profiler.memory=1;
```

By default, the profiler will record the impact a function call has on the amount of memory allocated by the Zend engine. If you wish to omit recording and outputting this information set profiler.memory=0 in your system php.ini

Callgrind Screenshots
=====================

profiler output is in the defacto callgrind format, which you can inspect using your favourite grinder ...

![alt text](https://github.com/krakjoe/profiler/raw/master/screenshots/kcachegrind-memory.png "Memory Profile View")
![alt text](https://github.com/krakjoe/profiler/raw/master/screenshots/kcachegrind-cpu.png "CPU Profile View")

Callgrind Notes
===============

profiler does not bother to generate a call tree, a decent callgrind viewer will still show detailed information with annotated source code, but the idea is to generate an overhead free summary of resource usage, rather than a full call tree - which I haven't yet found a way to do without incurring allocation and overhead.

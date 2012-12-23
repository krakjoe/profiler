profiler
========

An extension to profile PHP.

A simple set of functions to allow a basic profile view of your PHP scripts, during or after runtime.

Features
========

Profile all userland and internal function and method calls.
Detect overhead in memory and CPU time for each and every call while the profiler is enabled.
Fetch detailed associative output from the profiler at any time during execution.
Enable/Disable profiler during runtime.

Howto
=====

proto void profiler_enable();

You must enable the profiler before it will start to record data. A call to profiler_enable while the profiler is enabled will result in an E_WARNING being raised.

proto void profiler_fetch();

Will fetch details associative information about the current profiling session.

proto void profiler_disable();

Will stop recording information, does not destroy any previously recorded information.

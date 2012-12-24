profiler
========

An extension to allow a basic profile view of your PHP scripts, during or after runtime.

Features
========

* Profile all userland and internal function and method calls.
* Detect memory usage, time, and CPU time for each and every call while the profiler is enabled.
* Fetch detailed associative output from the profiler at any time during execution.
* Enable/Disable profiler during runtime.
* Control what is included in the profile for each function call via INI settings

Howto
=====

```
void profiler_enable();
```

You must enable the profiler before it will start to record data. A call to profiler_enable while the profiler is enabled will result in an E_WARNING being raised.

```
void profiler_fetch();
```

Will fetch detailed associative information about the current profiling session.

```
void profiler_clear();
```

Will clear profiler data, ready for another session, does not disable or enable the profiler.

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
profiler.timing=1;
```

By default, the profiler will record the unix time on entry and exit from every profiled function call. If you wish to omit recording and outputting detailed timing information set profiler.timing=0 in your system php.ini

```
profiler.memory=1;
```

By default, the profiler will record the impact a function call has on the amount of memory allocated by the Zend engine. If you wish to omit recording and outputting this information set profiler.memory=0 in your system php.ini


Example Output
==============

```
[...] => Array
(
    [type] => 1
    [timing] => Array
        (
            [entered] => Array
                (
                    [tv_sec] => 1356280086
                    [tv_usec] => 700295
                )

            [left] => Array
                (
                    [tv_sec] => 1356280086
                    [tv_usec] => 910377
                )

        )

    [location] => Array
        (
            [file] => /usr/src/php-5.4.8/ext/profiler/test.php
            [line] => 23
        )

    [call] => Array
        (
            [function] => my_test_method
            [scope] => my_test_class
            [memory] => 1784
			[cpu] => 124964
        )

)
```

Notes
=====
There is negligable overhead not accounted for with each element in the profile data - the overhead of a call is not adjusted to reflect this.

Giving direct access to the information at profile_fetch() time allows you to profile production applications with minimal overhead, during recording the overhead of the profiler should be negligable in most cases.

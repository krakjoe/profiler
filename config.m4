dnl $Id$
dnl config.m4 for extension profiler
PHP_ARG_ENABLE(profiler, whether to enable profiler support,
[  --enable-profiler           Enable profiler support])

PHP_ARG_WITH(profiler-max-frames, frame limit adjustment,
[  --with-profiler-max-frames  Adjust the maximum frame limit for profiler], 1000, 1000)

if test "$PHP_PROFILER" != "no"; then
  PHP_NEW_EXTENSION(profiler, php_profiler.c, $ext_shared)
  if test "$PHP_PROFILER_MAX_FRAMES" != "1000"; then
    AC_DEFINE_UNQUOTED(PROFILER_MAX_FRAMES, [$PHP_PROFILER_MAX_FRAMES], [maximum frames available to profiler])
  fi
fi

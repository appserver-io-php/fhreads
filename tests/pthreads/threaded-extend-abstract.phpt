--TEST--
Test extending abstract
--XFAIL--
Threaded::extend function not implemented yet.
--DESCRIPTION--
This is regression test for #409
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

var_dump(Threaded::extend(ReflectionFunctionAbstract::class));
?>
--EXPECT--
bool(true)

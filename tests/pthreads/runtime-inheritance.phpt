--TEST--
Test runtime extension
--XFAIL--
Threaded::extend function not implemented yet.
--DESCRIPTION--
This test verifies functionality of runtime extension
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Other {}

class Test extends Other {
    public function one() {}
}

/* force zend to declare Other extends Threaded */
Threaded::extend("Other");

$test = new Test();

var_dump($test instanceof Threaded,
         $test instanceof Other);
?>
--EXPECT--
bool(true)
bool(true)

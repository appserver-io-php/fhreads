--TEST--
Test legacy constructor (issue #336)
--DESCRIPTION--
This test verifies that legacy ctors do not induce failure
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class test2 {
        function test2() {
            echo "ctor test2\n";
        }
}

class test1 extends Thread {
        function run() {
                $x = new test2();
        }
}

$t = new test1();
$t->start();
$t->join();
--EXPECTF--
Deprecated: Methods with the same name as their class will not be constructors in a future version of PHP; test2 has a deprecated constructor in %s on line %d
ctor test2

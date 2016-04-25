--TEST--
Testing extended object functionality, magic methods bug #34
--DESCRIPTION--
Test that the fix for bug #34 is a success
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test {
    public static function __callStatic ($name, $args) {
        var_dump($name, $args);
    }
}

class UserThread extends Thread {
    public function run () {
        Test::called_func("argument");
    }
}

$thread = new UserThread;
$thread->start();
?>
--EXPECT--
string(11) "called_func"
array(1) {
  [0]=>
  string(8) "argument"
}


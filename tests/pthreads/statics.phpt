--TEST--
Test statics (bug 19)
--DESCRIPTION--
This test verifies that static members in declarations made outside of threads are available inside threads without error
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class TestThread extends Thread {
	static $static = "pthreads rocks!";

	public function run() { var_dump(self::$static); }
}

$thread = new TestThread();
$thread->start();
?>
--EXPECT--
string(15) "pthreads rocks!"

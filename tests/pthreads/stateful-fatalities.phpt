--TEST--
Test stateful fatalities
--DESCRIPTION--
This test verifies that state includes fatalities
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class TestThread extends Thread {
	public function run(){
		i_do_not_exist();
	}
}
$test = new TestThread();
$test->start();
$test->join();
var_dump($test->isTerminated());
?>
--EXPECTF--
Fatal error: Uncaught Error: Call to undefined function i_do_not_exist() in %s:8
Stack trace:
#0 [internal function]: TestThread->run()
#1 {main}
  thrown in %s on line 8
bool(true)


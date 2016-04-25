--TEST--
Test graceful fatalities
--DESCRIPTION--
This test verifies that fatalities are graceful with regard to state
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class TestThread extends Thread {
	public function run(){
		/* silent fatal error */
		echo @MY::$FATAL;
	}
}

$test = new TestThread();
$test->start();
$test->join();
var_dump($test->isTerminated());
?>
--EXPECTF--
Fatal error: Uncaught Error: Class 'MY' not found in %s:9
Stack trace:
#0 [internal function]: TestThread->run()
#1 {main}
  thrown in %s on line 9
bool(true)

--TEST--
Test include/require functions as expected
--DESCRIPTION--
This test verifies that require_once and include are working as expected
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

define("INC", sprintf("%s/includeme.inc", dirname(__FILE__)));

class TestThread extends Thread {
	public function run(){
		require_once(INC);
		if (!function_exists("myTestFunc")) {
			printf("FAILED\n");
		} else printf("OK\n");
	}
}
$test = new TestThread();
$test->start();
$test->join();
?>
--EXPECT--
OK

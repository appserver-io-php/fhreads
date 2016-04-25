--TEST--
Test unset defaults [moot: default values are ignored]
--DESCRIPTION--
This test verifies that unset members do not cause a problem in pthreads objects
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class TestThread extends Thread {
	public $default;
	
	public function run() {
		var_dump($this->default); 
	}
}

$thread = new TestThread();
$thread->start();
?>
--EXPECT--
NULL

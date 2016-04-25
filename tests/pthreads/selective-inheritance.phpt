--TEST--
Test selective inheritance
--XFAIL--
pthreads specific
--DESCRIPTION--
This test verifies the functionality of selective inheritance
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

function TestFunction(){
	return __FUNCTION__;
}

define ("TEST_CONSTANT", true);

class TestClass {}

class TestThread extends Thread {
	public function run() { 
		var_dump(function_exists("TestFunction"));
		var_dump(defined("TEST_CONSTANT"));
		var_dump(class_exists("TestClass")); 
	}
}

$thread = new TestThread();
$thread->start();
$thread->join();

$thread = new TestThread();
$thread->start(PTHREADS_INHERIT_NONE);
$thread->join();

$thread = new TestThread();
$thread->start(PTHREADS_INHERIT_FUNCTIONS | PTHREADS_INHERIT_CLASSES);
$thread->join();
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(false)
bool(false)
bool(false)
bool(true)
bool(false)
bool(true)



--TEST--
Test reading object properties without debug info
--DESCRIPTION--
This test verifies that reading properties from the object without var_dump/print_r will work
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Thread {
	public function run() { 
		$this->name = sprintf("%s", __CLASS__);
	}
}

$thread = new Test();
$thread->start();
$thread->join();
foreach($thread as $key => $value)
	var_dump($value);
?>
--EXPECT--
string(4) "Test"

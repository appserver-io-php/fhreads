--TEST--
Test access to static methods from within user threads
--DESCRIPTION--
Static methods as declared in the users implementation of Thread should now be available for calling in the thread scope
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class ThreadTest extends Thread {
	public static function staticTest(){
		return 1;
	}
	
	public function run(){
		$this->result = self::staticTest();
	}
}
$thread = new ThreadTest();
if($thread->start())
	if ($thread->join())
		var_dump($thread->result);
?>
--EXPECT--
int(1)
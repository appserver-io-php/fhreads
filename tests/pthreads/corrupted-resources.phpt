--TEST--
Testing sane handling of resources and objects bug #39
--DESCRIPTION--
Test that resources and objects are not corrupted when written to thread storage
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Work extends Threaded {
	public function run(){
		
	}
}

class Test extends Thread {
	public function run(){
		$test = new Work();
		$this->test = $test;
		print_r($this->test);
		var_dump($this->test);
		$stream = fopen("/tmp/test.txt", "w+");
		var_dump($stream);
		$this->stream = $stream;
		stream_set_blocking($this->stream, 0);
		var_dump($this->stream);
	}
}

$test =new Test();
$test->start();
$test->join();

?>
--EXPECT--
Work Object
(
)
object(Work)#2 (0) {
}
resource(2) of type (stream)
resource(2) of type (stream)



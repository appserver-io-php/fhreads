--TEST--
Test wait/notify
--DESCRIPTION--
This test will verify wait/notify functionality
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class ThreadTest extends Thread {
	public $sent;
	
	public function __construct() {
	    $this->sent = false;
	}
	
	public function run(){
	    $this->synchronized(function($self){
		    $self->sent = true;
		    $self->notify();
	    }, $this);
	}
}
$thread = new ThreadTest();
if($thread->start()) {
	$thread->synchronized(function($me){
	    if (!$me->sent) {
		    var_dump($me->wait());
	    } else var_dump($me->sent);
	}, $thread);
	
} else printf("bool(false)\n");

$thread->join();

?>
--EXPECT--
bool(true)

--TEST--
Test synchronized blocks
--DESCRIPTION--
This test verifies that syncronization is working
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class T extends Thread {
        public $data;
        public function run() {
            $this->synchronized(function($thread){
				$thread->data = true;
				$thread->notify();
			}, $this);               
        }
}
$t = new T;
$t->start();
$t->synchronized(function($thread){
	if (!$thread->data) {
		var_dump($thread->wait());
	} else var_dump($thread->data);
}, $t);
?>
--EXPECT--
bool(true)

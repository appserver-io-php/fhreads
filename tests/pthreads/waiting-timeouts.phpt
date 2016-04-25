--TEST--
Test waiting timeouts
--XFAIL--
wait timeouts feature not implemented yet.
--DESCRIPTION--
This test verifies that reaching at timeout returns the correct value
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

exit(0);

class T extends Thread {
        public $data;
        public function run() {
            $this->synchronized(function($thread){
				usleep(100000);
			}, $this);               
        }
}
$t = new T;
$t->start();
$t->synchronized(function($thread){
	var_dump($thread->wait(100));
}, $t);
?>
--EXPECT--
bool(false)

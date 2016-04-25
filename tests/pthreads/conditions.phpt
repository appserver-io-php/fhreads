--TEST--
Test conditions
--DESCRIPTION--
This test verifies conditions are working
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class T extends Thread {
    public $data;
	
	public function __construct($mutex, $condition) {
		$this->mutex = $mutex;
		$this->condition = $condition;
	}
	
    public function run(){
		Mutex::lock($this->mutex);
		Cond::broadcast($this->condition);
		Mutex::unlock($this->mutex);
	}
}

$mutex = Mutex::create();
$cond = Cond::create();

Mutex::lock($mutex);

$t = new T($mutex, $cond);
$t->start();

var_dump(Cond::wait($cond, $mutex));

Mutex::unlock($mutex);

Cond::destroy($cond);
Mutex::destroy($mutex);
?>
--EXPECT--
bool(true)

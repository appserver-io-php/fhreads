--TEST--
Testing closure members
--ENV--
USE_ZEND_ALLOC=0
--DESCRIPTION--
This test verifies that closures can be set as members and called from anywhere
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

$test = new Threaded();

$test->some = function(){
    echo "Hello Some" . PHP_EOL;
};

class T extends Thread {
    public $set = false;
    public $used = false;

    public function __construct($test) {
        $this->test = $test;
    }
    
    public function run() {

        /* call original closure */
        $this->call($this->test->some);
        
        /* set new closure */
        $this->synchronized(function($thread){
            $thread->set = true;
            $thread->test->some = function() {
                echo "Hello World" . PHP_EOL;
            };
            $thread->notify();
        });
        
        /* wait for new closure execution */
        $this->synchronized(function($thread){
            while (!$thread->used)
                $thread->wait();
        }, $this);
    }
    
    public function call(Closure $closure) {
        $closure();
    }
}

/* start thread to call closure */
$t = new T($test);
$t->start();

/* wait for new closure */
$t->synchronized(function() use($t) {
    while (!$t->set)
        $t->wait();
});
    
/* call new closure */
$some = $test->some;
$some();

/* allow new closure to be dtored with old context */
$t->synchronized(function() use($t) {
    $t->used = true;
    $t->notify();
});

$t->join();
--EXPECTF--
Hello Some
Hello World

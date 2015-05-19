<?php

// to be compatible with pthreads lib
if (!class_exists('\Thread')) {
    require_once __DIR__ . DIRECTORY_SEPARATOR . "Thread.php";
}

// define counter thread class which counts the shared data objects counter property
class CounterThread extends Thread
{
    public function __construct($data)
    {
        $this->data = $data;
    }
    
    public function run()
    {
        // wait randomized time
        usleep(rand(10000,20000));
        
        // lock data object
        fhread_mutex_lock($this->data->mutex);
        // inc counter
        ++$this->data->counter;
        // add message that this thread was here
        $this->data->{$this->getThreadId()} = $this->getThreadId();
        // unlock data object
        fhread_mutex_unlock($this->data->mutex);
    }
}

// create data storage object
$data = new \stdClass();
// init mutex
$data->mutex = fhread_mutex_init();
// init counter property to be zero
$data->counter = 0;


// init threads array
$ths = array();
// define max threads
$tMax = 10;

// initiate threads
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i] = new CounterThread($data);
}

// start threads
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->start();
}

// wait for all thread to be finished by joining them
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->join();
}

// echo status
echo "$tMax threads finished..." . PHP_EOL;

// dump shared data object
var_dump($data);

// echo status
echo PHP_EOL . "finished script!" . PHP_EOL;


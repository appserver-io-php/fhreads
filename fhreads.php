<?php

if (!class_exists('\Thread')) {
    require_once "ext/fhreads/Thread.php";
}

class TestObject
{
    public function __construct() {
        echo __METHOD__ . PHP_EOL;
    }
    
    public function __destruct() {
        echo __METHOD__ . PHP_EOL;
    }
}

class TestThread extends Thread
{
    public function __construct($data)
    {
        $this->data = $data;
        $this->testObjArray = array();
    }
    
    public function run()
    {
        
        usleep(rand(10000,20000));
        
        fhread_mutex_lock($this->data->mutex);
        ++$this->data->counter;
        fhread_mutex_unlock($this->data->mutex);
        
        /*
        $this->data->test[] = $this->getThreadId();
        
        fhread_mutex_lock($GLOBALS["mutex"]);
        $this->data->testObjArray[] = new TestObject();
        fhread_mutex_unlock($GLOBALS["mutex"]);
        
        
        
        
        $this->testInt = 123;

        $this->testStr = 'test';

        $this->testFloat = 1.234;

        $this->testBool = true;

        $this->testArray = array('a' => 'b');

        
        /*
        $this->testObj = new stdClass();
        $this->testObjArray[] = new stdClass();
        $this->testObjArray[0]->test = 'test';
        */
/*
        $this->data->test = $this->getThreadId();
        
        usleep(rand(1000,20000));
        */
    }
}

/*
for ($i = 1; $i < 200; $i++) {
    var_dump(fhread_tsrm_new_interpreter_context());
}


sleep(3);

exit(0);

*/

/*
$t = new TestThread();
$t->outside = 'outside';
$t->start();
$t->join();
*/


$data = new \stdClass();
$data->test = array();
$data->counter = 0;
$data->mutex = fhread_mutex_init();



$ths = array();
$tMax = 10;

$ctxCount = 1;


for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i] = new TestThread($data);
}
while(1) {
    

for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->start();
}

for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->join();
}

echo "$tMax threads finished, restarting..." . PHP_EOL;

var_dump($data);

sleep(1);

}

echo PHP_EOL . "finished script!" . PHP_EOL;


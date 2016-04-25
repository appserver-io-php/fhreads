--TEST--
Ensure that zend objects_store is thread-safe in concurrent operations
--FILE--
<?php 

if (!class_exists('\Thread')) {
    require_once __DIR__ . "/../bootstrap.inc";
}

class TestThread extends \Thread
{
    public function __construct($iMax) {
        $this->iMax = $iMax;
    }
    
    public function run() {
        $l = 0;
        while($l++ < $this->iMax) {
            $this->objects[rand(0,99)] = new \stdClass();
        }
    }
}

$tMax = 100;
$iMax = 10000;
$t = array();

for ($i = 0; $i < $tMax; $i++) {
    $t[$i] = new TestThread($iMax);
}

for ($i = 0; $i < $tMax; $i++) {
    $t[$i]->start();
}

for ($i = 0; $i < $tMax; $i++) {
    $t[$i]->join();
}

var_dump(fhread_objects_store_top() < $iMax + $tMax * 2);
?>
--EXPECT--
bool(true)

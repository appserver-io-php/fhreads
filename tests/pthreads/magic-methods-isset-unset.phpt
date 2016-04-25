--TEST--
Test magic __isset and __unset
--DESCRIPTION--
This test verifies that __isset and __unset work as expected
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Threaded {

    public function __isset($key) {
        printf("magic %s\n", __FUNCTION__);
        
        return isset($this[$key]);
    }
    
    public function __unset($key) {
        printf("magic %s\n", __FUNCTION__);
        
        unset($this[$key]);
    }
}
$test = new Test();
isset($test->one);
$test->two = "two";
var_dump(isset($test->two));
unset($test->one);
var_dump(isset($test->one));
?>
--EXPECT--
magic __isset
bool(true)
magic __unset
magic __isset
magic __isset
bool(false)


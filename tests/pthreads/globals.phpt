--TEST--
Test global inheritance
--DESCRIPTION--
Test functionality of globals inheritance
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Thread {
	public function run(){
		global $object, $array;
		
		var_dump($object, $array);
	}
}

$object = new stdClass();
$object->one = "one";
$object->two = "two";
$object->three = "three";

$array = array(1, 2, 3);

$test = new Test();
$test->start();
$test->join();
?>
--EXPECTF--
object(stdClass)#%d (%d) {
  ["one"]=>
  string(3) "one"
  ["two"]=>
  string(3) "two"
  ["three"]=>
  string(5) "three"
}
array(3) {
  [0]=>
  int(1)
  [1]=>
  int(2)
  [2]=>
  int(3)
}

--TEST--
Test class defaults
--DESCRIPTION--
Class defaults should now initialize defaults properly
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Thread {

	public function run(){
		var_dump($this);
	}
	
	public $string = "hello world";
	protected $array  = array(1, 2, 3);
	private $pstring  = "world hello";
	private $parray   = array(3, 2, 1);
	protected static $nocopy = true;
}

$test =new Test();
$test->string = strrev($test->string);
$test->start();
$test->join();
?>
--EXPECTF--
object(Test)#1 (%d) {
  ["string"]=>
  string(11) "dlrow olleh"
  ["array":protected]=>
  array(3) {
    [0]=>
    int(1)
    [1]=>
    int(2)
    [2]=>
    int(3)
  }
  ["pstring":"Test":private]=>
  string(11) "world hello"
  ["parray":"Test":private]=>
  array(3) {
    [0]=>
    int(3)
    [1]=>
    int(2)
    [2]=>
    int(1)
  }
  ["threadId":protected]=>
  %s(%d)
  ["fhreadHandle":protected]=>
  %s(%d)
  ["globalMutex":protected]=>
  int(%d)
  ["stateMutex":protected]=>
  int(%d)
  ["syncMutex":protected]=>
  int(%d)
  ["syncNotify":protected]=>
  int(%d)
  ["state":protected]=>
  int(%d)
}


--TEST--
Testing member count
--DESCRIPTION--
This test verifies that getting member counts works
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Threaded {
	public function run() { 
	}
}

$t = new Test();
$t[] = "one";
$t[] = "two";
$t["three"] = "three";
var_dump(count($t));
?>
--EXPECT--
int(3)

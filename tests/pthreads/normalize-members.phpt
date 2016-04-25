--TEST--
Testing normalizing members
--DESCRIPTION--
This tests that normalizing members works without effort
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

/* get a normal array */
$normal = (array) $t;
var_dump(is_array($normal));
?>
--EXPECT--
bool(true)

--TEST--
Test pool defaults
--XFAIL--
Pool class not implemented yet.
--DESCRIPTION--
This test verifies pool defaults
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Work extends Threaded {
	public function run() {
		var_dump($this);
	}
}

$pool = new Pool(1);
$pool->submit(new Work());
$pool->shutdown();

$pool->collect(function(Work $work) {
	return true;
});

var_dump($pool);
?>
--EXPECTF--
object(Work)#%d (%d) {
  ["worker"]=>
  object(Worker)#%d (%d) {
  }
}
object(Pool)#%d (%d) {
  ["size":protected]=>
  int(1)
  ["class":protected]=>
  string(6) "Worker"
  ["workers":protected]=>
  array(0) {
  }
  ["work":protected]=>
  array(0) {
  }
  ["ctor":protected]=>
  NULL
  ["last":protected]=>
  int(1)
}



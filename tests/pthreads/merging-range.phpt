--TEST--
Testing merging members (long keys)
--DESCRIPTION--
This tests that merging ranges works as expected (long keys)
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Storage extends Threaded {
    public function run() {}
}

$storage = new Storage();
$storage->merge(range("0", "3"));
var_dump(count($storage));
?>
--EXPECTF--
int(4)

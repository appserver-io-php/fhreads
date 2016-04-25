--TEST--
Test shutdown handlers #204
--DESCRIPTION--
Shutdown handlers that were closures were causing segfaults
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class Test extends Thread {
        public function run() {
                register_shutdown_function(function(){
                        var_dump(new stdClass());
                });
        }
}

$test = new Test();
$test->start();
?>
--EXPECTF--
object(stdClass)#%d (0) {
}


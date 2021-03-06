--TEST--
Test trait aliases
--SKIPIF--
<?php if(PHP_VERSION_ID < 50400) die("skip do not run for <5.4"); ?>
--DESCRIPTION--
Fix bug #274, trait aliases causing memory errors
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

trait testTrait {
    public function fromTrait ($foo) {
        var_dump(__FUNCTION__);
    }
}

class myThread extends Thread {
    use testTrait  {fromTrait as fromTraitAliased;}

    public function run() {
        $this->fromTrait('blah');
    }
}

$t = new myThread();
$t->start();
$t->join();
--EXPECT--
string(9) "fromTrait"


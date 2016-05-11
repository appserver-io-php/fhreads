--TEST--
Test spl autoloader sharing
--DESCRIPTION--
This test verifies that spl autoloader is shared between thread contexts
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php
class MyAutoLoader {
    static function autoLoad($className) {
        echo __METHOD__ . "($className)\n";
    }
}

class TestThread {

    public function run()
    {
        // check
        var_dump(class_exists("TestClass", true));
    }

}

spl_autoload_register('MyAutoLoader::autoLoad');

var_dump(spl_autoload_functions());

$threadId = null;
$fhreadHandle = null;
$rv = null;

fhread_create(new TestThread(), $threadId, $fhreadHandle);
fhread_join($fhreadHandle, $rv);
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(12) "MyAutoLoader"
    [1]=>
    string(8) "autoLoad"
  }
}
MyAutoLoader::autoLoad(TestClass)
bool(false)
===DONE===

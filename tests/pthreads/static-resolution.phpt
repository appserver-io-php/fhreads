--TEST--
Test static:: regression
--DESCRIPTION--
Bug #210 shows static:: requires different logic to self::
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

class testbug extends Thread
{
    public static $somevar = 123;
    
    function __construct()
    {
        var_dump(self::$somevar); 
        var_dump(static::$somevar);
    }

    public function run()
    {
        var_dump(self::$somevar);
        var_dump(static::$somevar);
    }
}

$testbug = new testbug;
$testbug->start();
?>
--EXPECT--
int(123)
int(123)
int(123)
int(123)

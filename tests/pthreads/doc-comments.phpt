--TEST--
Test doc comments are copied properly
--DESCRIPTION--
Test that doc comments are copied, no leaking/errors
--ENV--
USE_ZEND_ALLOC=0
--FILE--
<?php

if (!extension_loaded('pthreads'))
    require dirname(__DIR__) . DIRECTORY_SEPARATOR . 'bootstrap.inc';

/**
* Comment
* @doc comment
* @package package
* @subpackage subpackage
*/

/**
* Comment
*/
class T extends Thread {  
    /**
    * @var testing
    */
    public $content;
    /**
    * Method comment
    * @doc comment
    * @package package
    * @subpackage subpackage
    */
    public function run() {
       $reflect = new ReflectionMethod("T", "run");
       var_dump($reflect);
       var_dump($reflect->getDocComment());
    }
}

$t = new T();
$t->start();
$t->join();

$reflect = new ReflectionMethod("T", "run");
var_dump($reflect);
var_dump($reflect->getDocComment());

?>
--EXPECT--
object(ReflectionMethod)#2 (2) {
  ["name"]=>
  string(3) "run"
  ["class"]=>
  string(1) "T"
}
string(102) "/**
    * Method comment
    * @doc comment
    * @package package
    * @subpackage subpackage
    */"
object(ReflectionMethod)#2 (2) {
  ["name"]=>
  string(3) "run"
  ["class"]=>
  string(1) "T"
}
string(102) "/**
    * Method comment
    * @doc comment
    * @package package
    * @subpackage subpackage
    */"

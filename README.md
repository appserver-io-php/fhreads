# fhreads

## A PHP extension to get real system threading functionality in php userland.

[![Build Status](https://travis-ci.org/appserver-io-php/fhreads.svg?branch=master)](https://travis-ci.org/appserver-io-php/fhreads)

## What is it?

The extension gives you access to plain system threading functions for being able to implement threading utilites as part of the language directly in the PHP userland e.g. an abstract Thread class.

## How to use?

> This extension is in early-stage development. Please use with caution.

This is a simple example how to use a shared data object as normal php object with a thread safe counter property.
```php
<?php

require_once __DIR__ . DIRECTORY_SEPARATOR . "Thread.php";

// define counter thread class which counts the shared data objects counter property
class CounterThread extends Thread
{
    /**
     * Constructor which refs the shared data object to this
     * 
     * @param object $data
     */
    public function __construct($data)
    {
        $this->data = $data;
    }
    
    public function run()
    {
        fhread_mutex_lock($this->data->mutex);
        ++$this->data->counter;
        $this->data->{$this->getThreadId()} = $this->getThreadId();
        fhread_mutex_unlock($this->data->mutex);
    }
}

$data = new \stdClass();
$data->mutex = fhread_mutex_init();
$data->counter = 0;

$ths = array();
$tMax = 10;

// initiate threads
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i] = new CounterThread($data);
}
// start threads
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->start();
}
// wait for all thread to be finished by joining them
for ($i = 1; $i <= $tMax; $i++) {
    $ths[$i]->join();
}

// echo status
echo "$tMax threads finished..." . PHP_EOL;
// dump shared data object
var_dump($data);
// echo status
echo PHP_EOL . "finished script!" . PHP_EOL;
```

This should give us an amazing output! :)
```bash
10 threads finished...
object(stdClass)#1 (12) {
  ["mutex"]=>
  int(19463520)
  ["counter"]=>
  int(10)
  ["139792551667456"]=>
  int(139792551667456)
  ["139792533702400"]=>
  int(139792533702400)
  ["139792506754816"]=>
  int(139792506754816)
  ["139792488789760"]=>
  int(139792488789760)
  ["139792515737344"]=>
  int(139792515737344)
  ["139792524719872"]=>
  int(139792524719872)
  ["139792479807232"]=>
  int(139792479807232)
  ["139792560649984"]=>
  int(139792560649984)
  ["139792542684928"]=>
  int(139792542684928)
  ["139792497772288"]=>
  int(139792497772288)
}

finished script!
```

See `examples` folder for more usage demonstration.

More documentation and examples comming...


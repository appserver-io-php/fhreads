--TEST--
Check for fhreads presence
--SKIPIF--
<?php if (!extension_loaded("fhreads")) print "skip"; ?>
--FILE--
<?php 
echo "fhreads extension is available";
?>
--EXPECT--
fhreads extension is available

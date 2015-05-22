<?php

/**
 * NOTICE OF LICENSE
 *
 * This source file is subject to the Open Software License (OSL 3.0)
 * that is available through the world-wide-web at this URL:
 * http://opensource.org/licenses/osl-3.0.php
 *
 * PHP version 5
 *
 * @author    Johann Zelger <jz@appserver.io>
 * @copyright 2015 TechDivision GmbH <info@appserver.io>
 * @license   http://opensource.org/licenses/osl-3.0.php Open Software License (OSL 3.0)
 * @link      https://github.com/appserver-io-php/fhreads
 * @link      http://www.appserver.io
 */

/**
 * Interface Runnable
 * 
 * Simple interface for runnables
 *
 * @author    Johann Zelger <jz@appserver.io>
 * @copyright 2015 TechDivision GmbH <info@appserver.io>
 * @license   http://opensource.org/licenses/osl-3.0.php Open Software License (OSL 3.0)
 * @link      https://github.com/appserver-io/fhreads
 * @link      http://www.appserver.io
 */
interface Runnable
{
    public function run();
}

/**
 * Abstract Class Thread
 *
 * Simple thread abstract class which implements runnable interface
 *
 * @author    Johann Zelger <jz@appserver.io>
 * @copyright 2015 TechDivision GmbH <info@appserver.io>
 * @license   http://opensource.org/licenses/osl-3.0.php Open Software License (OSL 3.0)
 * @link      https://github.com/appserver-io/fhreads
 * @link      http://www.appserver.io
 */
abstract class Thread implements Runnable
{
    /**
     * Holds thread id if started
     * 
     * @var int
     */
    protected $threadId = null;
    
    /**
     * Abstract run function
     * 
     * @return void
     */
    abstract function run();
    
    /**
     * Internal function to get globals identifier string
     * 
     * @return string
     */
    public function getGlobalsIdentifier()
    {
        return get_class($this) . "#" . fhread_object_get_handle($this);
    }
    
    /**
     * Start method which will prepare, create and starts a thread
     * 
     * @return void
     */
    public function start()
    {
        // set ref to globals for thread to use it from
        $GLOBALS[$this->getGlobalsIdentifier()] = $this;

        // create, start thread and save thread id 
        $this->threadId = fhread_create($this->getGlobalsIdentifier());
    }
    
    /**
     * Destroyes the thread and remove from from globals
     * 
     * @return void
     */
    public function destroy()
    {
        unset($GLOBALS[$this->getGlobalsIdentifier()]);
    }
    
    /**
     * Returns the thread id
     * 
     * @return int
     */
    public function getThreadId()
    {
        return $this->threadId;
    }
    
    /**
     * Joins the current thread by its thread id.
     * 
     * @return void
     */
    public function join()
    {
        if ($this->getThreadId()) {
            fhread_join($this->getThreadId());
        }
    }
}

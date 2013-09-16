<?php
namespace Theapi\CctvBlindfoldBundle;


use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;

/**
 * Represents the movable cover over the camera lens.
 */
class Blindfold
{

    const CLOSED = 0;
    const OPEN = 1;


    /**
    * The physical driver of the blindfold (the stepper)
    */
    protected $driver;

    /**
     * The event dispatcher
     */
    protected $eventDispatcher;

    /**
    * Whether the blindfold is currently open or closed.
    * @var int
    */
    protected $state = self::CLOSED;

    /**
    * The OutputInterface object
    */
    protected $output;

    /**
    * Constructor
    */
    public function __construct($driver, EventDispatcherInterface $eventDispatcher)
    {
        $this->driver = $driver;
        $this->eventDispatcher = $eventDispatcher;
        $this->eventDispatcher->addListener('device_detector.found', array($this, 'close'));
        $this->eventDispatcher->addListener('device_detector.not_found', array($this, 'open'));
    }

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    public function toggle()
    {
        if ($this->getState() == self::OPEN) {
            $this->close();
        } else {
            $this->open();
        }
    }

    public function rotate($amount = 80)
    {
        $this->driver->rotate($amount);
    }

    /**
    * Ensure the blindfold is open.
    */
    public function open()
    {
        if ($this->getState() != self::OPEN) {
            $this->driver->open();
            $this->setState(self::OPEN);
        }
    }

    /**
    * Ensure the blindfold is closed.
    */
    public function close()
    {
        if ($this->getState() != self::CLOSED) {
            $this->driver->close();
            $this->setState(self::CLOSED);
        }
    }

    public function demo()
    {
        $this->driver->demo();
    }

    /**
    * Closed or Open
    *
    * @return int
    */
    public function getState()
    {
        if (file_exists('/tmp/cctvbf_state')) {
            $this->state = self::OPEN;
        } else {
            $this->state = self::CLOSED;
        }

        return $this->state;
    }

    /**
    * Set the internal knowledge of the blindfold state.
    */
    public function setState($state)
    {
        if ($state == self::CLOSED) {
            if (file_exists('/tmp/cctvbf_state')) {
                unlink('/tmp/cctvbf_state');
            }
            $this->state = self::CLOSED;
        } elseif ($state == self::OPEN) {
            touch('/tmp/cctvbf_state');
            $this->state = self::OPEN;
        }
    }
}

<?php
namespace Theapi\CctvBlindfoldBundle;


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
    * Whether the blindfold is currently open or closed.
    * @var int
    */
    protected $state = self::CLOSED;

    /**
    * Constructor
    */
    public function __construct($driver) {
        $this->driver = $driver;

    }

    /**
    * Ensure the blindfold is open.
    */
    public function open() {
        if ($this->getState() != self::OPEN) {
            $this->driver->open();
            $this->setState(self::OPEN);
        }
    }

    /**
    * Ensure the blindfold is closed.
    */
    public function close() {
        if ($this->getState() != self::CLOSED) {
            $this->driver->close();
            $this->setState(self::CLOSED);
        }
    }

    /**
    * Closed or Open
    *
    * @return int
    */
    public function getState() {
        return $this->state;
    }

    /**
    * Set the internal knowledge of the blindfold state.
    */
    public function setState($state) {
        if ($state == self::CLOSED) {
          $this->state = self::CLOSED;
        } else if ($state == self::CLOSED) {
          $this->state = self::OPEN;
        }
    }

}

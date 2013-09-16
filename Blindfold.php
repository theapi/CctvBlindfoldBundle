<?php
namespace Theapi\CctvBlindfoldBundle;

use Symfony\Component\EventDispatcher\GenericEvent;
use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use Symfony\Component\DependencyInjection\ContainerAware;
use Symfony\Component\DependencyInjection\ContainerInterface;

/**
 * Represents the movable cover over the camera lens.
 */
class Blindfold extends ContainerAware
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
     * Sensor sockets to listen to
     */
    protected $readSockets = array();

    /**
    * Constructor
    */
    public function __construct($driver)
    {
        $this->driver = $driver;

        // bit rough & ready ATM Socket connect to the analogue to digital converter (adc.py)
        $this->adcSocket = @stream_socket_client('tcp://localhost:8889', $errno, $errstr, 30);
        if ($this->adcSocket) {
          $this->readSockets[] = $this->adcSocket;
        }
    }

    public function setContainer(ContainerInterface $container = null)
    {
        $this->container = $container;
        $this->addListeners();
    }

    public function addListeners() {
        $this->eventDispatcher = $this->container->get('event_dispatcher');
        $this->eventDispatcher->addListener('device_detector.found', array($this, 'handleDeviceFound'));
        $this->eventDispatcher->addListener('device_detector.not_found', array($this, 'open'));
        $this->eventDispatcher->addListener('blindfold.stream_data', array($this, 'handleStreamData'));
    }

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    public function streamSelect() {
        if (empty($this->readSockets)) {

            return false;
        }

        $read = $this->readSockets;
        $write = $except = null;
        $changed = stream_select($read, $write, $except, 0, 200000);
        foreach ($read as $stream) {
            $data = fread(stream, 8192);
            if (strlen($data) > 0) {
                $event = new GenericEvent(
                    stream,
                    array('data' => $data)
                );
                $this->eventDispatcher->dispatch('blindfold.stream_data', $event);
            }
        }

        return true;
    }

    public function handleStreamData(GenericEvent $event) {
        $stream = $event->getSubject();
        $data = $event['data'];
        //@todo do something with the stream data...
    }

    public function handleDeviceFound() {
        $this->output->writeln(__FUNCTION__); // tmp just to check event is being caught
        // currently just ensure closed
        // but in future it depends on other sensors in handleStreamData()
        $this->close();
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

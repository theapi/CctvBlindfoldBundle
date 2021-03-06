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
    const RUN_MOVED = '/run/shm/cctvbf_moved';
    const RUN_STATE = '/run/shm/cctvbf_state';


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
     * The last time the socket asked for a toggle.
     */
    protected $lastSocketToggle = 0;

    /**
     * Whether to let the blindfold be opened.
     */
    protected $allowOpen = true;

    /**
     * Whether to let the blindfold be closed.
     */
    protected $allowClose = true;

    /**
     * Registerd plugins
     */
    protected $plugins = array();

    /**
    * Constructor
    */
    public function __construct($driver)
    {
        $this->driver = $driver;
    }

    public function setPlugin($plugin)
    {
        $this->plugins[] = $plugin;
    }

    public function setContainer(ContainerInterface $container = null)
    {
        $this->container = $container;
        $this->addListeners();
    }

    public function addListeners() {
        $this->eventDispatcher = $this->container->get('event_dispatcher');
        $this->eventDispatcher->addListener('device_detector.found', array($this, 'handleDeviceFound'));
        $this->eventDispatcher->addListener('device_detector.not_found', array($this, 'handleDeviceNotFound'));
        $this->eventDispatcher->addListener('blindfold.stream_data', array($this, 'handleStreamData'));
    }

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    /**
     * Start the python daemon that listens for inputs from sensors, switches etc.
     */
    public function startInputDaemon()
    {
        // The daemon must run independantly in the background.
        // So not using Symfony process.
        $cmd = 'nohup sudo python '. __DIR__ . '/python/input.py > /dev/null 2>&1 & echo $!';
        exec($cmd, $output);
        $this->inputPid = (int) $output[0];
        if (!empty($this->inputPid)) {

            return true;
        }

        return false;
    }

    /**
     * Connect to the socket server of the input daemon (input.py).
     */
    public function connectToInputDaemon()
    {
        $socket = @stream_socket_client('tcp://localhost:8889', $errno, $errstr, 30);
        if ($socket) {
            $this->readSockets[] = $socket;

            return true;
        }

        return false;
    }

    public function streamSelect()
    {
        if (empty($this->readSockets)) {

            return false;
        }

        $read = $this->readSockets;
        $write = $except = null;
        $changed = stream_select($read, $write, $except, 0, 200000);
        foreach ($read as $stream) {
            $data = fread($stream, 8192);
            if (strlen($data) > 0) {
                $event = new GenericEvent(
                    $stream,
                    array('data' => $data)
                );
                $this->eventDispatcher->dispatch('blindfold.stream_data', $event);
            }
        }

        return true;
    }

    public function handleStreamData(GenericEvent $event)
    {
        $stream = $event->getSubject();
        $data = $event['data'];

        $this->output->writeln($data); // tmp

        if ($data == 'TOGGLE') {
            // 1 second debounce
            $now = time();
            if ($now - $this->lastSocketToggle > 1) {
                $this->lastSocketToggle = $now;
                $this->toggle();
                //@todo stop the device detector overiding the toggle switch...
            }
        }
    }

    public function handleDeviceFound()
    {
        $this->output->writeln(__FUNCTION__); // tmp just to check event is being caught
        if ($this->getAllowClose()) {
            $this->close();
        }
    }

    public function handleDeviceNotFound()
    {
        $this->output->writeln(__FUNCTION__); // tmp just to check event is being caught
        if ($this->getAllowOpen()) {
            $this->open();
        }
    }

   /**
     * Whether the blindfold is allwed to be opened.
     */
    public function getAllowOpen()
    {
        return $this->allowOpen;
    }

    /**
     * Allow the blindfold to be opened.
     *
     * @todo priority - to arbitrate over arguments between sensors, plugins etc.
     */
    public function setAllowOpen($bool)
    {
        $this->allowOpen = (bool) $bool;
    }

    /**
     * Whether the blindfold is allwed to be closed.
     */
    public function getAllowClose()
    {
        return $this->allowClose;
    }

    /**
     * Allow the blindfold to be closed.
     *
     * @todo priority - to arbitrate over arguments between sensors, plugins etc.
     */
    public function setAllowClose($bool)
    {
        $this->allowClose = (bool) $bool;
    }

    public function toggle()
    {
        if ($this->getState() == self::OPEN) {
            $this->close();
        } else {
            $this->open();
        }
    }

    public function rotate($amount = 128)
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

            // Let stateless scripts know when it was moved.
            touch(self::RUN_MOVED);
            // Tell everyone.
            $this->eventDispatcher->dispatch('blindfold.open');
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

            // Let stateless scripts know when it was moved.
            touch(self::RUN_MOVED);
            // Tell everyone.
            $this->eventDispatcher->dispatch('blindfold.close');
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
        if (file_exists(self::RUN_STATE)) {
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
            if (file_exists(self::RUN_STATE)) {
                unlink(self::RUN_STATE);
            }
            $this->state = self::CLOSED;
        } elseif ($state == self::OPEN) {
            touch(self::RUN_STATE);
            $this->state = self::OPEN;
        }
    }
}

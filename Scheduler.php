<?php
namespace Theapi\CctvBlindfoldBundle;


use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use Symfony\Component\DependencyInjection\ContainerAware;


/**
 * Detect the precense of devices (mobile phones)
 */
class Scheduler extends ContainerAware
{

    /**
     * The event dispatcher
     */
    protected $eventDispatcher;

    /**
    * The OutputInterface object
    */
    protected $output;

    /**
     * The blindfold object from  the container.
     */
    protected $blindfold;

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    public function setContainer(ContainerInterface $container = null)
    {
        $this->container = $container;
        $this->addListeners();
        $this->blindfold = $container->get('theapi_cctvblindfold.blindfold');
    }

    public function addListeners()
    {
        $this->eventDispatcher = $this->container->get('event_dispatcher');
        $this->eventDispatcher->addListener('device_detector.pre_detect', array($this, 'onPreDetect'));
    }

    /**
     * Callback for device_detector.pre_detect
     */
    public function onPreDetect()
    {
        //@todo: configuration etc.



    }

}

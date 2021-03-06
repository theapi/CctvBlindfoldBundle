<?php
namespace Theapi\CctvBlindfoldBundle;

use Doctrine\ORM\EntityManager;

use Theapi\CctvBlindfoldBundle\Entity\Detection;

use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use Symfony\Component\DependencyInjection\ContainerAware;
use Symfony\Component\DependencyInjection\ContainerInterface;


/**
 * Detect the precense of devices (mobile phones)
 */
class DeviceDetector extends ContainerAware
{
    /**
     * The bluetooth devices to scan
     * @var array
     */
    protected $bluetooth_devices;

    /**
    * The devices to scan
    */
    protected $devices;

    /**
    * Devices that have been detected
    */
    protected $detected;

    /**
     * The event dispatcher
     */
    protected $eventDispatcher;

    /**
    * The OutputInterface object
    */
    protected $output;

    /**
    * A Symfony\Component\Process object
    */
    protected $process;

    /**
    * Constructor
    */
    public function __construct($bluetooth_devices, $devices, $process)
    {
        $this->bluetooth_devices = $bluetooth_devices;
        $this->devices = $devices;
        $this->process = $process;

        $this->found = array();

        try {
            $this->validateDevices();
        } catch (\InvalidArgumentException $e) {
            throw $e;
        }

    }

    public function setContainer(ContainerInterface $container = null)
    {
        $this->container = $container;
        $this->eventDispatcher = $this->container->get('event_dispatcher');
    }

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    public function setEntityManager(EntityManager $em)
    {
        $this->em = $em;
    }

    /**
    * Detect the the devices of interest.
    *
    * Dispatches events:
    *   device_detector.found
    *   device_detector.not_found
    *
    */
    public function detect($count = 0)
    {
        $this->eventDispatcher->dispatch('device_detector.pre_detect');

        try {
            // are the devices on the network...
            $str = $this->scanWifi();
            $this->analyseScanWifi($str);
            // the bluetooth dongle disconects itself after a couple of days :(
            //$this->scanBluetooth();
        } catch (\Exception $e) {
            // failed to get a meaningfull result
            if (!empty($this->output)) {
                $out = 'Scan failed: <info>' . $e->getMessage() . '</info> ' . date('r');
            }
            return;
        }

        if (!empty($this->detected)) {
            $present = true;
        } else {
            $present = false;
        }


        if (!$present && $count < 3) {
            // repeat in a few seconds because the phones take a while to respond initially...
            sleep(10);
            return $this->detect($count + 1);
        }


        if (!empty($this->output)) {
            if ($present) {
                $out = 'Detected:';
                foreach ($this->detected as $detected) {
                    $out .= ' <info>' . $detected . '</info>';
                }
                $devices = join(' ', $this->detected);
            } else {
                $out = 'Not found: <info>' . join(', ', $this->bluetooth_devices) . '</info> ' . date('r');
                $devices = '';
            }
            $this->output->writeln($out);
        }

        // TMP store in db
        /*
        if (isset($this->em)) {
            $detection = new Detection();
            $detection->setTimestamp(new \DateTime());
            $detection->setDevices($devices);

            try {
                $this->em->persist($detection);
                $this->em->flush();
            } catch (\PDOException $e) {
                // ignore for now
            }
        }
        */

        // Tell all who care
        $eventDispatcher = $this->container->get('event_dispatcher');
        if ($present) {
            $eventDispatcher->dispatch('device_detector.found');
        } else {
            $eventDispatcher->dispatch('device_detector.not_found');
        }

    }

    /**
     * Scan for named bluetooth devices
     * @throws \RuntimeException
     */
    public function scanBluetooth()
    {
        $this->detected = array();
        foreach ($this->bluetooth_devices as $bt) {
            $cmd = 'hcitool name ' . $bt;
            $this->process->setCommandLine($cmd);
            $this->process->run();
            if (!$this->process->isSuccessful()) {
                throw new \RuntimeException($this->process->getErrorOutput());
            }
            $output = $this->process->getOutput();
            if (!empty($output)) {
                $this->detected[] = $output;
            }
        }
    }

    /**
    * Scan the network for attached devices
    *
    * @throws \RuntimeException
    * @return string
    */
    public function scanWifi()
    {
        $cmd = 'sudo nmap -sP ' . $this->devices . ' -oX - 2>&1';

        $this->process->setCommandLine($cmd);
        $this->process->run();
        if (!$this->process->isSuccessful()) {
            throw new \RuntimeException($this->process->getErrorOutput());
        }
        $output = $this->process->getOutput();

        return trim($output);
    }

    /**
    * Analyse the output of the scan to find the relevant info.
    *
    * @throws Exception
    */
    public function analyseScanWifi($str)
    {
        $this->detected = array();

        // output results if verbose (app/console -v cctvbf:detect)
        if (!empty($this->output) && $this->output->getVerbosity() > 1) {
            $this->output->writeln($str);
        }

        try {
            $xml = new \SimpleXMLElement($str);
            $up = (int) $xml->runstats->hosts['up'];
            $this->output->writeln('Up: <info>' . $up . '</info>');
        } catch (\Exception $e) {
            // failed to get a meaningfull result
            throw $e;
        }

        if ($up == 0) {
            // no devices found
            return;
        }

        // Note which ones were found
        foreach ($xml->host as $host) {
            if ($host->status[0]['state'] == 'up') {
                foreach ($host->address as $address) {
                    if ($address['addrtype'] == 'ipv4') {
                        $this->detected[] = $address['addr'];
                    }
                }
            }
        }

        return;
    }

    protected function validateDevices()
    {
        // Expects space seperated ip addresses.
        //
        // theapi_cctv_blindfold:
        //     devices: 192.168.0.112 192.168.0.118
        $devices = explode(' ', $this->devices);
        $ips = array();
        foreach ($devices as $ip) {
            if ($valid = filter_var($ip, FILTER_VALIDATE_IP)) {
                $ips[] = $valid;
            }
        }

        if (empty($ips)) {
            $this->devices = null;
            throw new \InvalidArgumentException('No valid ip addresses provided for theapi_cctv_blindfold:devices');
        }

        $this->devices = join(' ', $ips);
    }
}

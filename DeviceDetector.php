<?php
namespace Theapi\CctvBlindfoldBundle;

use Doctrine\ORM\EntityManager;

use Theapi\CctvBlindfoldBundle\Entity\Detection;

use Symfony\Component\Console\Output\OutputInterface;
use Symfony\Component\EventDispatcher\EventDispatcherInterface;
use Symfony\Component\DependencyInjection\ContainerAware;


/**
 * Detect the precense of devices (mobile phones)
 */
class DeviceDetector extends ContainerAware
{

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
    public function __construct($devices, $process)
    {
        $this->devices = $devices;
        $this->process = $process;

        $this->found = array();

        try {
            $this->validateDevices();
        } catch (\InvalidArgumentException $e) {
            throw $e;
        }

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
    * @return bool
    */
    public function detect()
    {
        // are the phones on the network...
        $this->detected = array();
        $str = $this->scan();
        $present = $this->analyseScan($str);
        if (!$present) {
            // repeat in a few seconds because the phones take a while to respond initially...
            sleep(10);
            $str = $this->scan();
            $present = $this->analyseScan($str);
        }

        if (!empty($this->output)) {
            if ($present) {
                $out = 'Detected:';
                foreach ($this->detected as $detected) {
                    $out .= ' <info>' . $detected . '</info>';
                }
                $devices = join(' ', $this->detected);
            } else {
                $out = 'Devices: <info>' . $this->devices . '</info> not found';
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

        return $present;
    }

    /**
    * Scan the network for attached devices
    *
    * @throws \RuntimeException
    * @return string
    */
    public function scan()
    {
        $cmd = 'nmap -sP ' . $this->devices . ' 2>&1';

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
    * @return bool
    */
    public function analyseScan($str)
    {

        // output results if verbose (app/console -v cctvbf:detect)
        if (!empty($this->output) && $this->output->getVerbosity() > 1) {
            $this->output->writeln($str);
        }

        if (strstr($str, '(0 hosts up)')) {
            // no devices found
            return false;
        }

        // Note which ones were found
        $devices = explode(' ', $this->devices);

        foreach ($devices as $ip) {
            if (strstr($str, '(' . $ip . ')')) {
                $this->detected[] = $ip;
            }
        }

        return true;
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

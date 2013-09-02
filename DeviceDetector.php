<?php
namespace Theapi\CctvBlindfoldBundle;

use Symfony\Component\Console\Output\OutputInterface;

/**
 * Detect the precense of devices (mobile phones)
 */
class DeviceDetector
{

  /**
   * The devices to scan
   */
  protected $devices;

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
  public function __construct($devices, $process) {

    try {
      $this->validateDevices();
    } catch (\Exception $e) {
      throw $e;
    }

    $this->devices = $devices;
    $this->process = $process;
  }

  public function setOutput(OutputInterface $output) {
    $this->output = $output;
  }

  /**
   * Detect the the devices of interest.
   */
  public function detect() {
    // are the phones on the network...
    $str = $this->scan();

    $present = $this->analyseScan($str);

    // @todo: repeat in 30 seconds because the phones take a while to responde initially...



  }

  /**
   * Scan the network for attached devices
   *
   * @throws \RuntimeException
   * @return string
   */
  public function scan() {

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
  public function analyseScan($str) {

    // for now just output to screen
    if (!empty($this->output)) {
      $this->output->writeln($str);
    }

    // @todo: analyse...
    // look for the device ip address (or name):
    // Nmap scan report for android-xxxxxxx.example.com (192.168.0.118)


    return true;
  }

  protected function validateDevices() {
    // @todo
    // currently expects space seperated ip address
    //
    // theapi_cctv_blindfold:
    //     devices: 192.168.0.112 192.168.0.118
  }


}

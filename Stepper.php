<?php
namespace Theapi\CctvBlindfoldBundle;

use PhpGpio\Gpio;

use Symfony\Component\Console\Output\OutputInterface;

/**
 * Run the stepper motor
 */
class Stepper
{

  /**
   * The gpio handling class
   */
  protected $gpio;

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
  public function __construct($process) {
    $this->process = $process;

    $this->gpio = new GPIO(); // @todo: dependency inject
  }

  public function setOutput(OutputInterface $output) {
    $this->output = $output;
  }

  // @todo: convert this to work with less than php 5.4
  public function demo() {
    $gpio = $this->gpio; // @todo: remove this

    // BCM pin naming
    $control_pin = [24,25,8,7];

    foreach ($control_pin as $pin) {
        echo "Setting up pin $pin\n";
        $gpio->setup($pin, "out");
        $gpio->output($pin, 0);
    }

    $seq = [ [1,0,0,0],
            [1,1,0,0],
            [0,1,0,0],
            [0,1,1,0],
            [0,0,1,0],
            [0,0,1,1],
            [0,0,0,1],
            [1,0,0,1] ];


    foreach (range(0, 255) as $i) {
        foreach (range(0, 7) as $halfstep) {
            foreach (range(0, 3) as $pin) {
                $gpio->output($control_pin[$pin], $seq[$halfstep][$pin]);
            }
            usleep(100);
        }
    }

    sleep(2);

    // reverse
    foreach (range(0, 255) as $i) {
        foreach (range(7, 0) as $halfstep) {
            foreach (range(0, 3) as $pin) {
                $gpio->output($control_pin[$pin], $seq[$halfstep][$pin]);
            }
            usleep(1000);
        }
    }




    // Mmm, not the same as GPIO.cleanup()
    $gpio->unexportAll();


  }

}

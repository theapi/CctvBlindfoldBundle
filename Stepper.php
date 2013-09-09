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
     * The gpio pins to use to control the stepper.
     * BCM pin naming
     * @var array
     */
    protected $control_pins = array(24,25,8,7);

    /**
     * The stepping sequence
     * @var array
     */
    protected $seq;

    /**
     * The number of steps to perform.
     * 512 is a full turn
     * @var int
     */
    protected $steps = 80;

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

    public function open() {
        $this->setupPins();
        foreach (range(0, $this->steps) as $i) {
            foreach (range(7, 0) as $halfstep) {
                foreach (range(0, 3) as $pin) {
                    $this->gpio->output($this->control_pins[$pin], $this->seq[$halfstep][$pin]);
                }
                usleep(100);
            }
        }
    }

    public function close() {
        $this->setupPins();
        foreach (range(0, $this->steps) as $i) {
            foreach (range(0, 7) as $halfstep) {
                foreach (range(0, 3) as $pin) {
                    $this->gpio->output($this->control_pins[$pin], $this->seq[$halfstep][$pin]);
                }
                usleep(100);
            }
        }
    }

    protected function setupPins() {

        $this->setSeq();

        // Initiate the pins
        foreach ($this->control_pins as $pin) {
            $this->gpio->setup($pin, "out");
            $this->gpio->output($pin, 0);
        }
    }

    /**
     * The stepping sequence.
     */
    protected function setSeq() {
        // halfstep
        $this->seq = [ [1,0,0,0],
                       [1,1,0,0],
                       [0,1,0,0],
                       [0,1,1,0],
                       [0,0,1,0],
                       [0,0,1,1],
                       [0,0,0,1],
                       [1,0,0,1] ];
    }

    public function demo() {
      $gpio = $this->gpio;

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

      // start closed, so anticlockwise 1/4 turn
      // 512 is full turn
      $steps = 80;

      foreach (range(0, $steps) as $i) {
          foreach (range(7, 0) as $halfstep) {
              foreach (range(0, 3) as $pin) {
                  $gpio->output($control_pin[$pin], $seq[$halfstep][$pin]);
              }
              usleep(100);
          }
      }

      sleep(2);

      // now clockwise
      foreach (range(0, $steps) as $i) {
          foreach (range(0, 7) as $halfstep) {
              foreach (range(0, 3) as $pin) {
                  $gpio->output($control_pin[$pin], $seq[$halfstep][$pin]);
              }
              usleep(100);
          }
      }

      // GPIO.cleanup()
      foreach ($control_pin as $pin) {
          $gpio->output($pin, 0);
      }
      $gpio->unexportAll();


    }

}

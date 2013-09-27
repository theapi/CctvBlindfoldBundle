<?php
namespace Theapi\CctvBlindfoldBundle;

use Symfony\Component\Console\Output\OutputInterface;

/**
 * Run the stepper motor
 */
class Stepper
{

    /**
     * The OutputInterface object
     */
    protected $output;

    /**
     * A Symfony\Component\Process object
     */
    protected $process;

    /**
     * How many steps needed to open
     * @var int
     */
    protected $stepsOpen;

    /**
     * How many steps needed to close
     * @var int
     */
    protected $stepsClose;

    /**
     * Constructor
     */
    public function __construct($process, $stepsOpen = 100, $stepsClose = -100)
    {
        $this->process = $process;
        $this->stepsOpen = $stepsOpen;
        $this->stepsClose = $stepsClose;
    }

    public function setOutput(OutputInterface $output)
    {
        $this->output = $output;
    }

    public function rotate($steps)
    {
        // Call the python script which is MUCH faster than php gpio
        $cmd = 'sudo python '. __DIR__ . '/python/stepper.py ' . $steps;
        $this->process->setCommandLine($cmd);
        $this->process->run();

        if (!$this->process->isSuccessful()) {
          throw new \RuntimeException($this->process->getErrorOutput());
        }

    }

    public function open()
    {
        $this->rotate($this->stepsOpen);
    }

    public function close()
    {
        $this->rotate($this->stepsClose);
    }

    public function demo()
    {
        $this->open();
        sleep(2);
        $this->close();
    }
}

<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand;
use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\ArrayInput;
use Symfony\Component\Console\Input;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

class DaemonCommand extends ContainerAwareCommand
{

    protected function configure()
    {
        $this
            ->setName('cctvbf:daemon')
            ->setDescription('Monitor sensors and act as appropriate')
            ->setHelp('Runs continuously as a daemon.')
        ;
    }

    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $container = $this->getContainer();
        $detectCommand = $this->getApplication()->find('cctvbf:detect');
        $input = new ArrayInput(array('command' => 'detect'));
        $lastDetected = 0;

        while (true) {
            $now = time();
            //@todo configurable last detected time since.
            if ($now - $lastDetected > 120) {
                $detectCommand->run($input, $output);
                $lastDetected = $now;
            }

            //@todo Socket connect to the analogue to digital converter (adc.py)

            usleep(200000);
        }

    }
}

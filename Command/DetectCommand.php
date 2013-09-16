<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Theapi\CctvBlindfoldBundle\DeviceDetector;
use Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand;
use Symfony\Component\Console\Command\Command;
use Symfony\Component\Console\Input\ArrayInput;
use Symfony\Component\Console\Input;
use Symfony\Component\Console\Input\InputInterface;
use Symfony\Component\Console\Output\OutputInterface;

class DetectCommand extends ContainerAwareCommand
{

    protected function configure()
    {
        $this
            ->setName('cctvbf:detect')
            ->setDescription('Scan the network for attached devices')
            ->setHelp('Scan the network for attached devices.')
        ;
    }

    protected function execute(InputInterface $input, OutputInterface $output)
    {
        $container = $this->getContainer();
        //$command = $this->getApplication()->find('cctvbf:move');

        $deviceDetector = $container->get('theapi_cctvblindfold.device_detector');
        $deviceDetector->setOutput($output);
        $deviceDetector->detect();

        /*
        if ($deviceDetector->detect()) {
            // devices detected so ensure the blindfold is closed
            $input = new ArrayInput(
                array(
                    'command' => 'move',
                    'action' => 'close',
                )
            );
        } else {
            $input = new ArrayInput(
                array(
                    'command' => 'move',
                    'action' => 'open',
                )
            );
        }
        */

        //$returnCode = $command->run($input, $output);
    }
}

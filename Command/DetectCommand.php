<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Theapi\CctvBlindfoldBundle\DeviceDetector,
    Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand,
    Symfony\Component\Console\Command\Command,
    Symfony\Component\Console\Input\ArrayInput,
    Symfony\Component\Console\Input,
    Symfony\Component\Console\Input\InputInterface,
    Symfony\Component\Console\Output\OutputInterface;

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
        $command = $this->getApplication()->find('cctvbf:move');

        $deviceDetector = $container->get('theapi_cctvblindfold.device_detector');
        $deviceDetector->setOutput($output);

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

        $returnCode = $command->run($input, $output);
    }
}

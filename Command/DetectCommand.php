<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Theapi\CctvBlindfoldBundle\DeviceDetector;

use Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand;
use Symfony\Component\Console\Command\Command,
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

      $deviceDetector = $container->get('theapi_cctvblindfold.device_detector');
      $deviceDetector->setOutput($output);
      $deviceDetector->detect();
    }

}
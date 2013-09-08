<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Theapi\CctvBlindfoldBundle\Stepper;

use Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand;
use Symfony\Component\Console\Command\Command,
    Symfony\Component\Console\Input,
    Symfony\Component\Console\Input\InputInterface,
    Symfony\Component\Console\Output\OutputInterface;

class StepperCommand extends ContainerAwareCommand
{

    protected function configure()
    {
        $this
            ->setName('cctvbf:stepper:demo')
            ->setDescription('Move the stepper motor')
            ->setHelp('Move the stepper motor.')
        ;
    }

    protected function execute(InputInterface $input, OutputInterface $output)
    {
      $container = $this->getContainer();

      //$output->writeln('Not yet implemented');


      $stepper = $container->get('theapi_cctvblindfold.stepper');
      $stepper->setOutput($output);
      $stepper->demo();

    }

}
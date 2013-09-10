<?php
namespace Theapi\CctvBlindfoldBundle\Command;

use Theapi\CctvBlindfoldBundle\Stepper;

use Symfony\Bundle\FrameworkBundle\Command\ContainerAwareCommand;
use Symfony\Component\Console\Command\Command,
    Symfony\Component\Console\Input,
    Symfony\Component\Console\Input\InputInterface,
    Symfony\Component\Console\Input\InputArgument,
    Symfony\Component\Console\Input\InputOption,
    Symfony\Component\Console\Output\OutputInterface;

class BlindfoldCommand extends ContainerAwareCommand
{

    protected function configure() {
        $this
            ->setName('cctvbf:move')
            ->setDescription('Move the blindfold')
            ->addArgument(
                'action',
                InputArgument::OPTIONAL,
                '<info>demo, rotate, toggle, open or close</info>'
            )
            ->addOption(
                'steps',
                 'd',
                 InputOption::VALUE_OPTIONAL,
                 'Steps to rotate.'
            )
            ->setHelp('Defaults to toggle, but takes options to rotate, open & close.')
        ;
    }

    protected function execute(InputInterface $input, OutputInterface $output) {
      $container = $this->getContainer();

      $action = $input->getArgument('action');

      $bf = $container->get('theapi_cctvblindfold.blindfold');
      $bf->setOutput($output);

      switch ($action) {
          case 'rotate':
              $bf->rotate($input->getOption('steps'));
              break;

          case 'open':
              $bf->open();
              break;

          case 'close':
              $bf->close();
              break;

          case 'toggle':
              $bf->toggle();
              break;

          default:
              $bf->demo();
              break;
      }


    }

}

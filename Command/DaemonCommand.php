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
        $blindfold = $container->get('theapi_cctvblindfold.blindfold');
        $blindfold->setOutput($output);
        $blindfold->startInputDaemon();


        $detectCommand = $this->getApplication()->find('cctvbf:detect');
        $input = new ArrayInput(array('command' => 'detect'));
        $lastDetect = 0;

        sleep(2); // give the input daemon a chance to start
        $blindfold->connectToInputDaemon();

        while (true) {
            $now = time();
            //@todo configurable last detected time since.
            if ($now - $lastDetect > 120) {
                $detectCommand->run($input, $output);
                $lastDetect = $now;
            }

            if (!$blindfold->streamSelect()) {
              // The blindfold socket listener blocks for 200000 milliseconds
              // but not if it has no sockets to listen to
              // so sleep now.
              usleep(200000);
            }
        }
    }

}

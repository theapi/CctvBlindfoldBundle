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
        $lastDetect = 0;

        // bit rough & ready ATM Socket connect to the analogue to digital converter (adc.py)
        $fp = stream_socket_client('tcp://localhost:8889', $errno, $errstr, 30);
        if ($fp) {
          $read[] = $fp;
        }

        while (true) {
            $now = time();
            //@todo configurable last detected time since.
            if ($now - $lastDetect > 120) {
                $detectCommand->run($input, $output);
                $lastDetect = $now;
            }


            if (count($read) > 0) {
              $changed = stream_select($read, $write, $except, 0, 200000);

              foreach ($read as $stream) {
                $data = fread(stream, 8192);
                if (strlen($data) > 0) {
                  // send event
                }
              }

            } else {
              usleep(200000);
            }

        }

    }


}

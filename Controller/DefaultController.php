<?php

namespace Theapi\CctvBlindfoldBundle\Controller;

use Symfony\Component\HttpFoundation\StreamedResponse;
use Symfony\Component\HttpFoundation\BinaryFileResponse;
use Symfony\Component\HttpFoundation\ResponseHeaderBag;
use Symfony\Bundle\FrameworkBundle\Controller\Controller;

class DefaultController extends Controller
{

    public function indexAction()
    {
        if ($this->hasAccess()) {
            $blindfold = $this->get('theapi_cctvblindfold.blindfold');
            $state = $blindfold->getState();
            $word = $state ? 'CLOSE' : 'OPEN';
        } else {
            $word = ' '; // tell 'em nothing
        }
        return $this->render('TheapiCctvBlindfoldBundle:Default:index.html.twig', array('word' => $word));
    }
    
    public function toggleAction()
    {       
        try {
            if ($this->hasAccess()) {
                $blindfold = $this->get('theapi_cctvblindfold.blindfold');
                $state = $blindfold->getState();
                touch('/opt/watched/cctvbf');
                // wait for the state to change
                $count = 0;
                while ($state == $blindfold->getState()) {
                    ++$count;
                    if ($count > 60) {
                        $state = 'give up waiting';
                    }
                    sleep(1);
                }
            }
        } catch (\Exception $e) {
            // ignore error
        }
        
        return $this->redirect($this->generateUrl('theapi_cctvblindfold'));
    }

    public function hasAccess() 
    {
        // Restrict by ip
        $ip = $this->getRequest()->getClientIp();
        $access = false;
        if ($ip == '127.0.0.1') {
            $access = true;
        } else {
            $parts = explode('.', $ip);
            if ($parts[0] == '192' && $parts[1] == '168') {
                $access = true;
            } 
        }

        return $access;
    }

}

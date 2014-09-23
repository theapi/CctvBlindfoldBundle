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
        return $this->render('TheapiCctvBlindfoldBundle:Default:index.html.twig');
    }
    
    public function toggleAction()
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
        
        try {
            if ($access == true) {
                touch('/opt/watched/cctvbf');
            }
        } catch (\Exception $e) {
            // ignore error
        }
        
        return $this->redirect($this->generateUrl('theapi_cctvblindfold'));
    }

}

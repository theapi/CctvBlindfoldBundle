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
        try {
            $blindfold = $this->get('theapi_cctvblindfold.blindfold');
            $blindfold->toggle();
        } catch (\Exception $e) {
            // ignore error
        }
        
        return $this->redirect($this->generateUrl('theapi_cctvblindfold'));
    }

}

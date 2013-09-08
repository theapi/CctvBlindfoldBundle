CctvBlindfoldBundle
===================

An over engineered switch for a cctv camera using a Raspberry pi & Symfony.

Requires:
 - nmap
 - Raspberry Pi
 - Stepper motor
 - Cctv camera

Ultimately, the CctvBundle files should be downloaded to the
`vendor/theapi/cctvbundle/Theapi/CctvBlindfoldBundle` directory.

**Using composer**

Simply run assuming you have installed composer.phar or composer binary:

``` bash
$ composer require ronanguilloux/php-gpio dev-master

$ composer require theapi/cctvblindfoldbundle dev-master
```

### B) Enable the bundle

Enable the bundle in the kernel:

``` php
<?php
// app/AppKernel.php

public function registerBundles()
{
    $bundles = array(
        // ...
        new Theapi\CctvBundle\TheapiCctvBlindfoldBundle(),
    );
}


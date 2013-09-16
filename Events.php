<?php
namespace Theapi\CctvBlindfoldBundle;

/**
 * Document the events
 *
 */
final class Events
{

    /**
     * The device_detector.found event is thrown
     * when atleast one device has been detected.
     *
     * @var string
     */
    const DEVICE_DETECTOR_FOUND = 'device_detector.found';

    /**
     * The device_detector.found event is thrown
     * when none of the devices were detected.
     *
     * @var string
     */
    const DEVICE_DETECTOR_NOT_FOUND = 'device_detector.not_found';
}

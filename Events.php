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

    /**
     * The blindfold.stream_data event is thrown
     * when a connected socket reports data.
     *
     * @var string
     */
    const BLINDFOLD_STREAM_DATA = 'blindfold.stream_data';

    /**
     * The blindfold.open event is thrown
     * when the blindfold has just been opened.
     *
     * @var string
     */
    const BLINDFOLD_OPEN = 'blindfold.open';

    /**
     * The blindfold.close event is thrown
     * when the blindfold has just been closed.
     *
     * @var string
     */
    const BLINDFOLD_CLOSE = 'blindfold.close';
}

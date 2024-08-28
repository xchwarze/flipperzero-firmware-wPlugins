LIGHT_RED: int
'''
Constant value for the red LED light.
'''

LIGHT_GREEN: int
'''
Constant value for the green LED light.
'''

LIGHT_BLUE: int
'''
Constant value for the blue LED light.
'''

LIGHT_BACKLIGHT: int
'''
Constant value for the display backlight.
'''

def light_set(light: int, brightness: int) -> None:
    '''
    Control the RGB LED on your Flipper. You can also set the brightness of multiple channels at once using bitwise operations.
    The ``brightness`` parameter accepts values from 0 (light off) to 255 (very bright).

    :param light: The RGB channels to set.
    :param brightness: The brightness to use.

    :Example:
    
    .. code-block::
    
        import flipperzero as f0
        
        f0.light_set(f0.LIGHT_RED | f0.LIGHT_GREEN, 250)

    .. tip::

        You can use  up to seven colors using `additive mixing <https://en.wikipedia.org/wiki/Additive_color>`_.
    '''
    pass

def light_blink_start(light: int, brightness: int, on_time: int, period: int) -> None:
    '''
    Let the RGB LED blink. You can define the total duration of a blink period and the duration, the LED is active during a blink period.
    Hence, ``on_time`` must be smaller than ``period``. This is a non-blocking operation. The LED will continue to blink until you call :func:`light_blink_stop`.

    :param light: The RGB channels to set.
    :param brightness: The brightness to use.
    :param on_time: The LED's active duration in milliseconds.
    :param period: Total duration of a blink period in milliseconds.

    :Example:

    :Example:

    .. code-block::
    
        import flipperzero as f0
        
        f0.light_blink_start(f0.LIGHT_RED, 150, 100, 200)
    '''
    pass

def light_blink_set_color(light: int) -> None:
    '''
    Change the RGB LED's color while blinking. This is a non-blocking operation.
    Be aware, that you must start the blinking procedure first by using the :func:`light_blink_start` function.
    Call the :func:`light_blink_stop` function to stop the blinking LED.

    :param light: The RGB channels to set.
    '''
    pass

def light_blink_stop() -> None:
    '''
    Stop the blinking LED.
    '''
    pass

def vibro_set(state: bool) -> bool:
    '''
    Turn vibration on or off. This is a non-blocking operation. The vibration motor will continue to run until you stop it.

    :param state: :const:`True` to turn on vibration.
    :returns: :const:`True` if vibration is on.
    '''
    pass

def speaker_start(frequency: float, volume: float) -> bool:
    '''
    Output a steady tone of a defined frequency and volume on the Flipper's speaker.
    This is a non-blocking operation. The tone will continue until you call :func:`speaker_stop`.
    The ``volume`` parameter accepts values from 0.0 (silent) up to 1.0 (very loud).

    :param frequency: The frequency to play in `hertz <https://en.wikipedia.org/wiki/Hertz>`_.
    :param volume: The volume to use.
    :returns: :const:`True` if the speaker was acquired.

    :Example:

    .. code-block::
        
        import flipperzero as f0
        
        f0.speaker_start(50.0, 0.8)
    '''
    pass

def speaker_set_volume(volume: float) -> bool:
    '''
    Set the speaker's volume while playing a tone. This is a non-blocking operation.
    The tone will continue until you call :func:`speaker_stop`.
    The ``volume`` parameter accepts values from 0.0 (silent) up to 1.0 (very loud).
    
    :param volume: The volume to use.
    :returns: :const:`True` if the speaker was acquired.

    :Example:

    This function can be used to play `nice` sounds:

    .. code-block::

        import time
        import flipperzero as f0
        
        volume = 0.8

        f0.speaker_start(100.0, volume)

        for _ in range(0, 150):
            volume *= 0.9945679

            f0.speaker_set_volume(volume)

            time.sleep_ms(1)
        
        f0.speaker_stop()
    '''
    pass

def speaker_stop() -> bool:
    '''
    Stop the speaker output.

    :returns: :const:`True` if the speaker was successfully released.
    '''
    pass

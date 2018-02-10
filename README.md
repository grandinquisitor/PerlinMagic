# Perlin Magic

Using [WS2812b](https://learn.adafruit.com/adafruit-neopixel-uberguide/the-magic-of-neopixels) led strip with [Perlin noise](https://en.wikipedia.org/wiki/Perlin_noise) and an Arduino Nano. Any ATmega8-based microcontroller could be used without modification other than possibly with different pins chosen.

The leds are arranged inset in a logo. The logo is approximated by a circle. 3D Perlin noise is used with 2 of the dimensions interpreted as an angle on the circle. The density of the noise function is interpolated onto a color palette gradient, which is either generated randomly periodically or predefined.

A bunch of the parameters are configurable via switches and dials on the enclosure. The position of 3 flap switches are read on/off as bits 0 or 1, and combined to select a 3-bit register address (addressable space = 8 possible registers). Given a selected register, the enclosure has a rotary encoder dial, the turning of which will then mutate the value stored in that register. Different registers have different bounds, step sizes and overflow behavior defined. Given these definitions, the reset button can both reset all the register values to the factory default, or if pressed twice in quick succession, will instead fill them all with random legal values.

Last, a power switch enabled logical power-off with a nice on-off animation. This switches off the leds without physically switching primary power.

# Development notes

- The microcontroller goes into deep sleep mode based on the power switch and should consume virtually no power itself when off. However, many other components will consume power not the least of which is the quiescent current draw of the led pixels even when off. If power is to be fully minimized an N-channel power mosfet should be put between the led strip ground and common ground and should be switched by the microcontroller.
- I used completely different debouncing techniques for each type of input, essentially only out of the interest of experimentation.
- Some combinations of parameter settings create a strong flickering effect. I haven't tried to fix this.
- Floating point operations are not used at any point. This should permit greater portability across other inexpensive microcontrollers.

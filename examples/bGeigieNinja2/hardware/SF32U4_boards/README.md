Safecast Ninja v2 Arduino IDE support
=====================================

The Ninja v2 board has a design similar to that of SparkFun's promicro, Fiov3, etc.
We reuse the Fio bootload as is. The variant file is slightly modified.

Follow the instructions below to install.

The Ninja board can be accessed in the IDE through `Tools -> Board -> Safecast Ninja v2 (Leonardo)`.

-Robin (robin at safecast dot org)

SparkFun 32u4 Board Support
===========================

This contains support for the following SparkFun 32u4 based Arduino-compatible development boards:
* [ProMicro 5V](https://www.sparkfun.com/products/11098)
* [ProMicro 3.3V](https://www.sparkfun.com/products/10999)
* [LilyPadUSB](https://www.sparkfun.com/products/11190)
* [Fio v3](https://www.sparkfun.com/products/11520)
* [MakeyMakey](https://www.sparkfun.com/products/11511)
* [Safecast Ninja2](https://github.com/Safecast/Ninja2)


To add this support to your Arduino IDE, simply copy this folder into your hardware directory in one of two
places:
* the actual Arduino directory
* inside your Sketchbook directory
* **Please note: This will only work under Arduino IDE versions 1.0.1 - 1.0.5**

For version 1.5.2, please see the [Arduino
repository](https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5---3rd-party-Hardware-specification)
for more information on adding these files.  Keep in mind that version 1.5.2 is
still currently a beta IDE. 

Information on compiling and programming the bootloader can be found in the bootloader directory.

MSI Led Enabler
=====================

This is a proof of concept to get MSI keyboard light working on unix and OSX (hackintosh). It works on a MSI GT60 so I think this can activate backlight led keyboard on series GT and GE that have the same keyboard by steelseries.
The attached source is for compile on Mac if you want to compile on Unix I attached in https://www.dropbox.com/s/xtm1k0rm3d1rflw/MSI_GT_GE_Led_Enabler-master-linux.zip the modifications needed. Also you need to get dependencies:

sudo apt-get install build-essential libudev-dev libusb-1.0-0-dev libfox-1.6-dev autotools-dev

If you execute this and get "Unable to open MSI Led device." run as sudo.


Thanks to Signal11 for their HIDAPI.

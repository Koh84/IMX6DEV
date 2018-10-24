/*add to device/fsl/imx6/imx6.mk the following */
# Android infrastructures
PRODUCT_PACKAGES += \
	i2c_bar_test \
	vndk-sp


/*add the following folder to system/core*/
i2c_bar_test

/*disable the following in vendor/nxp-opensource/kernel_imx/arch/arm/boot/dts/imx6qdl-sabresd.dtsi */
/*you need to disable this because this will insert as device and prevent your user land i2c driver to communicate with the device*/
/*mag3110@0e {
		compatible = "fsl,mag3110";
		reg = <0x0e>;
		position = <1>;
		vdd-supply = <&reg_sensor>;
		vddio-supply = <&reg_sensor>;
		interrupt-parent = <&gpio3>;
		interrupts = <16 1>;
	};*/

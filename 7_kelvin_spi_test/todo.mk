/*add to device/fsl/imx6/imx6.mk the following */
# Android infrastructures
PRODUCT_PACKAGES += \
	spidev_test \
	vndk-sp


/*add the following folder to system/core*/
spi_test

/*disable the following in vendor/nxp-opensource/kernel_imx/arch/arm/boot/dts/imx6qdl-sabresd.dtsi */
/*you need to disable this because this will stop the kernel from booting*/
/*Kelvin disable due to blocking boot process
		bcmdhd_wlan_1: bcmdhd_wlan@0 {
			compatible = "android,bcmdhd_wlan";
			wlreg_on-supply = <&wlreg_on>;
		};*/


/*add the following in vendor/nxp-opensource/kernel_imx/arch/arm/boot/dts/imx6qdl-sabresd.dtsi */
&ecspi1 {
        compatible = "fsl,imx6q-ecspi";
	fsl,spi-num-chipselects = <1>;
	cs-gpios = <&gpio4 9 0>;
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_ecspi1>;
	status = "okay";
	spidev@0x00 {
        	compatible = "kelvin,spidevtest";
        	spi-max-frequency = <20000000>;
        	reg = <0>;
    	};
};

/* and comment the following in vendor/nxp-opensource/kernel_imx/arch/arm/boot/dts/imx6qdl-sabresd.dtsi*/
/* this is because the MX6QDL_PAD_KEY_COL1__UART5_TX_DATA pin is share between uart5 and ecspi1 */
/*&uart5 {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_uart5_1>;
	fsl,uart-has-rtscts;
	status = "okay";
};*/


/* in vendor/nxp-opensource/kernel_imx/drivers/spi/spidev.c
modify the following structure */

#ifdef CONFIG_OF
static const struct of_device_id spidev_dt_ids[] = {
	{ .compatible = "rohm,dh2228fv" },
	{ .compatible = "lineartechnology,ltc2488" },
	{ .compatible = "kelvin,spidevtest" },
	{},
};


have a look at the following website
https://blog.csdn.net/peixiuhui/article/details/72528512
https://blog.csdn.net/wxxgoodjob/article/details/76572346

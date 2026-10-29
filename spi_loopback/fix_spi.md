# SPI0 修改
## 编辑文件
替换文件 `device/board/unionman/unionpi_whale/kernel/drivers/dts/rk3588-erk101-lp4.dtsi` 内容为以下内容：
```
// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 *
 */

#include "dt-bindings/usb/pd.h"
#include "rk3588.dtsi"
#include "rk3588-evb.dtsi"
#include "rk3588-rk806-single-erk101.dtsi"
// #include "rk3588-ov5647.dtsi"
#include "rk3588-gc2053.dtsi"
// #include "rk3588-imx219.dtsi"

/ {
	motor_ctl:motor_ctl {
		compatible = "plusevm-motor";
		gpios = <&gpio4 RK_PA6 0>,
				<&gpio1 RK_PA2 0>,
				<&gpio4 RK_PB1 0>,
				<&gpio4 RK_PB0 0>,
				<&gpio4 RK_PB4 0>,
				<&gpio1 RK_PB3 0>,
				<&gpio3 RK_PB2 1>,
				<&gpio1 RK_PB2 0>,
				<&gpio1 RK_PB4 0>;
		status = "disabled";
	};
	clk8m: clk-8m {
        compatible = "fixed-clock";
        #clock-cells = <0>;
        clock-frequency = <8000000>;
    };

	pcie20_avdd0v85: pcie20-avdd0v85 {
		compatible = "regulator-fixed";
		regulator-name = "pcie20_avdd0v85";
		regulator-boot-on;
		regulator-always-on;
		regulator-min-microvolt = <850000>;
		regulator-max-microvolt = <850000>;
		vin-supply = <&vdd_0v85_s0>;
	};

	pcie20_avdd1v8: pcie20-avdd1v8 {
		compatible = "regulator-fixed";
		regulator-name = "pcie20_avdd1v8";
		regulator-boot-on;
		regulator-always-on;
		regulator-min-microvolt = <1800000>;
		regulator-max-microvolt = <1800000>;
		vin-supply = <&avcc_1v8_s0>;
	};

	pcie30_avdd0v75: pcie30-avdd0v75 {
		compatible = "regulator-fixed";
		regulator-name = "pcie30_avdd0v75";
		regulator-boot-on;
		regulator-always-on;
		regulator-min-microvolt = <750000>;
		regulator-max-microvolt = <750000>;
		vin-supply = <&avdd_0v75_s0>;
	};


	pcie30_avdd1v8: pcie30-avdd1v8 {
		compatible = "regulator-fixed";
		regulator-name = "pcie30_avdd1v8";
		regulator-boot-on;
		regulator-always-on;
		regulator-min-microvolt = <1800000>;
		regulator-max-microvolt = <1800000>;
		vin-supply = <&avcc_1v8_s0>;
	};


	vcc_1v1_nldo_s3: vcc-1v1-nldo-s3 {
		compatible = "regulator-fixed";
		regulator-name = "vcc_1v1_nldo_s3";
		regulator-always-on;
		regulator-boot-on;
		regulator-min-microvolt = <1100000>;
		regulator-max-microvolt = <1100000>;
		vin-supply = <&vcc5v0_sys>;
	};

	vcc5v0_host: vcc5v0-host {
		compatible = "regulator-fixed";
		regulator-name = "vcc5v0_host";
		regulator-boot-on;
		regulator-always-on;
		regulator-min-microvolt = <5000000>;
		regulator-max-microvolt = <5000000>;
		enable-active-high;
		gpio = <&gpio4 RK_PB6 GPIO_ACTIVE_HIGH>;
		vin-supply = <&vcc5v0_usb>;
		pinctrl-names = "default";
		pinctrl-0 = <&vcc5v0_host_en>;
	};
	

	vcc_3v3_sd_s0: vcc-3v3-sd-s0-regulator { 
		compatible = "regulator-fixed";
		gpio = <&gpio0 RK_PC6 GPIO_ACTIVE_HIGH>;
		pinctrl-names = "default";
		pinctrl-0 = <&sd_s0_pwr>;
		regulator-name = "vcc_3v3_sd_s0";
		enable-active-high;
	};
	
    gpio_demo: gpio-demo{
        compatible = "gpiotest,rk3588-gpio";
        status = "okay";
        pinctrl-names = "default";
        pinctrl-0 = <&pin108_128_gpio>;

        test-gpio9 = <&gpio3 RK_PB4 GPIO_ACTIVE_HIGH>;             /*GPIO3_B4*/
        test-gpio1 = <&gpio3 RK_PD0 GPIO_ACTIVE_HIGH>;             /*GPIO3_D0*/
        test-gpio2 = <&gpio3 RK_PD1 GPIO_ACTIVE_HIGH>;             /*GPIO3_D1*/
        test-gpio3 = <&gpio3 RK_PA0 GPIO_ACTIVE_HIGH>;             /*GPIO3_A0*/
        test-gpio4 = <&gpio0 RK_PD3 GPIO_ACTIVE_HIGH>;             /*GPIO0_D3*/
    };

	ws2812 {
		compatible = "worldsemi,ws2812";
		status = "okay";
		pinctrl-names = "default";
		pinctrl-0 = <&ws2812_pin>;
		data-gpios = <&gpio1 RK_PB2 GPIO_ACTIVE_HIGH>;
		num-leds = <16>;
		color-order = "GRB";
	};

};

&combphy0_ps {
	status = "okay";
};

&combphy1_ps {
	status = "okay";
};

&combphy2_psu {
	status = "okay";
};

/*
 * mipi_dcphy0 needs to be enabled
 * when dsi0 is enabled
 */
&dsi0 {
	status = "okay";
	dsi0_panel: panel@0 {
		status = "okay";
		compatible = "simple-panel-dsi";
		power-supply = <&vcc_3v3_s0>;
		reg = <0>;
		// backlight = <&backlight>;
		reset-delay-ms = <50>;
		enable-delay-ms = <10>;
		prepare-delay-ms = <10>;
		unprepare-delay-ms = <10>;
		disable-delay-ms = <10>;
		init-delay-ms = <10>;
		dsi,flags = <(MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
		 MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET)>; 
		dsi,format = <MIPI_DSI_FMT_RGB888>;
		dsi,lanes  = <2>;
		panel-init-sequence = [
			 23 01 02 7A C1
			 23 01 02 20 20
			 23 01 02 21 E0
			 23 01 02 22 13
			 23 01 02 23 28
			 23 01 02 24 30
			 23 01 02 25 28
			 23 01 02 26 00
			 23 01 02 27 0D
			 23 01 02 28 03
			 23 01 02 29 1D
			 23 01 02 34 80
			 23 01 02 36 28
			 23 01 02 86 29
			 23 01 02 B5 A0
			 23 01 02 5C FF
			 23 01 02 2A 01
			 23 01 02 56 92
			 23 01 02 6B 71
			 23 01 02 69 18
			 23 01 02 10 40
			 23 01 02 11 88
			 23 01 02 B6 20
			 23 01 02 51 20
			 23 01 02 09 10
		];

		panel-exit-sequence = [
		];

		disp_timings0: display-timings {
			native-mode = <&dsi0_timing0>;
			dsi0_timing0: timing0 {
				clock-frequency = <29232000>;
				hactive = <800>;
				vactive = <480>;

				hfront-porch = <40>;
				hsync-len = <48>;
				hback-porch = <40>;

				vfront-porch = <13>;
				vsync-len = <3>;
				vback-porch = <29>;

				hsync-active = <0>;
				vsync-active = <0>;
				de-active = <0>;
				pixelclk-active = <0>;

				swap-rb = <0>;
				swap-rg = <0>;
				swap-gb = <0>;
			};
		};

		ports {
			#address-cells = <1>;
			#size-cells = <0>;

			port@0 {
				reg = <0>;
				panel_in_dsi: endpoint {
					remote-endpoint = <&dsi_out_panel>;
				};
			};
		};
	};

	ports {
		#address-cells = <1>;
		#size-cells = <0>;

		port@1 {
			reg = <1>;
			dsi_out_panel: endpoint {
				remote-endpoint = <&panel_in_dsi>;
			};
		};
	};
};

&dsi0_in_vp2 {
	status = "okay";
};

&dsi0_in_vp3 {
	status = "okay";
};

&route_dsi0 {
	status = "okay";
	connect = <&vp3_out_dsi0>;
};

&mipi_dcphy0 {
	status = "okay";
};

&gmac1 {
	/* Use rgmii-rxid mode to disable rx delay inside Soc */
	phy-mode = "rgmii-rxid";
	clock_in_out = "output";

	snps,reset-gpio = <&gpio3 RK_PB7 GPIO_ACTIVE_LOW>;
	snps,reset-active-low;
	/* Reset time is 20ms, 100ms for rtl8211f */
	snps,reset-delays-us = <0 20000 100000>;

	pinctrl-names = "default";
	pinctrl-0 = <&gmac1_miim
		     &gmac1_tx_bus2
		     &gmac1_rx_bus2
			 //&gmac1_clkinout
		     &gmac1_rgmii_clk
		     &gmac1_rgmii_bus>;

	tx_delay = <0x43>;
	/* rx_delay = <0x3f>; */

	phy-handle = <&rgmii_phy1>;
	status = "okay";
};

&hdmi0 {
	enable-gpios = <&gpio1 RK_PA5 GPIO_ACTIVE_LOW>;
	status = "okay";
};

&hdmi0_in_vp0 {
	status = "okay";
};

&hdmi0_sound {
	status = "okay";
};

&hdptxphy_hdmi0 {
	status = "okay";
};

&route_hdmi0 {
	status = "okay";
};

&hdmi1 {
	enable-gpios = <&gpio1 RK_PA6 GPIO_ACTIVE_LOW>;
	status = "okay";
};

&hdmi1_in_vp2 {
	status = "okay";
};

&hdmi1_sound {
	status = "okay";
};

&hdptxphy_hdmi1 {
	status = "okay";
};

&route_hdmi1 {
	status = "okay";
};

&combphy0_ps {
    status = "okay";
};

&sata0 {
    status = "okay";
};

//&spi1{
//    spi_demo: spi_demo@00{
//        compatible = "firefly,rk3588-spi";
//        status = "okay";
//        reg = <0x00>;
//        spi-max-frequency = <50000000>;
        //spi-cpha;   /* SPI mode: CPHA=1 */
        //spi-cpol; 	/* SPI mode: CPOL=1 */
        //spi-cs-high;
//    };
//};

&spi0 {
	status = "okay";
	pinctrl-0 = <&spi0m2_pins>;
	pinctrl-names = "default";
	cs-gpios = <&gpio1 RK_PB4 GPIO_ACTIVE_HIGH>;
	max-freq = <5000000>;

	spidev0: spidev@0 {
		compatible = "rockchip,spidev";
		reg = <0>;
		spi-max-frequency = <10000000>;
		status = "okay";
	};
};

&i2c0 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c0m2_xfer>;

	vdd_cpu_big0_s0: vdd_cpu_big0_mem_s0: rk8602@42 {
		compatible = "rockchip,rk8602";
		reg = <0x42>;
		vin-supply = <&vcc5v0_sys>;
		regulator-compatible = "rk860x-reg";
		regulator-name = "vdd_cpu_big0_s0";
		regulator-min-microvolt = <550000>;
		regulator-max-microvolt = <1050000>;
		regulator-ramp-delay = <2300>;
		rockchip,suspend-voltage-selector = <1>;
		regulator-boot-on;
		regulator-always-on;
		regulator-state-mem {
			regulator-off-in-suspend;
		};
	};

	vdd_cpu_big1_s0: vdd_cpu_big1_mem_s0: rk8603@43 {
		compatible = "rockchip,rk8603";
		reg = <0x43>;
		vin-supply = <&vcc5v0_sys>;
		regulator-compatible = "rk860x-reg";
		regulator-name = "vdd_cpu_big1_s0";
		regulator-min-microvolt = <550000>;
		regulator-max-microvolt = <1050000>;
		regulator-ramp-delay = <2300>;
		rockchip,suspend-voltage-selector = <1>;
		regulator-boot-on;
		regulator-always-on;
		regulator-state-mem {
			regulator-off-in-suspend;
		};
	};
};

&i2c1 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c1m2_xfer>;

	vdd_npu_s0: vdd_npu_mem_s0: rk8602@42 {
		compatible = "rockchip,rk8602";
		reg = <0x42>;
		vin-supply = <&vcc5v0_sys>;
		regulator-compatible = "rk860x-reg";
		regulator-name = "vdd_npu_s0";
		regulator-min-microvolt = <550000>;
		regulator-max-microvolt = <950000>;
		regulator-ramp-delay = <2300>;
		rockchip,suspend-voltage-selector = <1>;
		regulator-boot-on;
		regulator-always-on;
		regulator-state-mem {
			regulator-off-in-suspend;
		};
	};
};

&i2c2 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c2m0_xfer>;
	i2c-scl-rising-time-ns = <300>;
	i2c-scl-falling-time-ns = <15>;

	icn6211_mcu: icn6211_mcu@45 {
		compatible = "icn6211_mcu";
		reg = <0x45>;
		status = "okay";
	};

	icn6211_ft5406: icn6211_ft5406@38 {
		compatible = "icn6211_ft5406";
		reg = <0x38>;
		status = "okay";
	}; 
		
	pca9575a: gpio@21 {
		compatible = "nxp,pca9575";
		reg = <0x21>;
		gpio-controller;
		#gpio-cells = <0x02>;
		status = "okay";
		gpio-line-names = 
			"GPIO_01", "GPIO_02", "GPIO_03", "GPIO_04",
			"GPIO_05", "GPIO_06", "GPIO_07", "GPIO_08", "GPIO_09",
			"GPIO_10", "GPIO_11", "GPIO_12", "GPIO_13", "GPIO_14",
			"GPIO_15", "GPIO_16";
		};
		
	pca9575b: gpio@23 {
		compatible = "nxp,pca9575";
		reg = <0x23>;
		gpio-controller;
		#gpio-cells = <0x02>;
		status = "okay";
		gpio-line-names = 
			"GPIO_21", "GPIO_22", "GPIO_23", "GPIO_24",
			"GPIO_25", "GPIO_26", "GPIO_27", "GPIO_28", "GPIO_29",
			"GPIO_30", "GPIO_31", "GPIO_32", "GPIO_33", "GPIO_34",
			"GPIO_35", "GPIO_36";
	};

};

&i2c6 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c6m0_xfer>;

	pcf8563: pcf8563@51 {
		compatible = "nxp,pcf8563";
		reg = <0x51>;
		#clock-cells = <0>;
		clock-frequency = <32768>;
		clock-output-names = "pcf8563";
		pinctrl-names = "default";
		pinctrl-0 = <&pcf8563_int>;
		interrupt-parent = <&gpio0>;
		interrupts = <RK_PB0 IRQ_TYPE_LEVEL_LOW>;
		wakeup-source;
	};
};


&i2c8{
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c8m2_xfer>;
    clock-frequency = <100000>;
	MLX90614@5a{
		compatible="melexis,mlx90614";
		reg=<0x5a>;
	};
};

/*&i2c5{
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&i2c2m2_xfer>;
    clock-frequency = <400000>;
};*/

&i2s0_8ch {
	status = "okay";
};

&i2s5_8ch {
	status = "okay";
};

&i2s6_8ch {
	status = "okay";
};

&i2s7_8ch {
	status = "disabled";
};

&mdio1 {
	rgmii_phy1: phy@1 {
		compatible = "ethernet-phy-ieee802.3-c22";
		reg = <0x1>;
	};
};


&pwm1{
	pinctrl-0 = <&pwm1m2_pins>;
    status = "okay";
	
};
&pwm0{
	pinctrl-0 = <&pwm0m2_pins>;
    status = "okay";
	
};

&pcie2x1l1 {
	reset-gpios = <&gpio4 RK_PA5 GPIO_ACTIVE_HIGH>;
	pinctrl-names = "default";
	pinctrl-0 = <&rtl8111_isolate>;
	status = "okay";
};

&pcie30phy {
	rockchip,pcie30-phymode = <PHY_MODE_PCIE_AGGREGATION>;
	status = "okay";
};

&pcie3x4 { 
	reset-gpios = <&gpio4 RK_PB6 GPIO_ACTIVE_HIGH>;
	// vpcie3v3-supply = <&vcc3v3_pcie30>;
	status = "disabled";
};

/* can0 */
&can0 {
        status = "okay";
        assigned-clocks = <&cru CLK_CAN0>;
        assigned-clock-rates = <200000000>;
        pinctrl-names = "default";
        pinctrl-0 = <&can1m1_pins>;
};


	pinctrl {
		ws2812_pin: ws2812-pin {
			rockchip,pins = <1 RK_PB2 0 &pcfg_pull_none>;  // 功能0 = GPIO
		};
		can1{
			can1_pins: can1-pins {
				rockchip,pins = <1 RK_PB5 RK_FUNC_GPIO &pcfg_pull_none>;
			};
		};

	pcf8563 {
		pcf8563_int: pcf8563-int {
			//rockchip,pins = <0 RK_PB0 RK_FUNC_GPIO &pcfg_pull_up>;
			rockchip,pins = <2 RK_PC5 RK_FUNC_GPIO &pcfg_pull_up>;
		};
	};

	rtl8111 {
		rtl8111_isolate: rtl8111-isolate {
			rockchip,pins = <4 RK_PA5 RK_FUNC_GPIO &pcfg_pull_up>;
		};
	};


	sdmmc {
		sd_s0_pwr: sd-s0-pwr {
			rockchip,pins = <0 RK_PC6 RK_FUNC_GPIO &pcfg_pull_up>;
		};
	};

	usb {
		vcc5v0_host_en: vcc5v0-host-en {
			rockchip,pins = <4 RK_PB6 RK_FUNC_GPIO &pcfg_pull_up>;
		};
	};

	gpio {
		pin108_128_gpio: pin108-128-gpio {
			rockchip,pins = <3 RK_PB4 0 &pcfg_pull_none>,
					<3 RK_PD0 0 &pcfg_pull_none>,
					<3 RK_PD1 0 &pcfg_pull_none>,
					<3 RK_PA0 0 &pcfg_pull_none>,
					<0 RK_PD3 0 &pcfg_pull_none>;
		};
	};
};

&sdmmc {
	status = "okay";
	vmmc-supply = <&vcc_3v3_sd_s0>;
	sd-uhs-sdr104;
};

&uart6 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart6m1_xfer>;
};

&uart7 {
	status = "okay";
	pinctrl-names = "default";
	pinctrl-0 = <&uart7m1_xfer>;
};

&uart1 {
	status = "disabled";
	pinctrl-names = "default";
	pinctrl-0 = <&uart1m1_xfer>;
};



&u2phy1_otg { 
	phy-supply = <&vcc5v0_host>; 
}; 

&u2phy1 {
	status = "okay";
};

&u2phy1_otg {
	status = "okay";
};

&usbdp_phy1 {
	status = "okay";
};

&usbdp_phy1_dp {
	status = "okay";
};

&usbdp_phy1_u3 {
	status = "okay";
};

&usbdrd3_1 {
	status = "okay";
};

&usbdrd_dwc3_1 {
	status = "okay";
};

&usbdp_phy0 {
	rockchip,dp-lane-mux = <0 1 2 3 >;
	status = "okay";
};

&usbdp_phy0_dp {
	status = "okay";
};

&usbdp_phy0_u3 {
	status = "okay";
};

&usbdrd_dwc3_0 {
	dr_mode = "otg";
	phys = <&u2phy0_otg>,<&usbdp_phy0_u3>;
	phy-names = "usb2-phy","usb3-phy";
	maximum-speed = "high-speed";
	status = "okay";
	usb-role-switch;
};
```

## 编译内核，参考命令：
```
rm -rf out/kernel/
touch device/board/unionman/unionpi_whale/kernel/build_kernel.sh
./build.sh --product-name unionpi_whale --ccache --no-prebuilt-sdk --disable-package-image
```

## 烧录
烧录固件 boot_linux.img
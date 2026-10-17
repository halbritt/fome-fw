/**
 * @file board_configuration.cpp
 * @brief Board configuration and 2007 Suzuki SV650 (K7 dual-plug) defaults for fome-mc
 */

#include "pch.h"

#include "adc_subscription.h"

#ifndef EFI_BOOTLOADER
#include "functional_sensor.h"
#include "linear_func.h"
#endif

// 4 independent high-impedance injector channels (VNLD5090TR-E, 42V active clamp)
static const brain_pin_e injPins[] = {
		Gpio::G5, // INJ1: Front Cylinder (#1) Primary Injector (SV650 ECM Pin k)
		Gpio::G6, // INJ2: Rear Cylinder (#2) Primary Injector (SV650 ECM Pin b)
		Gpio::G7, // INJ3: Front Secondary / Staged Injector
		Gpio::G8, // INJ4: Rear Secondary / Staged Injector
};

// 4 independent ignition channels (MIC4427 5V logic + FGD3040G2-F085C 400V 30A IGBT)
static const brain_pin_e ignPins[] = {
		Gpio::B3,  // IGN1 / IGBT1: Front Cylinder (#1) Primary Center Plug (SV650 ECM Pin l)
		Gpio::G15, // IGN2 / IGBT2: Rear Cylinder (#2) Primary Center Plug (SV650 ECM Pin c)
		Gpio::G14, // IGN3 / IGBT3: Cyl 3 (4-cyl) or Front Cylinder (#1) Trailing Side Plug
		Gpio::G13, // IGN4 / IGBT4: Cyl 4 (4-cyl) or Rear Cylinder (#2) Trailing Side Plug
};

static const brain_pin_e trailingPins[] = {
		Gpio::G14, // IGN3 / IGBT3: Front Cylinder (#1) Trailing Side Plug
		Gpio::G13, // IGN4 / IGBT4: Rear Cylinder (#2) Trailing Side Plug
};

static void setInjectorPins() {
	copyArrayPartial(engineConfiguration->injectionPins, injPins);
}

static void setIgnitionPins() {
	copyArrayPartial(engineConfiguration->ignitionPins, ignPins);
	copyArrayPartial(engineConfiguration->trailingCoilPins, trailingPins);
}

Gpio getCommsLedPin() {
	return Gpio::F2;
}

Gpio getRunningLedPin() {
	return Gpio::D14;
}

Gpio getWarningLedPin() {
	return Gpio::F1;
}

static void setupVbatt() {
	// 5.6k high side / 10k low side = 1.56 ratio divider on MCP6004 buffered analog inputs
	engineConfiguration->analogInputDividerCoefficient = 1.56f;

	// 82k high side / 10k low side = 9.2
	engineConfiguration->vbattDividerCoeff = (92.0f / 10.0f);

	// Battery sense on PC5 (ADC12_INP8 = EFI_ADC_15)
	engineConfiguration->vbattAdcChannel = EFI_ADC_15;

	engineConfiguration->adcVcc = 3.3f;
}

static void setupEtb() {
	// Dual TLE9201SG 6A H-Bridge drivers (used for 2x DBW throttles or 4-wire STVA stepper)
	// H-Bridge #1 (ETB1 / STVA Phase 1A-1B)
	engineConfiguration->etbIo[0].controlPin = Gpio::D12;
	engineConfiguration->etbIo[0].directionPin1 = Gpio::D10;
	engineConfiguration->etbIo[0].disablePin = Gpio::D11;
	engineConfiguration->etbIo[0].directionPin2 = Gpio::Unassigned;

	// H-Bridge #2 (ETB2 / STVA Phase 2A-2B)
	engineConfiguration->etbIo[1].controlPin = Gpio::D13;
	engineConfiguration->etbIo[1].directionPin1 = Gpio::D9;
	engineConfiguration->etbIo[1].disablePin = Gpio::D8;
	engineConfiguration->etbIo[1].directionPin2 = Gpio::Unassigned;

	// Also populate stepperDcIo so H-bridges can be selected for 4-wire bipolar stepper drive
	engineConfiguration->stepperDcIo[0] = engineConfiguration->etbIo[0];
	engineConfiguration->stepperDcIo[1] = engineConfiguration->etbIo[1];

	// TLE9201SG uses PWM + DIR + DIS (not two PWM direction wires)
	engineConfiguration->etb_use_two_wires = false;
}

static void setupStepperIdle() {
	// Onboard DRV8825PWPR bipolar stepper driver for SV650 K7 4-wire ISC idle valve
	engineConfiguration->useStepperIdle = true;
	engineConfiguration->useHbridgesToDriveIdleStepper = false;
	engineConfiguration->useRawOutputToDriveIdleStepper = false;
	engineConfiguration->idle.stepperStepPin = Gpio::B4;
	engineConfiguration->idle.stepperDirectionPin = Gpio::B7;
	engineConfiguration->stepperDirectionPinMode = OM_DEFAULT;
	engineConfiguration->stepperEnablePin = Gpio::B8;
	engineConfiguration->stepperEnablePinMode = OM_INVERTED;
	engineConfiguration->idleStepperReactionTime = 3.0f;
	engineConfiguration->idleStepperTotalSteps = 150;
}

void setBoardConfigOverrides() {
	setupVbatt();

	engineConfiguration->clt.config.bias_resistor = 2700;
	engineConfiguration->iat.config.bias_resistor = 2700;
	engineConfiguration->auxTempSensor1.config.bias_resistor = 2700;
	engineConfiguration->auxTempSensor2.config.bias_resistor = 2700;

	// CAN1 (Bus 0): External 500 kbps CAN bus on PD0/PD1
	engineConfiguration->canTxPin = Gpio::D1;
	engineConfiguration->canRxPin = Gpio::D0;

	// CAN2 (Bus 1): Internal 500 kbps Dual WBO CAN bus (+ external expansion) on PB12/PB13
	engineConfiguration->can2RxPin = Gpio::B12;
	engineConfiguration->can2TxPin = Gpio::B13;

	// Default to external 1-bar barometric sensor on J2-6 (PA2 / EFI_ADC_2) so the enclosure can be
	// flood-potted in a single pour without a barometer chimney dam or ePTFE lid vent.
	// Onboard LPS25HBTR (U24 on I2C2 PB10/PB11) is DNP on potted builds; can be enabled in TunerStudio for unpotted bench boards.
	engineConfiguration->lps25BaroSensorScl = Gpio::Unassigned;
	engineConfiguration->lps25BaroSensorSda = Gpio::Unassigned;

	// fome-mc does not use SPI4 WiFi; PE2..PE5 control WBO1/2 NRST & BOOT0 recovery lines
	engineConfiguration->is_enabled_spi_4 = false;

	// Clear SPI3 SD card pins so PC10..PC12 remain dedicated to 4-bit SDMMC1 SDNAND
	if (engineConfiguration->sdCardSpiDevice || isBrainPinValid(engineConfiguration->sdCardCsPin)) {
		engineConfiguration->is_enabled_spi_3 = false;
		engineConfiguration->spi3sckPin = Gpio::Unassigned;
		engineConfiguration->spi3misoPin = Gpio::Unassigned;
		engineConfiguration->spi3mosiPin = Gpio::Unassigned;
	}
	engineConfiguration->sdCardCsPin = Gpio::Unassigned;
	engineConfiguration->sdCardSpiDevice = SPI_NONE;

	engineConfiguration->max31855spiDevice = SPI_DEVICE_5;
}

#if EFI_BOOTLOADER
brain_pin_e getMisoPin(spi_device_e) {
	return Gpio::Unassigned;
}

brain_pin_e getMosiPin(spi_device_e) {
	return Gpio::Unassigned;
}

brain_pin_e getSckPin(spi_device_e) {
	return Gpio::Unassigned;
}
#endif // EFI_BOOTLOADER

void setBoardDefaultConfiguration() {
	setupEtb();
	setupStepperIdle();
	setInjectorPins();
	setIgnitionPins();

	// Default to 2007 Suzuki SV650 (K7 dual-plug 90-deg V-twin) baseline configuration
	// Note: FOME derives cylinder count (2) directly from firingOrder = FO_1_2
	engineConfiguration->firingOrder = FO_1_2;
	engineConfiguration->displacement = 0.645f;
	engineConfiguration->timing_offset_cylinder[0] = 0.0f;
	engineConfiguration->timing_offset_cylinder[1] = -90.0f;
	engineConfiguration->ignitionMode = IM_INDIVIDUAL_COILS;
	engineConfiguration->injectionMode = IM_SEQUENTIAL;

	// SV650 K7 24-2 VR Crank Trigger on VR 1 (PE14) + VVT_MAP_V_TWIN camless phase sync
	engineConfiguration->triggerInputPins[0] = Gpio::E14;
	engineConfiguration->triggerInputPins[1] = Gpio::Unassigned;
	engineConfiguration->trigger.type = trigger_type_e::TT_TOOTHED_WHEEL;
	engineConfiguration->trigger.customTotalToothCount = 24;
	engineConfiguration->trigger.customSkippedToothCount = 2;
	engineConfiguration->vvtMode[0] = VVT_MAP_V_TWIN;
	engineConfiguration->mapCamDetectionAnglePosition = 50.0f;

	// Primary Analog & Thermistor Inputs for SV650 K7
	engineConfiguration->map.sensor.hwChannel = EFI_ADC_0;   // AV1 (PA0, J2-5): Front Cylinder IAP/MAP #1
	engineConfiguration->baroSensor.hwChannel = EFI_ADC_2;   // AV3 (PA2, J2-6): External GM 12614970 / Delphi PS10150 1-Bar Baro Sensor
	engineConfiguration->baroSensor.type = MT_GM_1_BAR;      // 10-105 kPa across 0.0-5.0 V (init_map.cpp:62-63)
	engineConfiguration->tps1_1AdcChannel = EFI_ADC_1;       // AV2 (PA1, J2-3): Primary TPS
	engineConfiguration->tps2_1AdcChannel = EFI_ADC_3;       // AV4 (PA3, J2-4): Secondary TPS (STPS)
	engineConfiguration->clt.adcChannel = EFI_ADC_11;        // AT1 (PC1, J2-1): Engine Coolant Temp (ECT)
	engineConfiguration->iat.adcChannel = EFI_ADC_12;        // AT2 (PC2, J2-2): Intake Air Temp (IAT)

	// Auxiliary Low-Side Outputs for SV650 K7
	engineConfiguration->fuelPumpPin = Gpio::C6;             // AUX_LS1: Fuel Pump Relay (Pin X)
	engineConfiguration->fanPin = Gpio::C7;                  // AUX_LS2: Cooling Fan Relay
	engineConfiguration->tachOutputPin = Gpio::D3;           // AUX_LS4: 0-12V Tachometer (Pin Z)
	engineConfiguration->mainRelayPin = Gpio::E0;            // MAIN_RLY_LS: Main Relay Hold

	// Digital Inputs for SV650 K7
	engineConfiguration->vehicleSpeedSensorInputPin = Gpio::G0; // DIG1: Front Wheel Hall VSS
	engineConfiguration->clutchDownPin = Gpio::E11;             // DIG4: Clutch Switch (Pin 7)

	// Dual Onboard STM32F042K6T6 Wideband Controllers on CAN2 (Bus 1)
	engineConfiguration->widebandMode = WidebandMode::FOMEInternal;
	engineConfiguration->canWriteEnabled = true;
	engineConfiguration->canReadEnabled = true;
	engineConfiguration->can2BaudRate = B500KBPS;
#ifndef EFI_BOOTLOADER
	config->lambdaSensorSourceBus[0] = 1;   // CAN2 (Bus 1)
	config->lambdaSensorSourceIndex[0] = 0; // WBO1 Front (0x190/0x191, SEL1=GND, SEL2=Float)
	config->lambdaSensorSourceBus[1] = 1;   // CAN2 (Bus 1)
	config->lambdaSensorSourceIndex[1] = 1; // WBO2 Rear (0x192/0x193, SEL1=Float, SEL2=3.3V)
#endif

	engineConfiguration->isSdCardEnabled = true;
	engineConfiguration->enableSoftwareKnock = true;
}

void preHalInit() {
	// 4-bit SDMMC1 for soldered LGA-8 SLC SDNAND (MKDV1GIL-AS / MKDV4GIL-AST)
	efiSetPadMode("SDMMC", Gpio::C8, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2 | PAL_STM32_PUPDR_PULLUP);
	efiSetPadMode("SDMMC", Gpio::C9, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2 | PAL_STM32_PUPDR_PULLUP);
	efiSetPadMode("SDMMC", Gpio::C10, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2 | PAL_STM32_PUPDR_PULLUP);
	efiSetPadMode("SDMMC", Gpio::C11, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2 | PAL_STM32_PUPDR_PULLUP);
	efiSetPadMode("SDMMC", Gpio::C12, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2);
	efiSetPadMode("SDMMC", Gpio::D2, PAL_MODE_ALTERNATE(12) | PAL_STM32_OSPEED_MID2 | PAL_STM32_PUPDR_PULLUP);

	// De-assert WBO1/WBO2 BOOT0 lines (PE3, PE5 LOW) and release WBO1/WBO2 NRST lines (PE2, PE4 Open-Drain HIGH)
	efiSetPadMode("WBO1_BOOT0", Gpio::E3, PAL_MODE_OUTPUT_PUSHPULL);
	 palClearPad(GPIOE, 3);
	efiSetPadMode("WBO2_BOOT0", Gpio::E5, PAL_MODE_OUTPUT_PUSHPULL);
	palClearPad(GPIOE, 5);
	efiSetPadMode("WBO1_NRST", Gpio::E2, PAL_MODE_OUTPUT_OPENDRAIN | PAL_STM32_PUPDR_PULLUP);
	palSetPad(GPIOE, 2);
	efiSetPadMode("WBO2_NRST", Gpio::E4, PAL_MODE_OUTPUT_OPENDRAIN | PAL_STM32_PUPDR_PULLUP);
	palSetPad(GPIOE, 4);
}

#ifndef EFI_BOOTLOADER
void initBoardSensors() {
	{
		static LinearFunc mrSenseFunc;
		static FunctionalSensor mrSenseSensor(SensorType::MainRelayVoltage, MS2NT(100));

		// 82k high side / 10k low side = 9.2
		const float mrSenseRatio = (92.0f / 10.0f);

		mrSenseFunc.configure(0, 0, 1, mrSenseRatio, 0, 50);
		mrSenseSensor.setFunction(mrSenseFunc);
		AdcSubscription::SubscribeSensor(mrSenseSensor, EFI_ADC_16, /*bandwidth*/ 20, /*ratio*/ 1);
		mrSenseSensor.Register();
	}

	{
		static LinearFunc sensor5vFunc;
		static FunctionalSensor sensor5vSensor(SensorType::Sensor5vVoltage, MS2NT(100));

		const float sensor5vRatio = 2.0f;

		sensor5vFunc.configure(0, 0, 1, sensor5vRatio, 0, 50);
		sensor5vSensor.setFunction(sensor5vFunc);
		AdcSubscription::SubscribeSensor(sensor5vSensor, EFI_ADC_17, /*bandwidth*/ 20, /*ratio*/ 1);
		sensor5vSensor.Register();
	}

	{
		static LinearFunc vboostFunc;
		static FunctionalSensor vboostSensor(SensorType::VboostVoltage, MS2NT(100));

		// 82k high side / 10k low side = 9.2
		const float vboostRatio = (92.0f / 10.0f);

		vboostFunc.configure(0, 0, 1, vboostRatio, 0, 50);
		vboostSensor.setFunction(vboostFunc);
		AdcSubscription::SubscribeSensor(vboostSensor, EFI_ADC_18, /*bandwidth*/ 20, /*ratio*/ 1);
		vboostSensor.Register();
	}
}
#endif

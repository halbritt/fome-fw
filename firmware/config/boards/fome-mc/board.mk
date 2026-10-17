# fome-mc: Integrated FOME Motorcycle ECU (STM32H743 / STM32H753 + Dual STM32F042 WBO)
PROJECT_CPU = ARCH_STM32H7

# List of all the board related files.
BOARDCPPSRC = $(BOARD_DIR)/board_configuration.cpp
DDEFS += -DEFI_MAIN_RELAY_CONTROL=TRUE

DDEFS += -DLED_CRITICAL_ERROR_BRAIN_PIN=Gpio::F0
DDEFS += -DFIRMWARE_ID=\"fome-mc\"

# Software knock on ADC3 (PF4 = Ch1, PF5 = Ch2, 250 kHz w/ 4x oversampling)
DDEFS += -DEFI_SOFTWARE_KNOCK=TRUE -DSTM32_ADC_USE_ADC3=TRUE

# 4-bit SDMMC1 for soldered LGA-8 32Gbit (4GB) SDNAND (CSNP32GCR01-AOW C2841139)
DDEFS += -DEFI_SDC_DEVICE=SDCD1 -DEFI_MAX_31855=TRUE

# Active 32.768 kHz LSE oscillator runs in bypass mode
DDEFS += -DSTM32_LSE_BYPASS

# Inherit STM32H7 Atlas ADC1/2/3 & clock setup while identifying fome-mc
DDEFS += -DHW_ATLAS=1 -DHW_FOME_MC=1 -DHW_POLYGONUS_PRESETS=1

# Onboard ATWINC1500-MR210UB (U.FL) Wi-Fi on SPI4 (PE2..PE6 + PC13; WBO1/2 NRST/BOOT0 on PD4..PD7)
USE_WIFI = yes

# Enable SD/SDNAND card bootloader
SD_BOOTLOADER = yes

SHORT_BOARD_NAME = fome-mc

#include "pch.h"

#if EFI_FILE_LOGGING && EFI_PROD_CODE

#include "mmc_card.h"
#include "dma_buffers.h"

#include "ff.h"
#include "fatfs_drives.h"
#include "mass_storage_init.h"

static SdCardState sdCardState = SdCardState::Initializing;
static BaseBlockDevice* mountedCardBlockDevice = nullptr;

SdCardState getSdCardState() {
	return sdCardState;
}

#if HAL_USE_USB_MSD
static chibios_rt::BinarySemaphore usbConnectedSemaphore(/* taken =*/true);
static volatile bool usbConfiguredFlag = false;

void onUsbConnectedNotifyMmcI() {
	usbConfiguredFlag = true;
	usbConnectedSemaphore.signalI();
}

bool pollUsbHandOffRequested() {
	// Only permit handoff from internal FatFs logging to USB Mass Storage when USB is
	// configured AND the engine is completely stopped (RPM == 0).
	if (!usbConfiguredFlag) {
		return false;
	}
	return engine->rpmCalculator.isStopped();
}
#else
bool pollUsbHandOffRequested() {
	return false;
}
#endif /* HAL_USE_USB_MSD */

bool handOffSdToUsbMassStorage() {
#if HAL_USE_USB_MSD
	if (sdCardState != SdCardState::Mounted || !mountedCardBlockDevice) {
		return false;
	}
	if (!engine->rpmCalculator.isStopped()) {
		return false;
	}

	// 1. Unregister FatFs work area and detach drive before exposing block device to USB host
	f_mount(nullptr, nullptr, 0);
	fatfsAttachDrive(FATFS_DRIVE_SD, nullptr);

#if EFI_TUNER_STUDIO
	engine->outputChannels.sd_logging_internal = false;
#endif

	// 2. Transfer exclusive block device ownership to USB Mass Storage LUN 1
	attachMsdSdCard(mountedCardBlockDevice);
	sdCardState = SdCardState::UsbMassStorage;
	efiPrintf("SD: engine stopped and USB connected; unmounted FatFs and handed card to USB Mass Storage");
	return true;
#else
	return false;
#endif
}

bool mountSdFilesystem() {
	auto cardBlockDevice = initializeMmcBlockDevice();
	mountedCardBlockDevice = cardBlockDevice;

#if EFI_TUNER_STUDIO
	// If not null, card is present
	engine->outputChannels.sd_present = cardBlockDevice != nullptr;
#endif

	// if no card, don't try to mount FS
	if (!cardBlockDevice) {
		efiPrintf("SD: no card detected");
		sdCardState = SdCardState::NotPresent;
		return false;
	}

#if HAL_USE_USB_MSD
	// Wait for the USB stack to wake up, or a 5 second timeout, whichever occurs first
	msg_t usbResult = usbConnectedSemaphore.wait(TIME_MS2I(5000));

	bool hasUsb = (usbResult == MSG_OK) || usbConfiguredFlag;

	// If we have a device AND USB is connected AND the engine is stopped, mount the card to USB.
	// If the engine is cranking or running, keep internal logging active and defer USB MSD handoff.
	if (cardBlockDevice && hasUsb && engine->rpmCalculator.isStopped()) {
		efiPrintf("SD: USB connected with engine stopped, card handed to mass storage instead of internal logging");

		// Mount the real card to USB
		attachMsdSdCard(cardBlockDevice);
		sdCardState = SdCardState::UsbMassStorage;

		// At this point we're done: don't try to write files ourselves
		return false;
	}
#endif

	// We were able to connect the SD card, mount the filesystem
	fatfsAttachDrive(FATFS_DRIVE_SD, cardBlockDevice);
	FRESULT err = f_mount(dma_buffers::fs(), "/", 1);
	if (err == FR_OK) {
		efiPrintf("SD card mounted!");
		sdCardState = SdCardState::Mounted;
		return true;
	} else {
		// FatFs error number, not just "it didn't work": FR_NO_FILESYSTEM (13) means the card
		// needs formatting, while FR_DISK_ERR (1) points at the card or the bus instead.
		efiPrintf("SD card failed to mount filesystem, FatFs error %d", err);
		sdCardState = SdCardState::Failed;
		return false;
	}
}

void unmountSdFilesystem() {
	if (sdCardState != SdCardState::Mounted) {
		efiPrintf("Error: No File system is mounted. \"mountsd\" first");
		return;
	}

	sdCardState = SdCardState::Failed;

	// Unmount the volume
	f_mount(nullptr, nullptr, 0); // FatFs: Unregister work area prior to discard it
	fatfsAttachDrive(FATFS_DRIVE_SD, nullptr);

	stopMmcBlockDevice();
	mountedCardBlockDevice = nullptr;

	efiPrintf("MMC/SD card removed");
}

#endif // EFI_FILE_LOGGING && EFI_PROD_CODE

#pragma once

#include "disk.h"
#include <stdbool.h>
#include <stdint.h>

/*
this struct is "borrrowed" from the windows driver for ata
info is here:
https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ata/ns-ata-_identify_device_data
*/
typedef struct {
    struct {
        uint16_t Reserved1 : 1;
        uint16_t Retired3 : 1;
        uint16_t ResponseIncomplete : 1;
        uint16_t Retired2 : 3;
        uint16_t FixedDevice : 1;    // obsolete
        uint16_t RemovableMedia : 1; // obsolete
        uint16_t Retired1 : 7;
        uint16_t DeviceType : 1;
    } GeneralConfiguration; // word 0

    uint16_t NumCylinders;          // word 1, obsolete
    uint16_t SpecificConfiguration; // word 2
    uint16_t NumHeads;              // word 3, obsolete
    uint16_t Retired1[2];
    uint16_t NumSectorsPerTrack; // word 6, obsolete
    uint16_t VendorUnique1[3];
    char SerialNumber[20]; // word 10-19
    uint16_t Retired2[2];
    uint16_t Obsolete1;
    char FirmwareRevision[8];     // word 23-26
    char ModelNumber[40];         // word 27-46
    uint8_t MaximumBlockTransfer; // word 47. 01h-10h = Maximum number of sectors that shall be transferred per interrupt on READ/WRITE MULTIPLE commands
    uint8_t VendorUnique2;

    struct {
        uint16_t FeatureSupported : 1;
        uint16_t Reserved : 15;
    } TrustedComputing; // word 48

    struct {
        uint8_t CurrentLongPhysicalSectorAlignment : 2;
        uint8_t ReservedByte49 : 6;

        uint8_t DmaSupported : 1;
        uint8_t LbaSupported : 1; // Shall be set to one to indicate that LBA is supported.
        uint8_t IordyDisable : 1;
        uint8_t IordySupported : 1;
        uint8_t Reserved1 : 1; // Reserved for the IDENTIFY PACKET DEVICE command
        uint8_t StandybyTimerSupport : 1;
        uint8_t Reserved2 : 2; // Reserved for the IDENTIFY PACKET DEVICE command

        uint16_t ReservedWord50;
    } Capabilities; // word 49-50

    uint16_t ObsoleteWords51[2];

    uint16_t TranslationFieldsValid : 3; // word 53, bit 0 - Obsolete; bit 1 - words 70:64 valid; bit 2; word 88 valid
    uint16_t Reserved3 : 5;
    uint16_t FreeFallControlSensitivity : 8;

    uint16_t NumberOfCurrentCylinders; // word 54, obsolete
    uint16_t NumberOfCurrentHeads;     // word 55, obsolete
    uint16_t CurrentSectorsPerTrack;   // word 56, obsolete
    uint32_t CurrentSectorCapacity;    // word 57, word 58, obsolete

    uint8_t CurrentMultiSectorSetting; // word 59
    uint8_t MultiSectorSettingValid : 1;
    uint8_t ReservedByte59 : 3;
    uint8_t SanitizeFeatureSupported : 1;
    uint8_t CryptoScrambleExtCommandSupported : 1;
    uint8_t OverwriteExtCommandSupported : 1;
    uint8_t BlockEraseExtCommandSupported : 1;

    uint32_t Max28BitLBA; // word 60-61, for 28-bit commands

    uint16_t ObsoleteWord62;

    uint16_t MultiWordDMASupport : 8; // word 63
    uint16_t MultiWordDMAActive : 8;

    uint16_t AdvancedPIOModes : 8; // word 64. bit 0:1 - PIO mode supported
    uint16_t ReservedByte64 : 8;

    uint16_t MinimumMWXferCycleTime;     // word 65
    uint16_t RecommendedMWXferCycleTime; // word 66
    uint16_t MinimumPIOCycleTime;        // word 67
    uint16_t MinimumPIOCycleTimeIORDY;   // word 68

    struct {
        uint16_t Reserved : 2;
        uint16_t NonVolatileWriteCache : 1; // All write cache is non-volatile
        uint16_t ExtendedUserAddressableSectorsSupported : 1;
        uint16_t DeviceEncryptsAllUserData : 1;
        uint16_t ReadZeroAfterTrimSupported : 1;
        uint16_t Optional28BitCommandsSupported : 1;
        uint16_t IEEE1667 : 1; // Reserved for IEEE 1667
        uint16_t DownloadMicrocodeDmaSupported : 1;
        uint16_t SetMaxSetPasswordUnlockDmaSupported : 1;
        uint16_t WriteBufferDmaSupported : 1;
        uint16_t ReadBufferDmaSupported : 1;
        uint16_t DeviceConfigIdentifySetDmaSupported : 1; // obsolete
        uint16_t LPSAERCSupported : 1;                    // Long Physical Sector Alignment Error Reporting Control is supported.
        uint16_t DeterministicReadAfterTrimSupported : 1;
        uint16_t CFastSpecSupported : 1;
    } AdditionalSupported; // word 69

    uint16_t ReservedWords70[5]; // word 70 - reserved
                                 // word 71:74 - Reserved for the IDENTIFY PACKET DEVICE command

    // Word 75
    uint16_t QueueDepth : 5; //  Maximum queue depth - 1
    uint16_t ReservedWord75 : 11;

    struct {
        // Word 76
        uint16_t Reserved0 : 1; // shall be set to 0
        uint16_t SataGen1 : 1;  // Supports SATA Gen1 Signaling Speed (1.5Gb/s)
        uint16_t SataGen2 : 1;  // Supports SATA Gen2 Signaling Speed (3.0Gb/s)
        uint16_t SataGen3 : 1;  // Supports SATA Gen3 Signaling Speed (6.0Gb/s)

        uint16_t Reserved1 : 4;

        uint16_t NCQ : 1;       // Supports the NCQ feature set
        uint16_t HIPM : 1;      // Supports HIPM
        uint16_t PhyEvents : 1; // Supports the SATA Phy Event Counters log
        uint16_t NcqUnload : 1; // Supports Unload while NCQ commands are outstanding

        uint16_t NcqPriority : 1;  // Supports NCQ priority information
        uint16_t HostAutoPS : 1;   // Supports Host Automatic Partial to Slumber transitions
        uint16_t DeviceAutoPS : 1; // Supports Device Automatic Partial to Slumber transitions
        uint16_t ReadLogDMA : 1;   // Supports READ LOG DMA EXT as equivalent to READ LOG EXT

        // Word 77
        uint16_t Reserved2 : 1;    // shall be set to 0
        uint16_t CurrentSpeed : 3; // Coded value indicating current negotiated Serial ATA signal speed

        uint16_t NcqStreaming : 1;   // Supports NCQ Streaming
        uint16_t NcqQueueMgmt : 1;   // Supports NCQ Queue Management Command
        uint16_t NcqReceiveSend : 1; // Supports RECEIVE FPDMA QUEUED and SEND FPDMA QUEUED commands
        uint16_t DEVSLPtoReducedPwrState : 1;

        uint16_t Reserved3 : 8;
    } SerialAtaCapabilities;

    // Word 78
    struct {
        uint16_t Reserved0 : 1;            // shall be set to 0
        uint16_t NonZeroOffsets : 1;       // Device supports non-zero buffer offsets in DMA Setup FIS
        uint16_t DmaSetupAutoActivate : 1; // Device supports DMA Setup auto-activation
        uint16_t DIPM : 1;                 // Device supports DIPM

        uint16_t InOrderData : 1;                  // Device supports in-order data delivery
        uint16_t HardwareFeatureControl : 1;       // Hardware Feature Control is supported
        uint16_t SoftwareSettingsPreservation : 1; // Device supports Software Settings Preservation
        uint16_t NCQAutosense : 1;                 // Supports NCQ Autosense

        uint16_t DEVSLP : 1;            // Device supports link power state - device sleep
        uint16_t HybridInformation : 1; // Device supports Hybrid Information Feature (If the device does not support NCQ (word 76 bit 8 is 0), then this bit shall be cleared to 0.)

        uint16_t Reserved1 : 6;
    } SerialAtaFeaturesSupported;

    // Word 79
    struct {
        uint16_t Reserved0 : 1;            // shall be set to 0
        uint16_t NonZeroOffsets : 1;       // Non-zero buffer offsets in DMA Setup FIS enabled
        uint16_t DmaSetupAutoActivate : 1; // DMA Setup auto-activation optimization enabled
        uint16_t DIPM : 1;                 // DIPM enabled

        uint16_t InOrderData : 1;                  // In-order data delivery enabled
        uint16_t HardwareFeatureControl : 1;       // Hardware Feature Control is enabled
        uint16_t SoftwareSettingsPreservation : 1; // Software Settings Preservation enabled
        uint16_t DeviceAutoPS : 1;                 // Device Automatic Partial to Slumber transitions enabled

        uint16_t DEVSLP : 1;            // link power state - device sleep is enabled
        uint16_t HybridInformation : 1; // Hybrid Information Feature is enabled

        uint16_t Reserved1 : 6;
    } SerialAtaFeaturesEnabled;

    uint16_t MajorRevision; // word 80. bit 5 - supports ATA5; bit 6 - supports ATA6; bit 7 - supports ATA7; bit 8 - supports ATA8-ACS; bit 9 - supports ACS-2;
    uint16_t MinorRevision; // word 81. T13 minior version number

    struct {

        //
        // Word 82
        //
        uint16_t SmartCommands : 1;         // The SMART feature set is supported
        uint16_t SecurityMode : 1;          // The Security feature set is supported
        uint16_t RemovableMediaFeature : 1; // obsolete
        uint16_t PowerManagement : 1;       // shall be set to 1
        uint16_t Reserved1 : 1;             // PACKET feature set, set to 0 indicates not supported for ATA devices (only support for ATAPI devices)
        uint16_t WriteCache : 1;            // The volatile write cache is supported
        uint16_t LookAhead : 1;             // Read look-ahead is supported
        uint16_t ReleaseInterrupt : 1;      // obsolete
        uint16_t ServiceInterrupt : 1;      // obsolete
        uint16_t DeviceReset : 1;           // Shall be cleared to zero to indicate that the DEVICE RESET command is not supported
        uint16_t HostProtectedArea : 1;     // obsolete
        uint16_t Obsolete1 : 1;
        uint16_t WriteBuffer : 1; // The WRITE BUFFER command is supported
        uint16_t ReadBuffer : 1;  // The READ BUFFER command is supported
        uint16_t Nop : 1;         // The NOP command is supported
        uint16_t Obsolete2 : 1;

        //
        // Word 83
        //
        uint16_t DownloadMicrocode : 1; // The DOWNLOAD MICROCODE command is supported
        uint16_t DmaQueued : 1;         // obsolete
        uint16_t Cfa : 1;               // The CFA feature set is supported
        uint16_t AdvancedPm : 1;        // The APM feature set is supported
        uint16_t Msn : 1;               // obsolete
        uint16_t PowerUpInStandby : 1;  // The PUIS feature set is supported
        uint16_t ManualPowerUp : 1;     // SET FEATURES subcommand is required to spin-up after power-up
        uint16_t Reserved2 : 1;
        uint16_t SetMax : 1;              // obsolete
        uint16_t Acoustics : 1;           // obsolete
        uint16_t BigLba : 1;              // The 48-bit Address feature set is supported
        uint16_t DeviceConfigOverlay : 1; // obsolete
        uint16_t FlushCache : 1;          // Shall be set to one to indicate that the mandatory FLUSH CACHE command is supported
        uint16_t FlushCacheExt : 1;       // The FLUSH CACHE EXT command is supported
        uint16_t WordValid83 : 2;         // shall be 01b

        //
        // Word 84
        //
        uint16_t SmartErrorLog : 1;        // SMART error logging is supported
        uint16_t SmartSelfTest : 1;        // The SMART self-test is supported
        uint16_t MediaSerialNumber : 1;    // Media serial number is supported
        uint16_t MediaCardPassThrough : 1; // obsolete
        uint16_t StreamingFeature : 1;     // The Streaming feature set is supported
        uint16_t GpLogging : 1;            // The GPL feature set is supported
        uint16_t WriteFua : 1;             // The WRITE DMA FUA EXT and WRITE MULTIPLE FUA EXT commands are supported
        uint16_t WriteQueuedFua : 1;       // obsolete
        uint16_t WWN64Bit : 1;             // The 64-bit World wide name is supported
        uint16_t URGReadStream : 1;        // obsolete
        uint16_t URGWriteStream : 1;       // obsolete
        uint16_t ReservedForTechReport : 2;
        uint16_t IdleWithUnloadFeature : 1; // The IDLE IMMEDIATE command with UNLOAD feature is supported
        uint16_t WordValid : 2;             // shall be 01b

    } CommandSetSupport;

    struct {

        //
        // Word 85
        //
        uint16_t SmartCommands : 1;         // The SMART feature set is enabled
        uint16_t SecurityMode : 1;          // The Security feature set is enabled
        uint16_t RemovableMediaFeature : 1; // obsolete
        uint16_t PowerManagement : 1;       // Shall be set to one to indicate that the mandatory Power Management feature set is supported
        uint16_t Reserved1 : 1;             // Shall be cleared to zero to indicate that the PACKET feature set is not supported
        uint16_t WriteCache : 1;            // The volatile write cache is enabled
        uint16_t LookAhead : 1;             // Read look-ahead is enabled
        uint16_t ReleaseInterrupt : 1;      // The release interrupt is enabled
        uint16_t ServiceInterrupt : 1;      // The SERVICE interrupt is enabled
        uint16_t DeviceReset : 1;           // Shall be cleared to zero to indicate that the DEVICE RESET command is not supported
        uint16_t HostProtectedArea : 1;     // obsolete
        uint16_t Obsolete1 : 1;
        uint16_t WriteBuffer : 1; // The WRITE BUFFER command is supported
        uint16_t ReadBuffer : 1;  // The READ BUFFER command is supported
        uint16_t Nop : 1;         // The NOP command is supported
        uint16_t Obsolete2 : 1;

        //
        // Word 86
        //
        uint16_t DownloadMicrocode : 1; // The DOWNLOAD MICROCODE command is supported
        uint16_t DmaQueued : 1;         // obsolete
        uint16_t Cfa : 1;               // The CFA feature set is supported
        uint16_t AdvancedPm : 1;        // The APM feature set is enabled
        uint16_t Msn : 1;               // obsolete
        uint16_t PowerUpInStandby : 1;  // The PUIS feature set is enabled
        uint16_t ManualPowerUp : 1;     // SET FEATURES subcommand is required to spin-up after power-up
        uint16_t Reserved2 : 1;
        uint16_t SetMax : 1;              // obsolete
        uint16_t Acoustics : 1;           // obsolete
        uint16_t BigLba : 1;              // The 48-bit Address features set is supported
        uint16_t DeviceConfigOverlay : 1; // obsolete
        uint16_t FlushCache : 1;          // FLUSH CACHE command supported
        uint16_t FlushCacheExt : 1;       // FLUSH CACHE EXT command supported
        uint16_t Resrved3 : 1;
        uint16_t Words119_120Valid : 1; // Words 119..120 are valid

        //
        // Word 87
        //
        uint16_t SmartErrorLog : 1;        // SMART error logging is supported
        uint16_t SmartSelfTest : 1;        // SMART self-test supported
        uint16_t MediaSerialNumber : 1;    // Media serial number is valid
        uint16_t MediaCardPassThrough : 1; // obsolete
        uint16_t StreamingFeature : 1;     // obsolete
        uint16_t GpLogging : 1;            // The GPL feature set is supported
        uint16_t WriteFua : 1;             // The WRITE DMA FUA EXT and WRITE MULTIPLE FUA EXT commands are supported
        uint16_t WriteQueuedFua : 1;       // obsolete
        uint16_t WWN64Bit : 1;             // The 64-bit World wide name is supported
        uint16_t URGReadStream : 1;        // obsolete
        uint16_t URGWriteStream : 1;       // obsolete
        uint16_t ReservedForTechReport : 2;
        uint16_t IdleWithUnloadFeature : 1; // The IDLE IMMEDIATE command with UNLOAD FEATURE is supported
        uint16_t Reserved4 : 2;             // bit 14 shall be set to 1; bit 15 shall be cleared to 0

    } CommandSetActive;

    uint16_t UltraDMASupport : 8; // word 88. bit 0 - UDMA mode 0 is supported ... bit 6 - UDMA mode 6 and below are supported
    uint16_t UltraDMAActive : 8;  // word 88. bit 8 - UDMA mode 0 is selected ... bit 14 - UDMA mode 6 is selected

    struct { // word 89
        uint16_t TimeRequired : 15;
        uint16_t ExtendedTimeReported : 1;
    } NormalSecurityEraseUnit;

    struct { // word 90
        uint16_t TimeRequired : 15;
        uint16_t ExtendedTimeReported : 1;
    } EnhancedSecurityEraseUnit;

    uint16_t CurrentAPMLevel : 8; // word 91
    uint16_t ReservedWord91 : 8;

    uint16_t MasterPasswordID; // word 92. Master Password Identifier

    uint16_t HardwareResetResult; // word 93

    uint16_t CurrentAcousticValue : 8; // word 94. obsolete
    uint16_t RecommendedAcousticValue : 8;

    uint16_t StreamMinRequestSize;         // word 95
    uint16_t StreamingTransferTimeDMA;     // word 96
    uint16_t StreamingAccessLatencyDMAPIO; // word 97
    uint32_t StreamingPerfGranularity;     // word 98, 99

    uint64_t Max48BitLBA; // word 100-103

    uint16_t StreamingTransferTime; // word 104. Streaming Transfer Time - PIO

    uint16_t DsmCap; // word 105

    struct {
        uint16_t LogicalSectorsPerPhysicalSector : 4; // n power of 2: logical sectors per physical sector
        uint16_t Reserved0 : 8;
        uint16_t LogicalSectorLongerThan256Words : 1;
        uint16_t MultipleLogicalSectorsPerPhysicalSector : 1;
        uint16_t Reserved1 : 2;  // bit 14 - shall be set to  1; bit 15 - shall be clear to 0
    } PhysicalLogicalSectorSize; // word 106

    uint16_t InterSeekDelay;                 // word 107.     Inter-seek delay for ISO 7779 standard acoustic testing
    uint16_t WorldWideName[4];               // words 108-111
    uint16_t ReservedForWorldWideName128[4]; // words 112-115
    uint16_t ReservedForTlcTechnicalReport;  // word 116
    uint16_t WordsPerLogicalSector[2];       // words 117-118 Logical sector size (DWord)

    struct {
        uint16_t ReservedForDrqTechnicalReport : 1;
        uint16_t WriteReadVerify : 1;         // The Write-Read-Verify feature set is supported
        uint16_t WriteUncorrectableExt : 1;   // The WRITE UNCORRECTABLE EXT command is supported
        uint16_t ReadWriteLogDmaExt : 1;      // The READ LOG DMA EXT and WRITE LOG DMA EXT commands are supported
        uint16_t DownloadMicrocodeMode3 : 1;  // Download Microcode mode 3 is supported
        uint16_t FreefallControl : 1;         // The Free-fall Control feature set is supported
        uint16_t SenseDataReporting : 1;      // Sense Data Reporting feature set is supported
        uint16_t ExtendedPowerConditions : 1; // Extended Power Conditions feature set is supported
        uint16_t Reserved0 : 6;
        uint16_t WordValid : 2; // shall be 01b
    } CommandSetSupportExt;     // word 119

    struct {
        uint16_t ReservedForDrqTechnicalReport : 1;
        uint16_t WriteReadVerify : 1;         // The Write-Read-Verify feature set is enabled
        uint16_t WriteUncorrectableExt : 1;   // The WRITE UNCORRECTABLE EXT command is supported
        uint16_t ReadWriteLogDmaExt : 1;      // The READ LOG DMA EXT and WRITE LOG DMA EXT commands are supported
        uint16_t DownloadMicrocodeMode3 : 1;  // Download Microcode mode 3 is supported
        uint16_t FreefallControl : 1;         // The Free-fall Control feature set is enabled
        uint16_t SenseDataReporting : 1;      // Sense Data Reporting feature set is enabled
        uint16_t ExtendedPowerConditions : 1; // Extended Power Conditions feature set is enabled
        uint16_t Reserved0 : 6;
        uint16_t Reserved1 : 2; // bit 14 - shall be set to  1; bit 15 - shall be clear to 0
    } CommandSetActiveExt;      // word 120

    uint16_t ReservedForExpandedSupportandActive[6];

    uint16_t MsnSupport : 2; // word 127. obsolete
    uint16_t ReservedWord127 : 14;

    struct { // word 128
        uint16_t SecuritySupported : 1;
        uint16_t SecurityEnabled : 1;
        uint16_t SecurityLocked : 1;
        uint16_t SecurityFrozen : 1;
        uint16_t SecurityCountExpired : 1;
        uint16_t EnhancedSecurityEraseSupported : 1;
        uint16_t Reserved0 : 2;
        uint16_t SecurityLevel : 1; // Master Password Capability: 0 = High, 1 = Maximum
        uint16_t Reserved1 : 7;
    } SecurityStatus;

    uint16_t ReservedWord129[31]; // word 129...159. Vendor specific

    struct { // word 160
        uint16_t MaximumCurrentInMA : 12;
        uint16_t CfaPowerMode1Disabled : 1;
        uint16_t CfaPowerMode1Required : 1;
        uint16_t Reserved0 : 1;
        uint16_t Word160Supported : 1;
    } CfaPowerMode1;

    uint16_t ReservedForCfaWord161[7]; // Words 161-167

    uint16_t NominalFormFactor : 4; // Word 168
    uint16_t ReservedWord168 : 12;

    struct { // Word 169
        uint16_t SupportsTrim : 1;
        uint16_t Reserved0 : 15;
    } DataSetManagementFeature;

    uint16_t AdditionalProductID[4]; // Words 170-173

    uint16_t ReservedForCfaWord174[2]; // Words 174-175

    uint16_t CurrentMediaSerialNumber[30]; // Words 176-205

    struct {                                        // Word 206
        uint16_t Supported : 1;                     // The SCT Command Transport is supported
        uint16_t Reserved0 : 1;                     // obsolete
        uint16_t WriteSameSuported : 1;             // The SCT Write Same command is supported
        uint16_t ErrorRecoveryControlSupported : 1; // The SCT Error Recovery Control command is supported
        uint16_t FeatureControlSuported : 1;        // The SCT Feature Control command is supported
        uint16_t DataTablesSuported : 1;            // The SCT Data Tables command is supported
        uint16_t Reserved1 : 6;
        uint16_t VendorSpecific : 4;
    } SCTCommandTransport;

    uint16_t ReservedWord207[2]; // Words 207-208

    struct { // Word 209
        uint16_t AlignmentOfLogicalWithinPhysical : 14;
        uint16_t Word209Supported : 1; // shall be set to 1
        uint16_t Reserved0 : 1;        // shall be cleared to 0
    } BlockAlignment;

    uint16_t WriteReadVerifySectorCountMode3Only[2]; // Words 210-211
    uint16_t WriteReadVerifySectorCountMode2Only[2]; // Words 212-213

    struct {
        uint16_t NVCachePowerModeEnabled : 1;
        uint16_t Reserved0 : 3;
        uint16_t NVCacheFeatureSetEnabled : 1;
        uint16_t Reserved1 : 3;
        uint16_t NVCachePowerModeVersion : 4;
        uint16_t NVCacheFeatureSetVersion : 4;
    } NVCacheCapabilities;   // Word 214. obsolete
    uint16_t NVCacheSizeLSW; // Word 215. obsolete
    uint16_t NVCacheSizeMSW; // Word 216. obsolete

    uint16_t NominalMediaRotationRate; // Word 217; value 0001h means non-rotating media.

    uint16_t ReservedWord218; // Word 218

    struct {
        uint8_t NVCacheEstimatedTimeToSpinUpInSeconds;
        uint8_t Reserved;
    } NVCacheOptions; // Word 219. obsolete

    uint16_t WriteReadVerifySectorCountMode : 8; // Word 220. Write-Read-Verify feature set current mode
    uint16_t ReservedWord220 : 8;

    uint16_t ReservedWord221; // Word 221

    struct {                        // Word 222 Transport major version number
        uint16_t MajorVersion : 12; // 0000h or FFFFh = device does not report version
        uint16_t TransportType : 4;
    } TransportMajorVersion;

    uint16_t TransportMinorVersion; // Word 223

    uint16_t ReservedWord224[6]; // Word 224...229

    uint32_t ExtendedNumberOfUserAddressableSectors[2]; // Words 230...233 Extended Number of User Addressable Sectors

    uint16_t MinBlocksPerDownloadMicrocodeMode03; // Word 234 Minimum number of 512-byte data blocks per Download Microcode mode 03h operation
    uint16_t MaxBlocksPerDownloadMicrocodeMode03; // Word 235 Maximum number of 512-byte data blocks per Download Microcode mode 03h operation

    uint16_t ReservedWord236[19]; // Word 236...254

    uint16_t Signature : 8; // Word 255
    uint16_t CheckSum : 8;
} __attribute__((packed)) ATA_IdentifyData;

typedef struct {
    bool masterDriveExists;
    bool slaveDriveExists;
    ATA_IdentifyData *masterDriveData;
    ATA_IdentifyData *slaveDriveData;
} ATA_InitializeOutput;

void ATA_Initialize(ATA_InitializeOutput *output);
uint16_t ATA_ReadSectors(uint64_t lba, void *buffer, uint16_t count, DISK *disk); // returns the number of sectors read

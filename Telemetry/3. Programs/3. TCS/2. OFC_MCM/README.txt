Latest Uploaded Code in OFC_SNT Rabbit MCM Card (All 30 Antennae) :

*********************************************************************************************************************
	Date:29 Aug 2020.
	This OFC_SNT Code have REMOTE Miltec ON/OFF Facility.
 	Folder Name:Version_1.02 Miltec_ON.OFF
	Folder Path:\TELEMETRY\Telemetry\3. Programs\3. TCS\2. OFC_MCM\Version_1.02 Miltec_ON.OFF
	Global Macro Definations for Remote firmware in Dynamic_C:

            		Project optios --> Defines

				_FIRMWARE_NAME_    = "Miltec ON/OFF"
				_FIRMWARE_VERSION_ = 0x0102
				ADMIN_USER         = "telemetry"
				ADMIN_PASS         = "rcm4300"

	Changes in Previous Code:

		i) Added Set_MON() function in main.c file also modified define.c file
		ii)Added Set_ofc_atten function in main.c file from charu sir code on 29 Aug 2020.

*********************************************************************************************************************
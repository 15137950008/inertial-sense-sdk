/*
MIT LICENSE

Copyright 2014 Inertial Sense, LLC - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "cltool.h"
#include "ISDataMappings.h"

cmd_options_t g_commandLineOptions;
serial_port_t g_serialPort;
cInertialSenseDisplay g_inertialSenseDisplay;

int cltool_serialPortSendComManager(CMHANDLE cmHandle, int pHandle, buffer_t* bufferToSend)
{
	(void)cmHandle;
	(void)pHandle;
	return serialPortWrite(&g_serialPort, bufferToSend->buf, bufferToSend->size);
}

bool cltool_setupLogger(InertialSense& inertialSenseInterface)
{
	// Enable logging in continuous background mode
	return inertialSenseInterface.SetLoggerEnabled
	(
		g_commandLineOptions.enableLogging, // enable logger
		g_commandLineOptions.logPath, // path to log to, if empty defaults to DEFAULT_LOGS_DIRECTORY
		cISLogger::ParseLogType(g_commandLineOptions.logType), // log type
		g_commandLineOptions.rmcPresetPPD, // Stream rmc preset
		g_commandLineOptions.maxLogSpacePercent, // max space in percentage of free space to use, 0 for unlimited
		g_commandLineOptions.maxLogFileSize, // each log file will be no larger than this in bytes
		g_commandLineOptions.maxLogMemory, // logger will try and keep under this amount of memory
		g_commandLineOptions.logSubFolder // log sub folder name
	);

	// Call these elsewhere as needed
// 	inertialSenseInterface.EnableLogger(false);	// Enable/disable during runtime
// 	inertialSenseInterface.CloseLogger();		// Stop logging and save remaining data to file
}

static bool startsWith(const char* str, const char* pre)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

#define CL_DEFAULT_BAUD_RATE				IS_COM_BAUDRATE_DEFAULT 
#define CL_DEFAULT_COM_PORT					"*"
#define CL_DEFAULT_DISPLAY_MODE				cInertialSenseDisplay::DMODE_PRETTY 
#define CL_DEFAULT_LOG_TYPE					"dat"
#define CL_DEFAULT_LOGS_DIRECTORY			DEFAULT_LOGS_DIRECTORY
#define CL_DEFAULT_ENABLE_LOGGING			false 
#define CL_DEFAULT_MAX_LOG_FILE_SIZE		1024 * 1024 * 5
#define CL_DEFAULT_MAX_LOG_SPACE_PERCENT	0.5f 
#define CL_DEFAULT_MAX_LOG_MEMORY			131072
#define CL_DEFAULT_REPLAY_SPEED				1.0
#define CL_DEFAULT_BOOTLOAD_VERIFY			true

bool cltool_parseCommandLine(int argc, char* argv[])
{
	// set defaults
	g_commandLineOptions.baudRate = CL_DEFAULT_BAUD_RATE;
	g_commandLineOptions.comPort = CL_DEFAULT_COM_PORT;
	g_commandLineOptions.displayMode = CL_DEFAULT_DISPLAY_MODE;
	g_commandLineOptions.rmcPresetPPD = false;
	g_commandLineOptions.enableLogging = CL_DEFAULT_ENABLE_LOGGING;
	g_commandLineOptions.logType = CL_DEFAULT_LOG_TYPE;
	g_commandLineOptions.logPath = CL_DEFAULT_LOGS_DIRECTORY;
	g_commandLineOptions.maxLogFileSize = CL_DEFAULT_MAX_LOG_FILE_SIZE;
	g_commandLineOptions.maxLogSpacePercent = CL_DEFAULT_MAX_LOG_SPACE_PERCENT;
	g_commandLineOptions.maxLogMemory = CL_DEFAULT_MAX_LOG_MEMORY;
	g_commandLineOptions.replaySpeed = CL_DEFAULT_REPLAY_SPEED;
	g_commandLineOptions.bootloaderVerify = CL_DEFAULT_BOOTLOAD_VERIFY;
	g_commandLineOptions.timeoutFlushLoggerSeconds = 3;
	g_commandLineOptions.asciiMessages = "";

	cltool_outputHelp();

	if(argc <= 1)
	{	// Display usage menu if no options are provided 
		cltool_outputUsage();
		return false;
	}

	// parse command line.  Keep these options in alphabetic order!
	for (int i = 1; i < argc; i++)
	{
		const char* a = argv[i];
		if (startsWith(a, "-baud="))
		{
			g_commandLineOptions.baudRate = strtol(&a[6], NULL, 10);
		}
		else if (startsWith(a, "-b="))
		{
			g_commandLineOptions.bootloaderFileName = &a[3];
		}
		else if (startsWith(a, "-bv="))
		{
			g_commandLineOptions.bootloaderVerify = a[4] == '1';
		}
		else if (startsWith(a, "-c="))
		{
			g_commandLineOptions.comPort = &a[3];
		}
		else if (startsWith(a, "-dboc"))
		{
			g_commandLineOptions.disableBroadcastsOnClose = true;
		}
		else if (startsWith(a, "-flashConfig="))
		{
			g_commandLineOptions.flashConfig = &a[13];
		}
		else if (startsWith(a, "-flashConfig"))
		{
			g_commandLineOptions.flashConfig = ".";
		}
		else if (startsWith(a, "-host="))
		{
			g_commandLineOptions.host = &a[6];
		}
		else if (startsWith(a, "-h") || startsWith(a, "--h") || startsWith(a, "-help") || startsWith(a, "--help"))
		{
			cltool_outputUsage();
			return false;
		}
		else if (startsWith(a, "-lms="))
		{
			g_commandLineOptions.maxLogSpacePercent = (float)atof(&a[5]);
		}
		else if (startsWith(a, "-lmf="))
		{
			g_commandLineOptions.maxLogFileSize = (uint32_t)strtoul(&a[5], NULL, 10);
		}
		else if (startsWith(a, "-lmm="))
		{
			g_commandLineOptions.maxLogMemory = (uint32_t)strtoul(&a[5], NULL, 10);
		}
		else if (startsWith(a, "-lts="))
		{
			const char* subFolder = &a[5];
			if (*subFolder == '1' || startsWith(subFolder, "true"))
			{
				g_commandLineOptions.logSubFolder = cISLogger::CreateCurrentTimestamp();
			}
			else if (*subFolder == '\0' || *subFolder == '0' || startsWith(subFolder, "false"))
			{
				g_commandLineOptions.logSubFolder = cISLogger::g_emptyString;
			}
			else
			{
				g_commandLineOptions.logSubFolder = subFolder;
			}
		}
		else if (startsWith(a, "-lp="))
		{
			g_commandLineOptions.logPath = &a[4];
		}
		else if (startsWith(a, "-lt="))
		{
			g_commandLineOptions.logType = &a[4];
		}
		else if (startsWith(a, "-lon"))
		{
			g_commandLineOptions.enableLogging = true;
		}
		else if (startsWith(a, "-magRecal"))
		{
			g_commandLineOptions.rmcPresetPPD = false;
			g_commandLineOptions.magRecal = true;
			g_commandLineOptions.magRecalMode = strtol(a + 9, NULL, 10);
		}
		else if (startsWith(a, "-msgBaro"))
		{
			g_commandLineOptions.streamBaro = true;
		}
		else if (startsWith(a, "-msgPIMU"))
		{
			g_commandLineOptions.streamDThetaVel = true;
		}
		else if (startsWith(a, "-msgDualIMU"))
		{
			g_commandLineOptions.streamDualIMU = true;
		}
		else if (startsWith(a, "-msgGPS"))
		{
			g_commandLineOptions.streamGPS = true;
		}
		else if (startsWith(a, "-msgIMU1"))
		{
			g_commandLineOptions.streamIMU1 = true;
		}
		else if (startsWith(a, "-msgIMU2"))
		{
			g_commandLineOptions.streamIMU2 = true;
		}
		else if (startsWith(a, "-msgINS1"))
		{
			g_commandLineOptions.streamINS1 = true;
		}
		else if (startsWith(a, "-msgINS2"))
		{
			g_commandLineOptions.streamINS2 = true;
		}
		else if (startsWith(a, "-msgINS3"))
		{
			g_commandLineOptions.streamINS3 = true;
		}
		else if (startsWith(a, "-msgINS4"))
		{
			g_commandLineOptions.streamINS4 = true;
		}
		else if (startsWith(a, "-msgMag1"))
		{
			g_commandLineOptions.streamMag1 = true;
		}
		else if (startsWith(a, "-msgMag2"))
		{
			g_commandLineOptions.streamMag2 = true;
		}
		else if (startsWith(a, "-msgRTOS"))
		{
			g_commandLineOptions.streamRTOS = true;
		}
		else if (startsWith(a, "-msgSensors"))
		{
			g_commandLineOptions.streamSysSensors = true;
		}
		else if (startsWith(a, "-msgPPD"))
		{
			g_commandLineOptions.rmcPresetPPD = true;
		}
		else if (startsWith(a, "-q"))
		{
			g_commandLineOptions.displayMode = cInertialSenseDisplay::DMODE_QUIET;
		}
		else if (startsWith(a, "-rp="))
		{
			g_commandLineOptions.replayDataLog = true;
			g_commandLineOptions.logPath = &a[4];
		}
		else if (startsWith(a, "-rs="))
		{
			g_commandLineOptions.replayDataLog = true;
			g_commandLineOptions.replaySpeed = (float)atof(&a[4]);
		}
		else if (startsWith(a, "-r"))
		{
			g_commandLineOptions.replayDataLog = true;
		}
		else if (startsWith(a, "-stats"))
		{
			g_commandLineOptions.displayMode = cInertialSenseDisplay::DMODE_STATS;
		}
		else if (startsWith(a, "-svr="))
		{
			g_commandLineOptions.serverConnection = &a[5];
		}
		else if (startsWith(a, "-s"))
		{
			g_commandLineOptions.displayMode = cInertialSenseDisplay::DMODE_SCROLL;
		}
		else if (startsWith(a, "-log-flush-timeout="))
		{
			g_commandLineOptions.timeoutFlushLoggerSeconds = strtoul(&a[19], NULLPTR, 10);
		}
		else if (startsWith(a, "-asciiMessages="))
		{
			g_commandLineOptions.asciiMessages = &a[15];
		}
		else
		{
			cout << "Unrecognized command line option: " << a << endl;
			cltool_outputUsage();
			return false;
		}
	}

	// We are either using a serial port or replaying data
	if ((g_commandLineOptions.comPort.length() == 0) && !g_commandLineOptions.replayDataLog)
	{
		cltool_outputUsage();
		return false;
	}

	if (g_commandLineOptions.bootloaderFileName.length() && g_commandLineOptions.comPort.length() == 0)
		cout << "Use COM_PORT option \"-c=\" with bootloader" << endl;

	return true;
}

bool cltool_replayDataLog()
{
	if (g_commandLineOptions.logPath.length() == 0)
	{
		cout << "Please specify the replay log path!" << endl;
		return false;
	}

	cISLogger logger;
	if (!logger.LoadFromDirectory(g_commandLineOptions.logPath))
	{
		cout << "Failed to load log files: " << g_commandLineOptions.logPath << endl;
		return false;
	}

	cout << "Replaying log files: " << g_commandLineOptions.logPath << endl;
	p_data_t *data;
	while ((data = logger.ReadData()) != NULL)
	{
		g_inertialSenseDisplay.ProcessData(data, g_commandLineOptions.replayDataLog, g_commandLineOptions.replaySpeed);
	}

	cout << "Done replaying log files: " << g_commandLineOptions.logPath << endl;
	g_inertialSenseDisplay.Goodbye();
	return true;
}

void cltool_outputUsage()
{
	cout << boldOff;
	cout << "-----------------------------------------------------------------" << endl;
	cout << endlbOn;
	cout << "DESCRIPTION" << endlbOff;
	cout << "    Command line utility for communicating, logging, and updating" << endl;
	cout << "    firmware with Inertial Sense product line." << endl;
	cout << endlbOn;
	cout << "EXAMPLES" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c="  <<     EXAMPLE_PORT << " -msgPPD            " << EXAMPLE_SPACE_1 << boldOff << " # stream post processing data (PPD) with INS2" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c="  <<     EXAMPLE_PORT << " -msgPPD -lon       " << EXAMPLE_SPACE_1 << boldOff << " # stream PPD + INS2 data, logging" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c="  <<     EXAMPLE_PORT << " -msgPPD -lon -lts=1" << EXAMPLE_SPACE_1 << boldOff << " # stream PPD + INS2 data, logging, dir timestamp" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c="  <<     EXAMPLE_PORT << " -baud=115200 -msgINS2 -msgGPS -msgBaro" << boldOff << " # stream multiple at 115200 bps" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -rp=" <<     EXAMPLE_LOG_DIR                                           << boldOff << " # replay log files from a folder" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c="  <<     EXAMPLE_PORT << " -b= " << EXAMPLE_FIRMWARE_FILE          << boldOff << " # bootload firmware" << endlbOn;
	cout << "    " << APP_NAME << APP_EXT << " -c=* -baud=921600              "                    << EXAMPLE_SPACE_2 << boldOff << " # 921600 bps baudrate on all serial ports" << endlbOn;
	cout << endlbOn;
	cout << "OPTIONS (General)" << endl;
	cout << "    -h --help" << boldOff << "       display this help menu" << endlbOn;
	cout << "    -c=" << boldOff << "COM_PORT     select the serial port. Set COM_PORT to \"*\" for all ports and \"*4\" to use" << endlbOn;
	cout << "       " << boldOff << "             only the first four ports. Not specifying this parameter is same as \"-c=" << CL_DEFAULT_COM_PORT << "\"" <<  endlbOn;
	cout << "    -baud=" << boldOff << "BAUDRATE  set serial port baudrate.  Options: " << IS_BAUDRATE_115200 << ", " << IS_BAUDRATE_230400 << ", " << IS_BAUDRATE_460800 << ", " << IS_BAUDRATE_921600 << ", " << endlbOn;
	cout << "       " << boldOff << "             " << IS_BAUDRATE_3000000 << " (default) " << endlbOn;
	cout << "    -b=" << boldOff << "FILEPATH     bootload firmware using .hex file FILEPATH" << endlbOn;
	cout << "    -q" << boldOff << "              quite mode, no display" << endlbOn;
	cout << "    -s" << boldOff << "              scroll displayed messages to show history" << endlbOn;
	cout << "    -stats" << boldOff << "          display statistics of data received" << endlbOn;
	cout << "    -magRecal[n]" << boldOff << "    recalibrate magnetometers: 0=multi-axis, 1=single-axis" << endlbOn;
	cout << endlbOn;
	cout << "OPTIONS (Message Streaming)" << endl;
	cout << "    -msgPPD       " << boldOff << "  stream post processing data sets" << endlbOn;
	cout << "    -msgINS[n]    " << boldOff << "  stream DID_INS_[n], where [n] = 1, 2, 3 or 4 (without brackets)" << endlbOn;
	cout << "    -msgDualIMU   " << boldOff << "  stream DID_DUAL_IMU" << endlbOn;
	cout << "    -msgPIMU      " << boldOff << "  stream DID_PREINTEGRATED_IMU" << endlbOn;
	cout << "    -msgMag[n]    " << boldOff << "  stream DID_MAGNETOMETER_[n], where [n] = 1 or 2 (without brackets)" << endlbOn;
	cout << "    -msgBaro      " << boldOff << "  stream DID_BAROMETER" << endlbOn;
	cout << "    -msgGPS       " << boldOff << "  stream DID_GPS1_NAV" << endlbOn;
	cout << "    -msgSensors   " << boldOff << "  stream DID_SYS_SENSORS" << endlbOn;
	cout << endlbOn;
	cout << "OPTIONS (Logging to file, disabled by default)" << endl;
	cout << "    -lon" << boldOff << "            enable logging" << endlbOn;
	cout << "    -lt=" << boldOff << "TYPE        log type (dat, sdat, kml or csv, default is dat)" << endlbOn;
	cout << "    -lp=" << boldOff << "PATH        log data to path (default: " << CL_DEFAULT_LOGS_DIRECTORY << ")" << endlbOn;
	cout << "    -lms=" << boldOff << "PERCENT    log max space in percent of free space (default: " << CL_DEFAULT_MAX_LOG_SPACE_PERCENT << ")" << endlbOn;
	cout << "    -lmf=" << boldOff << "BYTES      log max file size in bytes (default: " << CL_DEFAULT_MAX_LOG_FILE_SIZE << ")" << endlbOn;
	cout << "    -lmm=" << boldOff << "BYTES      log max memory in bytes (default: "<< CL_DEFAULT_MAX_LOG_MEMORY << ")" << endlbOn;
	cout << "    -lts=" << boldOff << "0          log sub folder, 0 or blank for none, 1 for timestamp, else use as is" << endlbOn;
	cout << "    -r" << boldOff << "              replay data log from default path" << endlbOn;
	cout << "    -rp=" << boldOff << "PATH        replay data log from PATH" << endlbOn;
	cout << "    -rs=" << boldOff << "SPEED       replay data log at x SPEED" << endlbOn;
	cout << endlbOn;
	cout << "OPTIONS (Read or write flash configuration)" << endl;
	cout << "    -flashConfig" << boldOff << "    read and print to screen flash config" << endlbOn;
	cout << "    -flashConfig=key=value|key=value " << boldOff <<  endlbOn;
	cout << "        " << boldOff << "            set key / value pairs in flash config" << endlbOn;
	cout << endlbOn;
	cout << "OPTIONS (Client / server)" << endl;
	cout << "    -svr=" << boldOff << "INFO       used to retrieve external data and send to the uINS. Examples:" << endl;
	cout << "        - SERIAL:        -svr=RTCM3:SERIAL:COM9:57600         (port, baud rate)" << endl;
	cout << "        - RTCM3:         -svr=RTCM3:192.168.1.100:7777:URL:user:password" << endl;
	cout << "                                                              (URL, user, password optional)" << endl;
	cout << "        - UBLOX data:    -svr=UBLOX:192.168.1.100:7777        (no URL, user or password)" << endl;
	cout << "        - InertialSense: -svr=IS:192.168.1.100:7777           (no URL, user or password)" << endlbOn;
	cout << "    -host=" << boldOff << "IP:PORT   used to host a TCP/IP InertialSense server. Examples:" << endl;
	cout << "                         -host=:7777                          (IP is optional)" << endl;
	cout << "                         -host=192.168.1.43:7777" << endl;

	cout << boldOff;   // Last line.  Leave bold text off on exit.
}

void cltool_outputHelp()
{
	cout << endlbOff << "Run \"" << boldOn << "cltool -h" << boldOff << "\" to display the help menu." << endl;
}

bool cltool_updateFlashConfig(InertialSense& inertialSenseInterface, string flashConfigString)
{
	const nvm_flash_cfg_t& flashConfig = inertialSenseInterface.GetFlashConfig();
	const map_lookup_name_t& globalMap = cISDataMappings::GetMap();
	const map_name_to_info_t& flashMap = globalMap.at(DID_FLASH_CONFIG);

	if (flashConfigString.length() < 2)
	{
		// read flash config and display
		data_mapping_string_t stringBuffer;
		cout << "Current flash config" << endl;
		for (map_name_to_info_t::const_iterator i = flashMap.begin(); i != flashMap.end(); i++)
		{
			if (cISDataMappings::DataToString(i->second, NULL, (const uint8_t*)&flashConfig, stringBuffer))
			{
				cout << i->second.name << " = " << stringBuffer << endl;
			}
		}
		return false;
	}
	else
	{
		nvm_flash_cfg_t flashConfig = inertialSenseInterface.GetFlashConfig();
		vector<string> keyValues;
		splitString(flashConfigString, "|", keyValues);
		for (size_t i = 0; i < keyValues.size(); i++)
		{
			vector<string> keyAndValue;
			splitString(keyValues[i], "=", keyAndValue);
			if (keyAndValue.size() == 2)
			{
				if (flashMap.find(keyAndValue[0]) == flashMap.end())
				{
					cout << "Unrecognized flash config key '" << keyAndValue[0] << "' specified, ignoring." << endl;
				}
				else
				{
					const data_info_t& info = flashMap.at(keyAndValue[0]);
					cISDataMappings::StringToData(keyAndValue[1].c_str(), (int)(keyAndValue[1].length()), NULL, (uint8_t*)&flashConfig, info);
					cout << "Updated flash config key '" << keyAndValue[0] << "' to '" << keyAndValue[1].c_str() << "'" << endl;
				}
			}
		}
		inertialSenseInterface.SetFlashConfig(flashConfig);
		SLEEP_MS(1000);
		g_inertialSenseDisplay.Clear();
		return true;
	}
}
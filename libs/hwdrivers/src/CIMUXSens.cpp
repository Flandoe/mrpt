/* +---------------------------------------------------------------------------+
   |                 The Mobile Robot Programming Toolkit (MRPT)               |
   |                                                                           |
   |                          http://www.mrpt.org/                             |
   |                                                                           |
   | Copyright (c) 2005-2013, Individual contributors, see AUTHORS file        |
   | Copyright (c) 2005-2013, MAPIR group, University of Malaga                |
   | Copyright (c) 2012-2013, University of Almeria                                 |
   | All rights reserved.                                                      |
   |                                                                           |
   | Redistribution and use in source and binary forms, with or without        |
   | modification, are permitted provided that the following conditions are    |
   | met:                                                                      |
   |    * Redistributions of source code must retain the above copyright       |
   |      notice, this list of conditions and the following disclaimer.        |
   |    * Redistributions in binary form must reproduce the above copyright    |
   |      notice, this list of conditions and the following disclaimer in the  |
   |      documentation and/or other materials provided with the distribution. |
   |    * Neither the name of the copyright holders nor the                    |
   |      names of its contributors may be used to endorse or promote products |
   |      derived from this software without specific prior written permission.|
   |                                                                           |
   | THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       |
   | 'AS IS' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED |
   | TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR|
   | PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE |
   | FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL|
   | DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR|
   |  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)       |
   | HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,       |
   | STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN  |
   | ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           |
   | POSSIBILITY OF SUCH DAMAGE.                                               |
   +---------------------------------------------------------------------------+ */

#include <mrpt/hwdrivers.h> // Precompiled headers


#include <mrpt/system/threads.h>
#include <mrpt/hwdrivers/CIMUXSens.h>
#include <mrpt/slam/CObservationIMU.h>

IMPLEMENTS_GENERIC_SENSOR(CIMUXSens,mrpt::hwdrivers)

using namespace mrpt::utils;
using namespace mrpt::slam;
using namespace mrpt::hwdrivers;

#if MRPT_HAS_xSENS
	#include "xSens/cmt1.h"
	#include "xSens/cmt2.h"
	#include "xSens/cmt3.h"
	#include "xSens/cmtdef.h"
	#include "xSens/cmtmessage.h"
	#include "xSens/cmtpacket.h"
	#include "xSens/cmtscan.h"
	#include "xSens/xsens_fifoqueue.h"
	#include "xSens/xsens_janitors.h"
	#include "xSens/xsens_list.h"
	#include "xSens/xsens_std.h"
	#include "xSens/xsens_time.h"
#endif


// Adaptors for the "void*" memory blocks:
#define cmt3	    (*static_cast<xsens::Cmt3*>(m_cmt3_ptr))
#define deviceId    (*static_cast<CmtDeviceId*>(m_deviceId_ptr))


// Include libraries in linking:
#if MRPT_HAS_xSENS
	#ifdef MRPT_OS_WINDOWS
		// WINDOWS:
		#if defined(_MSC_VER) || defined(__BORLANDC__)
			#pragma comment (lib,"SetupAPI.lib")
		#endif
	#endif	// MRPT_OS_WINDOWS
#endif // MRPT_HAS_xSENS

/*-------------------------------------------------------------
					CIMUXSens
-------------------------------------------------------------*/
CIMUXSens::CIMUXSens( ) :
	m_COMbauds		(0),
	m_com_port		(),
	m_timeStartUI	(0),
	m_timeStartTT	(0),
	m_sensorPose    (),
	m_cmt3_ptr	(NULL),
	m_deviceId_ptr	(NULL),
	m_toutCounter	(0)
{
	m_sensorLabel = "XSensMTi";
#if MRPT_HAS_xSENS
    m_cmt3_ptr  = new xsens::Cmt3[1];
    m_deviceId_ptr = new CmtDeviceId[1];

#else
	THROW_EXCEPTION("MRPT has been compiled with 'MRPT_BUILD_xSENS'=OFF, so this class cannot be used.");
#endif

}

/*-------------------------------------------------------------
					~CIMUXSens
-------------------------------------------------------------*/
CIMUXSens::~CIMUXSens()
{
#if MRPT_HAS_xSENS
	cmt3.closePort();

    delete[] &cmt3;     m_cmt3_ptr= NULL;
    delete[] &deviceId; m_deviceId_ptr = NULL;
#endif
}

/*-------------------------------------------------------------
					doProcess
-------------------------------------------------------------*/
void CIMUXSens::doProcess()
{
#if MRPT_HAS_xSENS

	if(m_state == ssError)
	{
		mrpt::system::sleep(200);
		initialize();
	}

	if(m_state == ssError)
		return;

	XsensResultValue	res;
	unsigned int		cont = 0;

	do
	{
		CmtTimeStamp		nowUI;	// ms

		xsens::Packet packet(1/*NDevices*/,cmt3.isXm()/*Is Bus master*/);

		res = cmt3.waitForDataMessage(&packet);

		if( res == XRV_OK )
		{
			// Data properly collected
			nowUI		= packet.getRtc();
			m_state		= ssWorking;

			CObservationIMUPtr obs			= CObservationIMU::Create();

			// ANGLE MEASUREMENTS:
			if ( packet.containsOriEuler() )
			{
				CmtEuler	euler_data	= packet.getOriEuler();

				obs->rawMeasurements[IMU_YAW]	= DEG2RAD(euler_data.m_yaw);
				obs->dataIsPresent[IMU_YAW]		= true;
				obs->rawMeasurements[IMU_PITCH] = DEG2RAD(euler_data.m_pitch);
				obs->dataIsPresent[IMU_PITCH]	= true;
				obs->rawMeasurements[IMU_ROLL]	= DEG2RAD(euler_data.m_roll);
				obs->dataIsPresent[IMU_ROLL]	= true;
			}

			// ACCELEROMETERS MEASUREMENTS:
			if ( packet.containsCalAcc())
			{
				CmtVector 	acc_data = packet.getCalAcc(); // getRawAcc();

				obs->rawMeasurements[IMU_X_ACC]	= acc_data.m_data[0];
				obs->dataIsPresent[IMU_X_ACC]	= true;
				obs->rawMeasurements[IMU_Y_ACC]	= acc_data.m_data[1];
				obs->dataIsPresent[IMU_Y_ACC]	= true;
				obs->rawMeasurements[IMU_Z_ACC]	= acc_data.m_data[2];
				obs->dataIsPresent[IMU_Z_ACC]	= true;
			}

			// GYROSCOPES MEASUREMENTS:
			if ( packet.containsCalGyr())
			{
				CmtVector gir_data	= packet.getCalGyr(); // getRawGyr();

				obs->rawMeasurements[IMU_YAW_VEL]	= gir_data.m_data[2];
				obs->dataIsPresent[IMU_YAW_VEL]	= true;
				obs->rawMeasurements[IMU_PITCH_VEL]	= gir_data.m_data[1];
				obs->dataIsPresent[IMU_PITCH_VEL]	= true;
				obs->rawMeasurements[IMU_ROLL_VEL]	= gir_data.m_data[0];
				obs->dataIsPresent[IMU_ROLL_VEL]	= true;
			}

			// TimeStamp
			uint64_t AtUI = 0;
			if( m_timeStartUI == 0 )
			{
				m_timeStartUI = nowUI;
				m_timeStartTT = mrpt::system::now();
			}
			else
				AtUI	= nowUI - m_timeStartUI;

			double AtDO	= AtUI * 10000.0;								// Difference in intervals of 100 nsecs
			obs->timestamp		= m_timeStartTT	+ AtDO;
			obs->sensorPose		= m_sensorPose;
			obs->sensorLabel	= m_sensorLabel;

			appendObservation(obs);
			m_toutCounter	= 0;

		} // end if XRV_OK

		if(res == XRV_TIMEOUT)
		{
			if(++m_toutCounter>3)
			{
				m_toutCounter	= 0;
				m_state			= ssError;
				if( cmt3.isPortOpen() )
					cmt3.closePort();

				std::cerr << "[CIMUXSens::doProcess()] Error: No data available [XRV_TIMEOUT]" << std::endl;
			}
		} // end if XRV_TIMEOUT

		if(res == XRV_TIMEOUTNODATA)
		{
//			m_state			= ssError;
//			m_timeStartUI	= 0;
//			if( cmt3.isPortOpen() )
//				cmt3.closePort();
//			std::cerr << "[CIMUXSens::doProcess()] Error: No data available [XRV_TIMEOUTNODATA]" << std::endl;
		} // end if XRV_TIMEOUTNODATA
	} while( res == XRV_OK && cont++ < 30);

#else
	THROW_EXCEPTION("MRPT has been compiled with 'MRPT_BUILD_xSENS'=OFF, so this class cannot be used.");
#endif
}

/*-------------------------------------------------------------
					lookForPort
-------------------------------------------------------------*/
bool CIMUXSens::searchPortAndConnect()
{
#if MRPT_HAS_xSENS
	uint32_t baudrate;
	if(cmt3.getBaudrate(baudrate) == XRV_OK)
		return true;

	XsensResultValue res;
	xsens::List<CmtPortInfo> portInfo;
	unsigned long portCount = 0;
	unsigned short mtCount = 0;

	if( m_com_port.empty() ) {		// Scan COM ports
		std::cout << "Scanning for connected Xsens devices..." << std::endl;
		xsens::cmtScanPorts(portInfo);
		portCount = portInfo.length();
		std::cout << "Done" << std::endl;
		if (portCount == 0) {
			std::cout << "No xSens device found" << std::endl;
			m_state = ssError;
			return false;

		} // end if (error)
	} // end if
	else														// Port defined by user in .ini file
	{
		CmtPortInfo	pInfo;
		pInfo.m_baudrate	= m_COMbauds;
		strcpy( pInfo.m_portName, m_com_port.c_str());  //m_portNr		= (unsigned char)m_com_port;
		portInfo.append( pInfo );
		portCount++;
	} // end else

	ASSERT_(portCount == 1);
	std::cout << "Using COM port " << portInfo[0].m_portName /*(long)portInfo[0].m_portNr*/ << " at " << portInfo[0].m_baudrate << " baud" << std::endl;
	std::cout << "Opening port..." << std::endl;
	//open the port which the device is connected to and connect at the device's baudrate.
	res = cmt3.openPort(portInfo[0].m_portName , portInfo[0].m_baudrate);
	if (res != XRV_OK) {
		std::cerr << "COM Port could not be opened" << std::endl;
		m_state = ssError;
		return false;
	}
	std::cout << "done" << std::endl;

	//get the Mt sensor count.
	std::cout << "Retrieving MotionTracker count (excluding attached Xbus Master(s))" << std::endl;
	mtCount = cmt3.getMtCount();
	std::cout << "MotionTracker count: " << mtCount << std::endl;

	ASSERT_(mtCount == 1);

	// retrieve the device IDs
	std::cout << "Retrieving MotionTracker device ID" << std::endl;
	res = cmt3.getDeviceId(mtCount, deviceId);
	std::cout << "Device ID at busId 1: " << (long) deviceId << std::endl;	//printf("Device ID at busId 1: %08x\n",(long) deviceId);
	if (res != XRV_OK) {
		std::cerr << "Device ID could not be gathered" << std::endl;
		m_state = ssError;
		return false;
	}

	return true;
#else
	return false;
#endif
} // end lookForPort

/*-------------------------------------------------------------
					initialize
-------------------------------------------------------------*/
void CIMUXSens::initialize()
{
#if MRPT_HAS_xSENS

	XsensResultValue	res;

	if(cmt3.isPortOpen())
		return;

	m_state = ssInitializing;

	// Search for the COM PORT and connect
	if(!searchPortAndConnect())
	{
		m_state = ssError;
		std::cerr << "Error Could not initialize the device" << std::endl;
		return;
	}

	std::cout << "xSens IMU detected and connected" << std::endl;
	CmtOutputMode		mode		= CMT_OUTPUTMODE_ORIENT | CMT_OUTPUTMODE_CALIB;
	CmtOutputSettings	settings	= CMT_OUTPUTSETTINGS_ORIENTMODE_EULER | CMT_OUTPUTSETTINGS_TIMESTAMP_SAMPLECNT | CMT_OUTPUTSETTINGS_CALIBMODE_ACCGYR;

	// set the sensor to config state
	res = cmt3.gotoConfig();
	if (res != XRV_OK) {
		m_state = ssError;	//EXIT_ON_ERROR(res,"gotoConfig");
		std::cerr << "An error ocurred when setting the device to config mode" << std::endl;
		return;
	}

	unsigned short sampleFreq;
	sampleFreq = cmt3.getSampleFrequency();

	// set the device output mode for the device(s)
	std::cout << "Configuring mode selection" << std::endl;
	CmtDeviceMode deviceMode(mode, settings, sampleFreq);
	res = cmt3.setDeviceMode(deviceMode, true, deviceId);
	if (res != XRV_OK) {
		m_state = ssError;	//EXIT_ON_ERROR(res,"setDeviceMode");
		std::cerr << "An error ocurred when configuring the device" << std::endl;
		return;
	}

	// start receiving data
	res = cmt3.gotoMeasurement();
	if (res != XRV_OK) {
		m_state = ssError;	//EXIT_ON_ERROR(res,"gotoMeasurement");
		std::cerr << "An error ocurred when setting the device to measurement mode" << std::endl;
		return;
	}

	std::cout << "Getting initial TimeStamp" << std::endl;
	// Get initial TimeStamp
	xsens::Packet packet(1/*NDevices*/,cmt3.isXm()/*Is Bus master*/);
	do
	{
		res = cmt3.waitForDataMessage(&packet);
		if( res == XRV_OK )
		{
			m_timeStartUI = (uint64_t)packet.getRtc();
			m_timeStartTT = mrpt::system::now();
		} // end if
	} while( res != XRV_OK );

	std::cout << "Gathering data" << std::endl;
	m_state = ssWorking;

#else
	THROW_EXCEPTION("MRPT has been compiled with 'MRPT_BUILD_xSENS'=OFF, so this class cannot be used.");
#endif
}

/*-------------------------------------------------------------
					loadConfig_sensorSpecific
-------------------------------------------------------------*/
void  CIMUXSens::loadConfig_sensorSpecific(
	const mrpt::utils::CConfigFileBase &configSource,
	const std::string	  &iniSection )
{
	m_sensorPose.setFromValues(
        configSource.read_float( iniSection, "pose_x", 0, false ),
        configSource.read_float( iniSection, "pose_y", 0, false ),
        configSource.read_float( iniSection, "pose_z", 0, false ),
        DEG2RAD( configSource.read_float( iniSection, "pose_yaw", 0, false ) ),
        DEG2RAD( configSource.read_float( iniSection, "pose_pitch", 0, false ) ),
        DEG2RAD( configSource.read_float( iniSection, "pose_roll", 0, false ) ) );

	m_COMbauds = configSource.read_int(iniSection, "baudRate", m_COMbauds, false );

#ifdef MRPT_OS_WINDOWS
	m_com_port = configSource.read_string(iniSection, "COM_port_WIN", m_com_port, false );
#else
	m_com_port = configSource.read_string(iniSection, "COM_port_LIN", m_com_port, false );
#endif


}

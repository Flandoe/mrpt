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
#ifndef  CJOYSTICK_H
#define  CJOYSTICK_H

#include <mrpt/config.h>
#include <mrpt/utils/utils_defs.h>
#include <mrpt/hwdrivers/link_pragmas.h>

/*---------------------------------------------------------------
	Class
  ---------------------------------------------------------------*/
namespace mrpt
{
	namespace hwdrivers
	{
		/** Access to joysticks and gamepads (read buttons and position), and request number of joysticks in the system.
		  * \ingroup mrpt_hwdrivers_grp
		  */
		class HWDRIVERS_IMPEXP CJoystick
		{
		private:
			/** The axis limits:
			 */
			int 	m_x_min,m_x_max,m_y_min,m_y_max,m_z_min,m_z_max;


	#if defined(MRPT_OS_LINUX)
			int			m_joy_fd;		//!< File FD for the joystick, or -1 if not open (Linux only)
			int			m_joy_index;	//!< The index of the joystick open in m_joy_fd (Linux only)
			/** Using an event system we only have deltas, need to keep the whole joystick state (Linux only) */
			vector_bool	m_joystate_btns;
			/** Using an event system we only have deltas, need to keep the whole joystick state (Linux only) */
			vector_int	m_joystate_axes;
	#endif

		public:
			/** Constructor
			 */
			CJoystick();

			/** Destructor
			 */
			virtual ~CJoystick();

			/** Returns the number of Joysticks in the computer.
			 */
			static int  getJoysticksCount();

			/** Gets joystick information.
			 *
			 *   This method will try first to open the joystick, so you can safely call it while the joystick is plugged and removed arbitrarly.
			 *
			 * \param nJoy The index of the joystick to query: The first one is 0, the second 1, etc... See CJoystick::getJoysticksCount to discover the number of joysticks in the system.
			 * \param x The x axis position, range [-1,1]
			 * \param y The y axis position, range [-1,1]
			 * \param z The z axis position, range [-1,1]
			 * \param buttons Each element will hold true if buttons are pressed. The size of the vector will be set automatically to the number of buttons.
			 * \param raw_x_pos If it is desired the raw integer measurement from JoyStick, set this pointer to a desired placeholder.
			 * \param raw_y_pos If it is desired the raw integer measurement from JoyStick, set this pointer to a desired placeholder.
			 * \param raw_z_pos If it is desired the raw integer measurement from JoyStick, set this pointer to a desired placeholder.
			 *
			 * \return Returns true if successfull, false on error, for example, if joystick is not present.
			 *
			 * \sa setLimits
			 */
			bool  getJoystickPosition(
				int 	nJoy,
				float 	&x,
				float 	&y,
				float 	&z,
				std::vector<bool>	&buttons,
				int *raw_x_pos=NULL,
				int	*raw_y_pos=NULL,
				int *raw_z_pos=NULL );

			/** Set the axis limit values, for computing a [-1,1] position index easily (Only required to calibrate analog joystick).
			 *   It seems that these values must been calibrated for each joystick model.
			 *
			 * \sa getJoystickPosition
			 */
	#ifdef MRPT_OS_WINDOWS
			void  setLimits( int x_min = 0,int x_max = 0xFFFF, int y_min=0,int y_max = 0xFFFF,int z_min=0,int z_max = 0xFFFF );
	#else
			void  setLimits( int x_min = -32767,int x_max = 32767, int y_min=-32767,int y_max = 32767,int z_min=-32767,int z_max = 32767);
	#endif
		}; // End of class def.

	} // End of namespace
} // End of namespace

#endif

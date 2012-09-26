/*
 alpaodm.h -- Alpao deformable mirror module header
 Copyright (C) 2011-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef HAVE_ALPAODM_H
#define HAVE_ALPAODM_H

#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "wfc.h"
#include "devices.h"

using namespace std;
const string alpaodm_type = "alpaodm";

/*!
 @brief Alpao deformable mirror driver class
 
 AlpaoDM is derived from Wfc. It can control an Alpao deformable mirror.
 
 Although the underlying library supports control for multiple DMs at the same 
 time, support is currently limited for 1 device in total.
 
 \section alpaodm_mirror Alpao DM 
 
 \subsection alpaodm_actmap
 
 Actuator map for Alpao DM 97:
 
 <pre>
         66 55 44 33 22
      77 67 56 45 34 23 13
   86 78 68 57 46 35 24 14  6
93 87 79 69 58 47 36 25 15  7 1
94 88 80 70 59 48 37 26 16  8 2
95 89 81 71 60 49 38 27 17  9 3
96 90 82 72 61 50 39 28 18 10 4
97 91 83 73 62 51 40 29 19 11 5
   92 84 74 63 52 41 30 20 12
      85 75 64 53 42 31 21
         76 65 54 43 32
 </pre>
 
 Make actuator maps:
 
 <pre>
actlayout= [66, 55, 44, 33, 22,
        77, 67, 56, 45, 34, 23, 13,
    86, 78, 68, 57, 46, 35, 24, 14, 6,
93, 87, 79, 69, 58, 47, 36, 25, 15, 7,  1,
94, 88, 80, 70, 59, 48, 37, 26, 16, 8,  2,
95, 89, 81, 71, 60, 49, 38, 27, 17, 9,  3,
96, 90, 82, 72, 61, 50, 39, 28, 18, 10, 4,
97, 91, 83, 73, 62, 51, 40, 29, 19, 11, 5,
    92, 84, 74, 63, 52, 41, 30, 20, 12,
        85, 75, 64, 53, 42, 31, 21,
            76, 65, 54, 43, 32]

actmask =  [0,0,1,0,0,
          0,1,1,1,1,1,0,
        0,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,0,
      1,1,1,1,1,1,1,1,1,1,1,
      0,1,1,1,1,1,1,1,1,1,0,
      0,1,1,1,1,1,1,1,1,1,0,
        0,1,1,1,1,1,1,1,0,
          0,1,1,1,1,1,0,
            0,0,1,0,0] 
 

import numpy as np 
actmap = np.r_[actlayout][np.r_[actmask] == 1]
actmapstr = str(len(actmap))
actmaphumanstr = str(len(actmap))
for virt_act, real_act in enumerate(actmap):
  actmapstr += " %d %d" % (virt_act, real_act)
  actmaphumanstr += ", %d -> %d" % (virt_act, real_act)

print actmapstr
print actmaphumanstr
 </pre>
 
 
 \section alpaodm_cfg Configuration parameters
 
 AlpaoDM supports the following configuration parameters:
 
 - serial: serial number of the DM to drive (required)
 
 \section alpaodm_cfg Network IO
 
 - get serial: return DM serial
 - get offset: return factory defined offset
 
 */
class AlpaoDM: public Wfc {
private:
	string serial;										//!< Alpao DM serial number
	int dm_id;												//!< IDs of the DM we are driving
	
	Path conf_acfg;										//!< Alpao DM .acfg file
	Path conf_data;										//!< Alpao DM associated binary data file

	vector<double> hwoffset;					//!< DM factory offset (calibrated safe 'zero' position)
	string hwoffset_str;							//!< Space-separated representation of offset

	vector<double> act_vec;						//!< Local temporary actuate vector (copy of ctrlparms.ctrl_vec)
	
	pthread::mutex alpao_mutex;				//!< acedev5Send() can only be called sequentially! Lock mutex before actuating, or the Alpao driver double free()'s some memory.

public:
	AlpaoDM(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~AlpaoDM();
	
	int reset_zerovolt();							//!< Set DM to zero volts, which bypasses the factory offsets
	
	// From Wfc::
	virtual int dm_actuate(const bool block=false);
	virtual int calibrate();
	virtual int reset();
	
	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_ALPAODM_H

/*!
 \page dev_wfc_alpaodm Alpao deformable mirror devices
 
 The AlpaoDM class can drive an Alpao deformable mirror.
 
 */
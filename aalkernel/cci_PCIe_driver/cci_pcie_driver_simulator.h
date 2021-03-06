//******************************************************************************
// This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
//         redistributing this file, you may do so under either license.
//
//                            GPL LICENSE SUMMARY
//
//  Copyright(c) 2011-2016, Intel Corporation.
//
//  This program  is  free software;  you  can redistribute it  and/or  modify
//  it  under  the  terms of  version 2 of  the GNU General Public License  as
//  published by the Free Software Foundation.
//
//  This  program  is distributed  in the  hope that it  will  be useful,  but
//  WITHOUT   ANY   WARRANTY;   without   even  the   implied   warranty    of
//  MERCHANTABILITY  or  FITNESS  FOR  A  PARTICULAR  PURPOSE.  See  the   GNU
//  General Public License for more details.
//
//  The  full  GNU  General Public License is  included in  this  distribution
//  in the file called README.GPLV2-LICENSE.TXT.
//
//  Contact Information:
//  Henry Mitchel, henry.mitchel at intel.com
//  77 Reed Rd., Hudson, MA  01749
//
//                                BSD LICENSE
//
//  Copyright(c) 2011-2016, Intel Corporation.
//
//  Redistribution and  use  in source  and  binary  forms,  with  or  without
//  modification,  are   permitted  provided  that  the  following  conditions
//  are met:
//
//    * Redistributions  of  source  code  must  retain  the  above  copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in  binary form  must  reproduce  the  above copyright
//      notice,  this  list of  conditions  and  the  following disclaimer  in
//      the   documentation   and/or   other   materials   provided  with  the
//      distribution.
//    * Neither   the  name   of  Intel  Corporation  nor  the  names  of  its
//      contributors  may  be  used  to  endorse  or promote  products derived
//      from this software without specific prior written permission.
//
//  THIS  SOFTWARE  IS  PROVIDED  BY  THE  COPYRIGHT HOLDERS  AND CONTRIBUTORS
//  "AS IS"  AND  ANY  EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING,  BUT  NOT
//  LIMITED  TO, THE  IMPLIED WARRANTIES OF  MERCHANTABILITY  AND FITNESS  FOR
//  A  PARTICULAR  PURPOSE  ARE  DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,
//  SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL   DAMAGES  (INCLUDING,   BUT   NOT
//  LIMITED  TO,  PROCUREMENT  OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF USE,
//  DATA,  OR PROFITS;  OR BUSINESS INTERRUPTION)  HOWEVER  CAUSED  AND ON ANY
//  THEORY  OF  LIABILITY,  WHETHER  IN  CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE  OR OTHERWISE) ARISING  IN ANY WAY  OUT  OF THE USE
//  OF  THIS  SOFTWARE, EVEN IF ADVISED  OF  THE  POSSIBILITY  OF SUCH DAMAGE.
//******************************************************************************
//****************************************************************************
//        FILE: cciv4_simulator.h
//     CREATED: Jul 27, 2015
//      AUTHOR: Joseph Grecco <joe.grecco@intel.com>
//
// PURPOSE:   Definitions for CCIv3 Simulator
// HISTORY:
// COMMENTS:
// WHEN:          WHO:     WHAT:
//****************************************************************************///
#ifndef __CCI_PCIE_DRIVER_SIMULATOR_H__
#define __CCI_PCIE_DRIVER_SIMULATOR_H__

#include "aalsdk/kernel/kosal.h"

#define CCI_SIM_APERTURE_SIZE     (  0x120000  )       /// Size of device CSR region, in bytes

// Using NLB AFU ID for Mode 0 NLB
#ifndef BDX_SIM
   #define BDX_SIM 0
#endif
#ifndef SKX_SIM
   #define SKX_SIM 1
#endif

// FWIW, I think the High and Low are reversed, here.
#if BDX_SIM
   #define CCI_SIM_AFUIDH         ( 0xC000C9660D824272L )
   #define CCI_SIM_AFUIDL         ( 0x9AEFFE5F84570612L )
#elif SKX_SIM
   #define CCI_SIM_AFUIDH         ( 0xD8424DC4A4A3C413L )
   #define CCI_SIM_AFUIDL         ( 0xF89E433683F9040BL )
#endif /* BDX */

#define CCI_SIM_MAFUIDH           ( 0x6d0b2b05111c460eL )
#define CCI_SIM_MAFUIDL           ( 0xaaf163c9f423dc1dL )

#define CCI_SIM_CMAFUIDH          ( 0xf05c0786e3e64b43L )
#define CCI_SIM_CMAFUIDL          ( 0x94d78e96d28152c6L )

//
// Prototypes
int cci_sim_discover_devices(unsigned  numdevices,
                             kosal_list_head *g_device_list);



#endif /* CCI_PCIE_DRIVER_SIMULATOR_H_ */

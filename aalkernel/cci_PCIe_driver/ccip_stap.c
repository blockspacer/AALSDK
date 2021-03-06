//******************************************************************************
// This  file  is  provided  under  a  dual BSD/GPLv2  license.  When using or
//         redistributing this file, you may do so under either license.
//
//                            GPL LICENSE SUMMARY
//
//  Copyright(c) 2015-2016, Intel Corporation.
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
//  Copyright(c) 2015-2016, Intel Corporation.
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
/// @file ccip_stap.c
/// @brief  Definitions for ccip Signal Tap AAL device.
/// @ingroup aalkernel_ccip
/// @verbatim
//        FILE: ccip_port_mmio.c
//     CREATED: Nov 11, 2015
//      AUTHOR:
//
// PURPOSE:   This file contains the implementation of the CCIP Signal Tap
//             low-level function (i.e., Physical Interface Protocol driver).
// HISTORY:
// COMMENTS:
// WHEN:          WHO:     WHAT:
//****************************************************************************///
#include "aalsdk/kernel/kosal.h"
#define MODULE_FLAGS CCIPCIE_DBG_MOD

#include "aalsdk/kernel/AALTransactionID_s.h"
#include "aalsdk/kernel/aalbus-ipip.h"
#include "aalsdk/kernel/ccip_defs.h"

#include "aalsdk/kernel/ccipdriver.h"
#include "ccipdrv-events.h"

#include "ccip_port.h"
#include "cci_pcie_driver_PIPsession.h"



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////                                     //////////////////////
/////////////////              PIP INTERFACE               ////////////////////
////////////////////                                     //////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int CommandHandler( struct aaldev_ownerSession *,
                           struct aal_pipmessage*);

// TODO going to try and use a common mmapper for all objects
extern int cci_mmap( struct aaldev_ownerSession *pownerSess,
                     struct aal_wsid *wsidp,
                     btAny os_specific );

///=============================================================================
/// Name: cci_STPpip
/// @brief Physical Interface Protocol Interface for the Signal Tap AFU
///              kernel based AFU engine.
///=============================================================================
struct aal_ipip cci_STAPpip = {
   .m_messageHandler = {
      .sendMessage   = CommandHandler,       // Command Handler
      .bindSession   = BindSession,          // Session binder
      .unBindSession = UnbindSession,        // Session unbinder
   },

   .m_fops = {
     .mmap = cci_mmap,
   },

   // Methods for binding and unbinding PIP to generic aal_device
   //  Unused in this PIP
   .binddevice    = NULL,      // Binds the PIP to the device
   .unbinddevice  = NULL,      // Binds the PIP to the device
};

///============================================================================
/// Name: cci_create_AAL_SignalTap_Device
/// @brief Creates and registers Signal Tap objects (resources) we want to
///        expose through AAL.
///
/// @param[in] pportdev - Port device
/// @param[in] paalid - Base AAL ID for this device.
/// @return    AAL Device pointer
///============================================================================
struct cci_aal_device   *
               cci_create_AAL_SignalTap_Device( struct port_device  *pportdev,
                                                struct aal_device_id *paalid)
{
   struct cci_aal_device   *pcci_aaldev = NULL;
   int ret;

   PTRACEIN;

   PVERBOSE("Creating Signal Tap device\n");

   // Construct the cci_aal_device object
   pcci_aaldev = cci_create_aal_device();

   ASSERT(NULL != pcci_aaldev);
   if( NULL == pcci_aaldev){
      return NULL;
   }

   // Make it a User AFU
   cci_aaldev_type(pcci_aaldev)     = cci_dev_STAP;

   // Record parentage
   cci_aaldev_pport(pcci_aaldev)    = pportdev;       // Save its port
   cci_aaldev_pfme(pcci_aaldev)     = ccip_port_dev_fme(pportdev);

   // Save the PCI devcie handle
   cci_aaldev_pci_dev(pcci_aaldev)  = ccip_dev_to_pci_dev( ccip_port_to_ccidev(pportdev) );

   // Device Address is the same as the Port. Set the AFU ID information
   // The following attributes describe the interfaces supported by the device
   aaldevid_afuguidl(*paalid)            = CCIP_STAP_GUIDL;
   aaldevid_afuguidh(*paalid)            = CCIP_STAP_GUIDH;
   aaldevid_pipguid(*paalid)             = CCIP_STAP_PIPIID;

   // Setup the MMIO region parameters
   cci_aaldev_kvp_afu_mmio(pcci_aaldev)   = (btVirtAddr)ccip_port_stap(pportdev);
   cci_aaldev_len_afu_mmio(pcci_aaldev)   = sizeof(struct CCIP_PORT_DFL_STAP);
   cci_aaldev_phys_afu_mmio(pcci_aaldev)  = kosal_virt_to_phys((btVirtAddr)ccip_port_stap(pportdev));

   // Create the AAL device and attach it to the CCI device object
   cci_aaldev_to_aaldev(pcci_aaldev) =  aaldev_create( "CCIPSTAP",         // AAL device base name
                                                       paalid,             // AAL ID
                                                       &cci_STAPpip);

   //CCI device object create fails, delete Signal AAL device
   if(NULL == cci_aaldev_to_aaldev(pcci_aaldev) ){
      cci_destroy_aal_device(pcci_aaldev);
      return NULL;
   }

   //===========================================================
   // Set up the optional aal_device attributes
   //

   // Set how many owners are allowed access to this device simultaneously
   cci_aaldev_to_aaldev(pcci_aaldev)->m_maxowners = 1;

   // Set the config space mapping permissions
   cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI = AAL_DEV_APIMAP_NONE;

   if( cci_aaldev_allow_map_mmior_space(pcci_aaldev) ){
      cci_aaldev_to_aaldev(pcci_aaldev)->m_mappableAPI |= AAL_DEV_APIMAP_MMIOR;
   }

   // The PIP uses the PIP context to get a handle to the CCI Device from the generic device.
   aaldev_pip_context(cci_aaldev_to_aaldev(pcci_aaldev)) = (void*)pcci_aaldev;

   // Method called when the device is released (i.e., its destructor)
   //  The canonical release device calls the user's release method.
   //  If NULL is provided then only the canonical behavior is done
   dev_setrelease(cci_aaldev_to_aaldev(pcci_aaldev), cci_release_device);

      // Device is ready for use.  Publish it with the Configuration Management Subsystem
   ret = cci_publish_aaldevice(pcci_aaldev);
   ASSERT(ret == 0);
   if(0> ret){
      PERR("Failed to initialize AAL Device for CCIPSTAP[%d:%d:%d:%d]",aaldevid_devaddr_busnum(*paalid),
                                                                       aaldevid_devaddr_devnum(*paalid),
                                                                       aaldevid_devaddr_fcnnum(*paalid),
                                                                       aaldevid_devaddr_subdevnum(*paalid));
      cci_destroy_aal_device(pcci_aaldev);
      return NULL;
   }
   return pcci_aaldev;
}


//=============================================================================
// Name: CommandHandler
// Description: Implements the PIP command handler
// Interface: public
// Inputs: pownerSess - Session between App and device
//         Message - Message to process
// Outputs: none.
// Comments:
//=============================================================================
int
CommandHandler(struct aaldev_ownerSession *pownerSess,
               struct aal_pipmessage       *Message)
{
#if (1 == ENABLE_DEBUG)
#define AFU_COMMAND_CASE(x) case x : PDEBUG("%s\n", #x);
#else
#define AFU_COMMAND_CASE(x) case x :
#endif // ENABLE_DEBUG


   // Private session object set at session bind time (i.e., when object allocated)
   struct cci_PIPsession *pSess = (struct cci_PIPsession *)aalsess_pipHandle(pownerSess);
   struct cci_aal_device  *pdev  = NULL;

   // Overall return value for this function. Set before exiting if there is an error.
   //    retval = 0 means good return.
   int retval = 0;

   // UI Driver message
   struct aalui_CCIdrvMessage *pmsg = (struct aalui_CCIdrvMessage *) Message->m_message;

   // Save original response buffer size in case we return something
   btWSSize         respBufSize     = Message->m_respbufSize;

   // Assume returning nothing. By setting the response buffer size to 0
   //   we tell the upstream side that there is no payload to copy back.
   //   Setting it here means we don't have to set it (or forget to) in each
   //   command.  We've recorded the payload buffer size above if we do need
   //   intend to send a payload.
   Message->m_respbufSize          = 0;

   PTRACEIN;

   // Perform some basic checks while assigning the pdev
   ASSERT(NULL != pSess );
   if ( NULL == pSess ) {
      PDEBUG("Error: No PIP session\n");
      return -EIO;
   }

   // Get the cci device
   pdev = cci_PIPsessionp_to_ccidev(pSess);
   if ( NULL == pdev ) {
      PDEBUG("Error: No device\n");
      return -EIO;
   }

   //=====================
   // Message processor
   //=====================
   switch ( pmsg->cmd ) {

      // Returns a workspace ID for the Config Space
      AFU_COMMAND_CASE(ccipdrv_getMMIORmap) {
         struct aalui_WSMEvent WSID;
         struct ccidrvreq *preq = (struct ccidrvreq *)pmsg->payload;

         // Used to hold the workspace ID
         struct aal_wsid   *wsidp            = NULL;

         // clear WSID structure
         memset(&WSID, 0, sizeof(struct aalui_WSMEvent));

         wsidp = ccidrv_getwsid(pownerSess->m_device, preq->ahmreq.u.wksp.m_wsid);
         if ( NULL == wsidp ) {
            PERR("Could not allocate workspace\n");
            retval = -ENOMEM;
            /* generate a failure event back to the caller? */
            goto ERROR;
         }

         wsidp->m_type = WSM_TYPE_MMIO;
         PDEBUG("Getting CSR %s Aperature WSID %p using id %llx .\n",
                   ((WSID_CSRMAP_WRITEAREA == preq->ahmreq.u.wksp.m_wsid) ? "Write" : "Read"),
                   wsidp,
                   preq->ahmreq.u.wksp.m_wsid);

         PDEBUG("Apt = %" PRIxPHYS_ADDR " Len = %d.\n",cci_aaldev_phys_afu_mmio(pdev), (int)cci_aaldev_len_afu_mmio(pdev));

         WSID.evtID           = uid_wseventMMIOMap;
         WSID.wsParms.wsid    = pwsid_to_wsidHandle(wsidp);
         WSID.wsParms.physptr = cci_aaldev_phys_afu_mmio(pdev);
         WSID.wsParms.size    = cci_aaldev_len_afu_mmio(pdev);

         // Make this atomic. Check the original response buffer size for room
         if(respBufSize >= sizeof(struct aalui_WSMEvent)){
            *((struct aalui_WSMEvent*)Message->m_response) = WSID;
            Message->m_respbufSize = sizeof(struct aalui_WSMEvent);
         }

         PDEBUG("Buf size =  %u Returning WSID %llx\n",(unsigned int)Message->m_respbufSize, *((btWSID*)Message->m_response)  );
         if ( 0 != retval ) {
            goto ERROR;
         }

         Message->m_errcode = uid_errnumOK;
         retval = 0;

         // Add the new wsid onto the session
         aalsess_add_ws(pownerSess, wsidp->m_list);

      } break;

      default: {
         // No payload
         PDEBUG("Unrecognized command %" PRIu64 " or 0x%" PRIx64 " in AFUCommand\n", pmsg->cmd, pmsg->cmd);
         Message->m_respbufSize          = 0;
         Message->m_errcode = uid_errnumInvalidRequest;
         retval = -EINVAL;
      } break;
   } // switch (pmsg->cmd)

   ASSERT(0 == retval);

ERROR:
   return retval;
}


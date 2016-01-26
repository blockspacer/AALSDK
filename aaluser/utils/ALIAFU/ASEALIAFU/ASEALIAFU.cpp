// Copyright(c) 2015-2016, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//****************************************************************************
/// @file ASEALIAFU.cpp
/// @brief Implementation of ASE ALI AFU Service.
/// @ingroup ASEALIAFU
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer Sample Application
///
///    This application is for example purposes only.
///    It is not intended to represent a model for developing commercially-deployable applications.
///    It is designed to show working examples of the AAL programming model and APIs.
///
/// AUTHORS: Henry Mitchel, Intel Corporation
///          Joseph Grecco, Intel Corporation
///          
///
/// This sample demonstrates how to create an AFU Service that uses a host-based AFU engine.
///  This design also applies to AFU Services that use hardware via a
///  Physical Interface Protocol (PIP) module.
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 07/20/2015     HM       Initial version.
/// 12/16/2015     RRS      Integration with ASE App-backend.@endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include <aalsdk/kernel/aalui.h>

#include <aalsdk/AAL.h>
#include <aalsdk/AALLoggerExtern.h>
#include <aalsdk/aas/AALInProcServiceFactory.h>
#include <aalsdk/ase/ase_common.h>
#include "ASEALIAFU.h"

BEGIN_NAMESPACE(AAL)

/// @addtogroup ASEALIAFU
/// @{

CriticalSection ASEALIAFU::sm_ASEMtx;

btBool ASEALIAFU::init(IBase               *pclientBase,
                       NamedValueSet const &optArgs,
                       TransactionID const &TranID)
{
// NOTE: caller must publish IServiceClient, and depending on its needs some others,
//       but not CCIClient. This needs to check for the correct Client.
//
//   ICCIClient *pClient = dynamic_ptr<ICCIClient>(iidCCIClient, getServiceClientBase());
//   ASSERT( NULL != pClient );
//   if ( NULL == pClient ) {
//      /// ObjectCreatedExceptionEvent Constructor.
//      initFailed( new CExceptionTransactionEvent( this,
//                                                  TranID,
//                                                  errBadParameter,
//                                                  reasMissingInterface,
//                                                  "Client did not publish ICCIClient Interface") );
//      return false;
//   }

  session_init();
  initComplete(TranID);
  return true;
}



btBool ASEALIAFU::Release(TransactionID const &TranID, btTime timeout)
{
  session_deinit();
  return ServiceBase::Release(TranID, timeout);
}


// ---------------------------------------------------------
// MMIO actions
// ---------------------------------------------------------
//
// mmioGetAddress. Return address of MMIO space.
//
btVirtAddr ASEALIAFU::mmioGetAddress( void )
{
  m_MMIORmap =  (btVirtAddr)mmio_afu_vbase;
  return m_MMIORmap;
}

//
// mmioGetLength. Return length of MMIO space.
//
btCSROffset ASEALIAFU::mmioGetLength( void )
{
  m_MMIORsize = (MMIO_LENGTH - MMIO_AFU_OFFSET);
  return m_MMIORsize;		
}

//
// mmioRead32. Read 32bit CSR. Offset given in bytes.
//
btBool ASEALIAFU::mmioRead32(const btCSROffset Offset, btUnsigned32bitInt * const pValue)
{
  mmio_read32(Offset, pValue);
  return true;
}

//
// mmioWrite32. Write 32bit CSR. Offset given in bytes.
//
btBool ASEALIAFU::mmioWrite32(const btCSROffset Offset, const btUnsigned32bitInt Value)
{
  mmio_write32(Offset, Value);
  return true;
}

//
// mmioRead64. Read 64bit CSR. Offset given in bytes.
//
btBool ASEALIAFU::mmioRead64(const btCSROffset Offset, btUnsigned64bitInt * const pValue)
{
  mmio_read64(Offset, (uint64_t*)pValue);
  return true;
}

//
// mmioWrite64. Write 64bit CSR. Offset given in bytes.
//
btBool ASEALIAFU::mmioWrite64(const btCSROffset Offset, const btUnsigned64bitInt Value)
{
  mmio_write64(Offset, Value);
  return true;
}

// -----------------------------------------------------
// Buffer allocation API
// -----------------------------------------------------
AAL::ali_errnum_e ASEALIAFU::bufferAllocate( btWSSize             Length,
                                             btVirtAddr          *pBufferptr,
                                             NamedValueSet const &rInputArgs,
                                             NamedValueSet       &rOutputArgs )
{
  struct buffer_t *buf;
  int ret;

  buf = (struct buffer_t *) malloc (sizeof(struct buffer_t));
  memset(buf, 0, sizeof(buffer_t));

  buf->memsize = (uint32_t)Length;
  // ASECCIAFU::sm_ASEMtx.Lock();
  allocate_buffer(buf);
  //ASECCIAFU::sm_ASEMtx.Unlock();
  if ( ( ASE_BUFFER_VALID != buf->valid )   ||
       ( MAP_FAILED == (void *)buf->vbase ) ||
       ( 0 == buf->fake_paddr ) ) 
    {
      std::cout << "Error Allocating ASE buffer ... EXITING\n";
      return ali_errnumNoMem;
    }

  *pBufferptr = (btVirtAddr)buf->vbase;

  // Add info to Workspace map
  struct aalui_WSMParms wsParms;
  wsParms.wsid = buf->index;
  wsParms.ptr = (btVirtAddr)buf->vbase;
  wsParms.physptr = buf->fake_paddr;
  wsParms.size = buf->memsize;
  
  m_mapWkSpc[(btVirtAddr)buf->vbase] = wsParms;

  return ali_errnumOK;
}


AAL::ali_errnum_e ASEALIAFU::bufferFree( btVirtAddr Address)
{
  // Find in map and remove
   mapWkSpc_t::iterator i = m_mapWkSpc.find(Address);
   if (i == m_mapWkSpc.end()) {  // not found
      AAL_ERR(LM_All, "Tried to free non-existent Buffer");
      return ali_errnumBadParameter;
   }

  struct aalui_WSMParms wsParms;
  wsParms = i->second;

  // Call ase_common:deallocate_buffer_by_index
  deallocate_buffer_by_index((int)wsParms.wsid);
  
  return ali_errnumOK;
}


// Exactly the same as HWALIAFU::bufferGetIOVA
btPhysAddr ASEALIAFU::bufferGetIOVA( btVirtAddr Address)
{
   mapWkSpc_t::iterator i = m_mapWkSpc.find(Address);
   if (i != m_mapWkSpc.end()) {
      return i->second.physptr;
   }

   for (mapWkSpc_t::iterator i = m_mapWkSpc.begin(); i != m_mapWkSpc.end(); i++)
     {
       if (Address < i->second.ptr + i->second.size) {
         return i->second.physptr + (Address - i->second.ptr);
       }
     }

   // not found
   return 0;
}


// ---------------------------------------------------------------------------
// IALIUMsg interface implementation
// ---------------------------------------------------------------------------

//
// umsgGetNumber. Return number of UMSGs.
//
btUnsignedInt ASEALIAFU::umsgGetNumber( void )
{
  return true;
}

//
// umsgGetAddress. Get address of specific UMSG.
//
btVirtAddr ASEALIAFU::umsgGetAddress( const btUnsignedInt UMsgNumber )
{
  return 0;
}


void ASEALIAFU::umsgTrigger64( const btVirtAddr pUMsg,
			       const btUnsigned64bitInt Value )
{

}  // umsgTrigger64


//
// umsgSetAttributes. Set UMSG attributes.
//
// TODO: not implemented
//
bool ASEALIAFU::umsgSetAttributes( NamedValueSet const &nvsArgs)
{
   return true;
}


/// @}

END_NAMESPACE(AAL)


#if defined( __AAL_WINDOWS__ )

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
  switch ( ul_reason_for_call ) {
  case DLL_PROCESS_ATTACH :
    break;
  case DLL_THREAD_ATTACH  :
    break;
  case DLL_THREAD_DETACH  :
    break;
  case DLL_PROCESS_DETACH :
    break;
  }
  return TRUE;
}

#endif // __AAL_WINDOWS__


#define SERVICE_FACTORY AAL::InProcSvcsFact< AAL::ASEALIAFU >

#if defined ( __AAL_WINDOWS__ )
# pragma warning(push)
# pragma warning(disable : 4996) // destination of copy is unsafe
#endif // __AAL_WINDOWS__

ASEALIAFU_BEGIN_SVC_MOD(SERVICE_FACTORY)
/* No commands other than default, at the moment. */
ASEALIAFU_END_SVC_MOD()

#if defined ( __AAL_WINDOWS__ )
# pragma warning(pop)
#endif // __AAL_WINDOWS__


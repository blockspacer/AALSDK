// Copyright (c) 2015, Intel Corporation
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
/// @file UIDriverInterfaceAdapter.cpp
/// @brief Implements the Interface Adapter between the OS specific driver
///        interface and the portable AAL framework..
/// @ingroup AIA
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHOR: Joseph Grecco, Intel Corporation.
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 08/21/2015     JG       Initial version started
///                @endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H


#include "aalsdk/uaia/AALuAIA_UIDriverClient.h"
#include "aalsdk/AALLoggerExtern.h"

btBool UIDriverClient::MapWSID(btWSSize Size, btWSID wsid, btVirtAddr *pRet)
{
   ASSERT(NULL != pRet);

#if   defined( __AAL_WINDOWS__ )
   DWORD                 bytes = 0;
   OVERLAPPED            overlappedIO;
   struct aalui_ioctlreq resp;
   struct aalui_ioctlreq req;
   HANDLE                hEvent;

   req.context = (btObjectType)wsid;

   memset(&overlappedIO, 0, sizeof(OVERLAPPED));
   memset(&resp, 0, sizeof(struct aalui_ioctlreq));
   memset(&req, 0, sizeof(struct aalui_ioctlreq));
   req.context = reinterpret_cast<btObjectType>(wsid);

   hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   overlappedIO.hEvent = hEvent;

   if ( !DeviceIoControl(m_hClient, AALUID_IOCTL_MMAP,
                         &req, sizeof(struct aalui_ioctlreq),
                         &resp, sizeof(struct aalui_ioctlreq),
                         &bytes, &overlappedIO) ) {

      if ( ERROR_IO_PENDING == GetLastError() ) {
         AAL_VERBOSE(LM_UAIA, __AAL_FUNCSIG__ << ": About to wait (2 of 2)" << endl);
         GetOverlappedResult(m_hClient, &overlappedIO, &bytes, TRUE);
      } else { // DeviceIoControl() failed.
         CloseHandle(hEvent);
         return false;
      }
      *pRet = (btVirtAddr)resp.context;    
   }

   CloseHandle(hEvent);    
   return true;
#elif defined( __AAL_LINUX__ )
   *pRet = (btVirtAddr)mmap(NULL, Size, PROT_READ| PROT_WRITE, MAP_SHARED, m_fdClient, wsid);
   if ( (btVirtAddr)MAP_FAILED == *pRet ) {
      *pRet = NULL;
      return false;
   }
   return true;
#endif // __AAL_LINUX__

}

void UIDriverClient::UnMapWSID(btVirtAddr ptr, btWSSize Size)
{
#ifdef __AAL_LINUX__
   munmap(ptr, Size);
#endif // __AAL_LINUX__

#ifdef __AAL_WINDOWS__
// TODO UIDriverClient uses munmap - need equiv for Windows.
#endif // __AAL_WINDOWS__
}

//==========================================================================
// Name: UIDriverClient
// Description: Constructor
//==========================================================================
UIDriverClient::UIDriverClient() :
#if   defined( __AAL_WINDOWS__ )
   m_hClient(INVALID_HANDLE_VALUE),
#elif defined( __AAL_LINUX__ )
   m_fdClient(-1),
#endif // OS
   m_bIsOK(false)
{}

//==========================================================================
// Name: ~UIDriverClient
// Description: Destructor
//==========================================================================
UIDriverClient::~UIDriverClient()
{
#if   defined( __AAL_WINDOWS__ )
   if ( INVALID_HANDLE_VALUE != m_hClient ) {
      Close();
   }
#elif defined( __AAL_LINUX__ )
   if ( m_fdClient >= 0 ) {
      Close();
   }
#endif // OS

   m_bIsOK = false;
}

//==========================================================================
// Name:  operator <<
// Description: Defines a manipulator that can take arguments
//              Since an operator takes at most 1 argument the multiple
//              arguments must be passed through a function object which
//              contains the actual manipulator pointer and its args
//==========================================================================
UIDriverClient & UIDriverClient::operator << (const UIDriverClient_uidrvManip &m)
{
     //Invoke the helper that runs the manipulator
     return m.m_fop( *this,
                      m.m_cmd,
                      m.m_payload,
                      &m.m_tid,
                      m.m_mgsRoutep,
                      m.m_DevObject);
}


#if defined( __AAL_WINDOWS__ )
# include <SetupAPI.h>

// TODO Remove short-term hack to enable SPL2 on Windows.
// {3A704F1B-0DBA-408D-B4D5-9D3D7A350CF3}
//DEFINE_GUID(GUID_DEVINTERFACE_SPL2,
//0x3a704f1b, 0xdba, 0x408d, 0xb4, 0xd5, 0x9d, 0x3d, 0x7a, 0x35, 0xc, 0xf3);

EXTERN_C const GUID DECLSPEC_SELECTANY GUID_DEVINTERFACE_SPL2
                = { 0x3a704f1b, 0xdba, 0x408d, { 0xb4, 0xd5,  0x9d,  0x3d,  0x7a,  0x35,  0xc,  0xf3 } };

static HANDLE OpenFirstDevice(const GUID *pinterface_guid)
{
  
   // Get the device information set for the device interface class
   HDEVINFO hardwareDeviceInfo;
   hardwareDeviceInfo = SetupDiGetClassDevs ( (LPGUID)pinterface_guid,
                                               NULL, // Any device
                                               NULL, // Not applicable
                                               (DIGCF_PRESENT |         // Only Devices present
                                               DIGCF_DEVICEINTERFACE)); // Only for this interface class.
   if ( INVALID_HANDLE_VALUE == hardwareDeviceInfo ) {
      printf("SetupDiGetClassDevs failed: %x\n", GetLastError());
      return INVALID_HANDLE_VALUE;
   }

   // Get the Interface data for the 
   SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
   deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
   
   if ( !SetupDiEnumDeviceInterfaces(hardwareDeviceInfo,
                                     0, 
                                     (LPGUID)pinterface_guid,
                                     0, // Only look at the first device
                                     &deviceInterfaceData) ) {
      SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
      return INVALID_HANDLE_VALUE;
   }

   //
   // To get the interface details for the device is a 2 step process.
   //   First we call SetupDiGetDeviceInterfaceDetail() to determine 
   //   the size buffer we will need to get the details structure.
   //   Second allocate the structure and call the function again to 
   //   retrieve the data.

   PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
   ULONG                            structSize                = 0;

   // Get the required buffer size
   if ( !SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,   // Device Info Set
                                         &deviceInterfaceData, // Interface
                                         NULL,                 // No output
                                         0,                    // No output
                                         &structSize,
                                         NULL) ) { // Device instance not applicable
      if ( ERROR_INSUFFICIENT_BUFFER != GetLastError() ) {
         printf("SetupDiGetDeviceInterfaceDetail failed %d\n", GetLastError());
         SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
         return INVALID_HANDLE_VALUE;
      }

   }

   // Allocate the buffer
   deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) new(std::nothrow) btByte[structSize];
   if ( NULL == deviceInterfaceDetailData ) {
      printf("Couldn't allocate %d bytes for device interface details.\n", structSize);
      SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
      return INVALID_HANDLE_VALUE;
   }

   deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

   if ( !SetupDiGetDeviceInterfaceDetail(hardwareDeviceInfo,         // Device info set
                                         &deviceInterfaceData,       // Interface
                                         deviceInterfaceDetailData,  // WHere to return info
                                         structSize,                 // Struct size
                                         NULL,                       // Would == struct size 
                                         NULL) ) {                   // No used
      printf("Error in SetupDiGetDeviceInterfaceDetail\n");
      SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);
      delete[] (btByteArray)deviceInterfaceDetailData;
      return INVALID_HANDLE_VALUE;
   }

   // Done with the device info set
   SetupDiDestroyDeviceInfoList(hardwareDeviceInfo);


   // Open the SPL2 device
   printf("\nOpening the SPL2 interface @:\n %s\n", deviceInterfaceDetailData->DevicePath);

   HANDLE dev = CreateFile(deviceInterfaceDetailData->DevicePath,
                           GENERIC_READ | GENERIC_WRITE ,
                           0,
                           NULL, // no SECURITY_ATTRIBUTES structure
                           OPEN_EXISTING, // No special create flags
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // asynchronous
                           NULL);

   if ( INVALID_HANDLE_VALUE == dev ) {
      printf("Error in CreateFile: %x", GetLastError());
   }

   delete[] (btByteArray)deviceInterfaceDetailData;
   return dev;
}

#endif // __AAL_WINDOWS__

//==========================================================================
// Name: Open
// Description: Open the channel to the service
//==========================================================================
void UIDriverClient::Open(const char *devName)
{
   // Open the device
#if   defined( __AAL_WINDOWS__ )
   
   // TODO Remove short-term hack to enable SPL2 on Windows.

   if ( INVALID_HANDLE_VALUE != m_hClient ) {
      return;
   }

   m_hClient = OpenFirstDevice(&GUID_DEVINTERFACE_SPL2);

   if ( INVALID_HANDLE_VALUE == m_hClient ) {
      m_bIsOK = false;
      return;
   }

#elif defined( __AAL_LINUX__ )

   if ( m_fdClient != -1 ) {
      return;
   }

   std::string strName;
   if ( devName == NULL ) {
      strName="/dev/aalui";
   } else {
	   strName = devName;
   }

   m_fdClient = open(strName.c_str(), O_RDWR);
   if ( -1 == m_fdClient ) {
      m_bIsOK = false;
      return;
   }

#endif // OS

   m_bIsOK = true;
}  // UIDriverClient::Open

//==========================================================================
// Name: Close
// Description: lose the channel to the service
//==========================================================================
void UIDriverClient::Close() {
#if   defined( __AAL_WINDOWS__ )

   if ( INVALID_HANDLE_VALUE != m_hClient ) {
      CloseHandle(m_hClient);
      m_hClient = INVALID_HANDLE_VALUE;
      m_bIsOK   = false;
   }

#elif defined( __AAL_LINUX__ )

   if ( m_fdClient >= 0 ) {
      close(m_fdClient);
      m_fdClient = -1;
      m_bIsOK    = false;
   }

#endif // OS
}  // UIDriverClient::Close

//==========================================================================
// Name: GetMessage
// Description: Polls for messages and returns when one is available
// Comment:
//==========================================================================
btBool UIDriverClient::GetMessage(uidrvMessage *uidrvMessagep)
{
   struct aalui_ioctlreq ioctlMessage;


   if ( !IsOK() ) {
      return false;
   }

#if   defined( __AAL_WINDOWS__ )
   btHANDLE     hEvent;
   DWORD      	bytes;
   OVERLAPPED 	overlappedIO;

#elif defined( __AAL_LINUX__ )

   btInt         ret = 0;
   struct pollfd pollfds[1];

   pollfds[0].fd     = m_fdClient;
   pollfds[0].events = POLLPRI;

#endif // OS

#if   defined( __AAL_WINDOWS__ )
   // TODO THIS CAN BE CHANGED TO ALLOW POLL TO RETURN GET MSG DESC AND SAVE A CALL
   
   
   hEvent = CreateEvent(NULL, TRUE, FALSE,NULL);
   bytes = 0;
   memset(&ioctlMessage, 0, sizeof(struct aalui_ioctlreq));
   memset(&overlappedIO, 0, sizeof(OVERLAPPED));

   Lock();
   overlappedIO.hEvent = hEvent;
   if ( !DeviceIoControl(m_hClient, AALUID_IOCTL_POLL,
                           &ioctlMessage, sizeof(struct aalui_ioctlreq),
                           &ioctlMessage, sizeof(struct aalui_ioctlreq),
                           &bytes, &overlappedIO) ) {

      if ( ERROR_IO_PENDING == GetLastError() ) {

         Unlock();

         AAL_VERBOSE(LM_UAIA, __AAL_FUNCSIG__ << ": About to wait (2 of 2)" << endl);
         GetOverlappedResult(m_hClient, &overlappedIO, &bytes, TRUE);

         Lock();

      } else { // DeviceIoControl() failed.
         CloseHandle(hEvent);
         goto FAILED;
      }
   }

   memset(&ioctlMessage, 0, sizeof(struct aalui_ioctlreq));
   memset(&overlappedIO, 0, sizeof(OVERLAPPED));
   overlappedIO.hEvent = hEvent;

   // retrieve the size of the payload buffer in ioctlMessage.size
   if ( !DeviceIoControl(m_hClient, AALUID_IOCTL_GETMSG_DESC,
                           &ioctlMessage, sizeof(struct aalui_ioctlreq),
                           &ioctlMessage, sizeof(struct aalui_ioctlreq),
                           &bytes, &overlappedIO) ) {

      if ( ERROR_IO_PENDING == GetLastError() ) {

         Unlock();

         AAL_VERBOSE(LM_UAIA, __AAL_FUNCSIG__ << ": About to wait (1 of 2)" << endl);
         GetOverlappedResult(m_hClient, &overlappedIO, &bytes, TRUE);

         Lock();

      } else { // DeviceIoControl() failed.
         CloseHandle(hEvent);
         goto FAILED;
      }

   }
   ASSERT(sizeof(struct aalui_ioctlreq) == bytes);

   uidrvMessagep->size(ioctlMessage.size);

   bytes = 0;
   memset(&overlappedIO, 0, sizeof(OVERLAPPED));
   overlappedIO.hEvent = hEvent;
   if ( !DeviceIoControl(m_hClient, AALUID_IOCTL_GETMSG,
                           uidrvMessagep->GetReqp(), (DWORD)uidrvMessagep->msgsize(),
                           uidrvMessagep->GetReqp(), (DWORD)uidrvMessagep->msgsize(),
                           &bytes, &overlappedIO) ) {

      if ( ERROR_IO_PENDING == GetLastError() ) {

         Unlock();

         AAL_VERBOSE(LM_UAIA, __AAL_FUNCSIG__ << ": About to wait (2 of 2)" << endl);
         GetOverlappedResult(m_hClient, &overlappedIO, &bytes, TRUE);

         Lock();

      } else { // DeviceIoControl() failed.
         CloseHandle(hEvent);
         goto FAILED;
      }

   }

      // success
      Unlock();
      return true;
FAILED: // If got here then DeviceIoControl failed.
   Unlock();
   AAL_ERR(LM_UAIA, __AAL_FUNCSIG__ << ": DeviceIoControl() failed." << endl);
   return false;

#elif defined( __AAL_LINUX__ )

   // Break out of the while with a return
   do
   {
      {
         AutoLock(this);

         // Check for messages first
         memset(&ioctlMessage,0, sizeof(struct aalui_ioctlreq));
         if( ( ret = ioctl(m_fdClient, AALUID_IOCTL_GETMSG_DESC, &ioctlMessage) ) == 0 ) {

            uidrvMessagep->size(ioctlMessage.size);

            // Get the message -
            if( ioctl(m_fdClient, AALUID_IOCTL_GETMSG, uidrvMessagep->GetReqp()) == -1 ) {
               goto FAILED;
            }

            return true;
         }

      }

      AAL_VERBOSE(LM_UAIA, "UIDriverClient::GetMessage: About to wait" << std::endl);

      ret = poll(pollfds, 1, -1);
      if ( ret != 1 ) {   // expect a 1 here, generally
         AAL_DEBUG(LM_UAIA, "UIDriverClient::GetMessage: Returned value from poll() is not 1 as expected, but " << ret << std::endl);
      }

   // TODO: HM 20090605 Joe and I both think this logic is buggy. Revisit.
   } while( ( 0 == ret ) || ( -EAGAIN != ret ) );

FAILED: // If got here then the poll failed
   perror("UIDriverClient::GetMessage:poll");
   return false;

#endif // OS
}  // UIDriverClient::GetMessage


//==========================================================================
// Name: SendMessage
// Description: Sends a message down RMC connection
//==========================================================================
btBool UIDriverClient::SendMessage(btUnsigned64bitInt cmd, struct aalui_ioctlreq *reqp)
{

   AutoLock(this);

   if ( !IsOK() ) {
      return true;
   }

#if   defined( __AAL_WINDOWS__ )

   DWORD      bytes,bytes_to_send;
   btHANDLE  hEvent;
   OVERLAPPED overlappedIO;
   bytes_to_send = sizeof(struct aalui_ioctlreq) + reqp->size;
   bytes = 0;
   memset(&overlappedIO, 0, sizeof(OVERLAPPED));
   hEvent = CreateEvent(NULL, TRUE, FALSE,NULL);
   overlappedIO.hEvent = hEvent;
   if ( !DeviceIoControl(m_hClient, (DWORD)cmd,
                         reqp, bytes_to_send,
                         reqp, bytes_to_send,
                         &bytes, &overlappedIO) ) {

      if ( ERROR_IO_PENDING != GetLastError() ) {
         AAL_ERR(LM_UAIA, __AAL_FUNCSIG__ << "failed." << endl);
         m_bIsOK = false;
      }

   }
   CloseHandle(hEvent);

#elif defined( __AAL_LINUX__ )

   if ( -1 == ioctl(m_fdClient, cmd, reqp) ) {
      perror("UIDriverClient::SendMessage");
      m_bIsOK = false;
   }

#endif // OS

   return true;
}  // UIDriverClient::SendMessage


BEGIN_C_DECLS

//==========================================================================
// Name: msgMarshaller
// Description: This function is a helper method that is responsible for
//              building the appropriate method to send to the RMC. This
//              is called by the << operator
// Inputs: rThis  - reference to the instance of this UIDriverClient
//         cmd    - Commmand ID
//         stTidp - Pointer to transactionID structure
//         payload - Wrapper for a payload.  Passed by value so that
//                   it can garbage collect itself. I.e., if the buffer
//                   was malloced (newed) it will delete.  The buffer owner
//                   is responsible for wrapping in a Payload that does the
//                   correct thing.
//         mgsRoutep - Routing object used to route the return message
//                     (NOTE: Object must remain in scope until message delivered)
//         Handle - Device Handle
// Comment: This version requires that the payload and all of its subpayloads
//            be contiguous. Function creates a single contiguous message
//            to be sent downsteam.
//==========================================================================
UIDriverClient & msgMarshaller(UIDriverClient           &rThis,
                               uid_msgIDs_e              cmd,
                               UIDriverClient_msgPayload payload,
                               stTransactionID_t const  *stTidp,
                               uidrvMessageRoute        *mgsRoutep,
                               btObjectType              Handle)
{
   struct aalui_ioctlreq *preq = reinterpret_cast<struct aalui_ioctlreq*>(new char[sizeof(struct aalui_ioctlreq) + (size_t)payload.size()]);
   unsigned long ioctlCMD;
   char * ppayload = reinterpret_cast<char*>((preq))+sizeof(struct aalui_ioctlreq);

   preq->id      = cmd;
   preq->size    = payload.size();
   preq->tranID  = *stTidp;
   preq->context = mgsRoutep;
   preq->handle  = Handle;

   memcpy(ppayload, payload.ptr(), (size_t)payload.size());

#if 0
   cout <<"########CHECK########\n"
   		<<"\n\treq="
		<<std::hex<<&req
   		<<"\n\treq.payload="
   		<< std::hex<< req.payload << endl;    // payloads are often binary and of unknown length
#endif

   switch ( cmd ) {

   case reqid_UID_Bind:
   case reqid_UID_ExtendedBindInfo:
   case reqid_UID_UnBind:
      ioctlCMD = AALUID_IOCTL_BINDDEV;
      break;

   case reqid_UID_Activate:
      ioctlCMD = AALUID_IOCTL_ACTIVATEDEV;
      break;

   case reqid_UID_Deactivate:
      ioctlCMD = AALUID_IOCTL_DEACTIVATEDEV;
      break;

   case reqid_UID_SendAFU:
   case reqid_UID_SendPIP:
   case reqid_UID_SendWSM:
      ioctlCMD = AALUID_IOCTL_SENDMSG;
      break;
    default:
      std::cerr << "UIDRV: Unknown command class" << std::endl;
      rThis.IsOK(false);    //ERROR
      break;
   }

   UIDriverClient &ret = rThis.Send(ioctlCMD, preq);
   return ret;
}  // UIDriverClient::msgMarshaller

END_C_DECLS


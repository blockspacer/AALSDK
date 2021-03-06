// Copyright(c) 2006-2016, Intel Corporation
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
/// @file AALTransactionID.cpp
/// @brief AAL TransactionID
/// @ingroup Events
/// @verbatim
/// Accelerator Abstraction Layer
///
/// AUTHORS: Joseph Grecco, Intel Corporation
///          Henry Mitchel, Intel Corporation
///
/// WHEN:          WHO:     WHAT:
/// 01/04/2006     JG       Initial version
/// 04/11/2006     JG       Made transaction ID an [in|out] object that carries an
///                         application specified context. In addition the Tid has
///                         been extended to provide a listener used to scope per
///                         transaction event handling.
/// 01/04/2006     JG       Initial version started
/// 03/13/2007     JG       Added a constructor for AppContext
///                         only and removed default eventhandler
///                         argument which caused a potential
///                         unwanted type coversion of type void*
///                         to transaction ID when passing to an
///                         argument expecting a tranID &
/// 03/21/2007     JG       Ported to Linux
/// 10/04/2007     JG       Renamed to AALTransactionID
/// 05/08/2008     HM       Comments & License
/// 06/27/2008     HM       Added ostream << for TransactionID
/// 11/10/2008     JG       Replaced discrete member variables with typedef
/// 12/08/2008     HM/JG    Added new TransactionID ctor and fixed random intID
/// 01/04/2009     HM       Updated Copyright
/// 08/12/2010     HM       Added new CTOR: Application specified ID and Handler
/// 04/25/2014     JG       Added Support for IBase@endverbatim
//****************************************************************************
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // HAVE_CONFIG_H

#include "aalsdk/AALBase.h"
#include "aalsdk/AALTransactionID.h"

BEGIN_NAMESPACE(AAL)

//=============================================================================
// Name: TransactionID
// Description: Default constructor  - System assigned ID. Default Listener
//=============================================================================
TransactionID::TransactionID()
{
   m_tid.m_Context = NULL;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = true;

   //Assign an internal value
   m_tid.m_intID = TransactionID::NextUniqueID();
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for a stTransactionID_ structure
//=============================================================================
TransactionID::TransactionID(const stTransactionID_t &stTid)
{
   m_tid = stTid;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for a Specified Integer ID
//=============================================================================
TransactionID::TransactionID(btID intID)
{
   m_tid.m_Context = NULL;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = true;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for a Specified Integer ID
//=============================================================================
TransactionID::TransactionID(btID                 intID,
                             btApplicationContext Context)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = true;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Copy constructor
//=============================================================================
TransactionID::TransactionID(const TransactionID &rOther) :
   m_tid(rOther.m_tid)
{}

//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID
//              and Handler (if provided)
//=============================================================================
TransactionID::TransactionID(btApplicationContext Context)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = true;
   m_tid.m_intID   = TransactionID::NextUniqueID();
}


//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID, Context, Handler,
//              and filter flag
//=============================================================================
TransactionID::TransactionID(btID                 intID,
                             btApplicationContext Context,
                             btEventHandler       evtHandler,
                             btBool               Filter)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = evtHandler;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID
//              and Handler (if provided)
//=============================================================================
TransactionID::TransactionID(btApplicationContext Context,
                             btEventHandler       evtHandler,
                             btBool               Filter)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = evtHandler;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = TransactionID::NextUniqueID();
}


//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID and Handler
//=============================================================================
TransactionID::TransactionID(btID           intID,
                             btEventHandler evtHandler,
                             btBool         Filter)
{
   m_tid.m_Context = NULL;
   m_tid.m_Handler = evtHandler;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for system assigned ID application provided
//              Handler.
//=============================================================================
TransactionID::TransactionID(btEventHandler evtHandler,
                             btBool         Filter)
{
   //Assign an internal value
   m_tid.m_Context = NULL;
   m_tid.m_Handler = evtHandler;
   m_tid.m_IBase   = NULL;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = TransactionID::NextUniqueID();
}


//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID, Context, Handler,
//              and filter flag
//=============================================================================
TransactionID::TransactionID(btID                 intID,
                             btApplicationContext Context,
                             IBase               *pIBase,
                             btBool               Filter)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = pIBase;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID
//              and Handler (if provided)
//=============================================================================
TransactionID::TransactionID(btApplicationContext Context,
                             IBase               *pIBase,
                             btBool               Filter)
{
   m_tid.m_Context = Context;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = pIBase;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = TransactionID::NextUniqueID();
}


//=============================================================================
// Name: TransactionID
// Description: Constructor for Application specified ID and Handler
//=============================================================================
TransactionID::TransactionID(btID       intID,
                             IBase     *pIBase,
                             btBool     Filter)
{
   m_tid.m_Context = NULL;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = pIBase;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = intID;
}

//=============================================================================
// Name: TransactionID
// Description: Constructor for system assigned ID application provided
//              Handler.
//=============================================================================
TransactionID::TransactionID(IBase *pIBase,
                             btBool Filter)
{
   //Assign an internal value
   m_tid.m_Context = NULL;
   m_tid.m_Handler = NULL;
   m_tid.m_IBase   = pIBase;
   m_tid.m_Filter  = Filter;
   m_tid.m_intID   = TransactionID::NextUniqueID();
}

//=============================================================================
// Name: Context
// Description: Returns Context
//=============================================================================
btApplicationContext TransactionID::Context() const
{
   // return value
   return m_tid.m_Context;
}

//=============================================================================
// Name: Filter
// Description: Returns Filter flag
//=============================================================================
btBool TransactionID::Filter() const
{
   //Return filter flag
   return m_tid.m_Filter;
}

//=============================================================================
// Name: Handler
// Description: Returns Event Handler
//=============================================================================
btEventHandler TransactionID::Handler() const
{
   //Return handler
   return m_tid.m_Handler;
}

//=============================================================================
// Name: MsgHandler
// Description: Returns Message Handler
//=============================================================================
IBase * TransactionID::Ibase() const
{
   //Return handler
   return m_tid.m_IBase;
}

//=============================================================================
// Name: ID
// Description: Returns ID
//=============================================================================
btID TransactionID::ID() const
{
   //Return handler
   return m_tid.m_intID;
}

void TransactionID::Context(btApplicationContext Context) { m_tid.m_Context = Context; }
void TransactionID::Handler(btEventHandler       Handler) { m_tid.m_Handler = Handler; }
void   TransactionID::Ibase(IBase               *pIBase)  { m_tid.m_IBase   = pIBase;  }
void  TransactionID::Filter(btBool               Filter)  { m_tid.m_Filter  = Filter;  }
void      TransactionID::ID(btID                 intID)   { m_tid.m_intID   = intID;   }

TransactionID & TransactionID::operator = (const TransactionID &rOther)
{
   if ( &rOther != this ) {
      m_tid = rOther.m_tid;
   }
   return *this;
}

TransactionID & TransactionID::operator = (const stTransactionID_t &tid)
{
   if ( &tid != &m_tid ) {
      m_tid = tid;
   }
   return *this;
}

TransactionID::operator const stTransactionID_t & () const { return m_tid; }
TransactionID::operator stTransactionID_t &       ()       { return m_tid; }

btBool TransactionID::operator == (const TransactionID &rhs) const
{
   const IBase *pMyIBase  = m_tid.m_IBase;
   const IBase *pRHSIBase = rhs.m_tid.m_IBase;

   if ( ( NULL == pMyIBase ) && ( NULL != pRHSIBase ) ) {
      return false;
   }
   if ( ( NULL != pMyIBase ) && ( NULL == pRHSIBase ) ) {
      return false;
   }

   if ( NULL != pMyIBase ) {
      // We have an IBase * for both lhs and rhs.
      ASSERT(NULL != pRHSIBase);

      if ( pMyIBase->operator != (*pRHSIBase) ) {
         return false;
      }
   }

   // compare the other data members.
   return m_tid.m_Context == rhs.m_tid.m_Context &&
          m_tid.m_Handler == rhs.m_tid.m_Handler &&
          m_tid.m_Filter  == rhs.m_tid.m_Filter  &&
          m_tid.m_intID   == rhs.m_tid.m_intID;
}

//=============================================================================
// Name: ~TransactionID
// Description: Destructor
//=============================================================================
TransactionID::~TransactionID() {

   int x=1;
}

//=============================================================================
// Name: operator <<
// Description: print out a TransactionID
//=============================================================================
std::ostream & operator << (std::ostream &s, const TransactionID &TranID)
{
   s << "TransactionID: ID=" << TranID.ID() << " Context=" << TranID.Context()
     << " Handler=" << TranID.Handler() << " Filter=" << TranID.Filter() << std::endl;
   return s;
}

btID            TransactionID::sm_NextUniqueID = 0;
CriticalSection TransactionID::sm_CS;

btID TransactionID::NextUniqueID()
{
   AutoLock(&TransactionID::sm_CS);
   return TransactionID::sm_NextUniqueID++;
}

END_NAMESPACE(AAL)


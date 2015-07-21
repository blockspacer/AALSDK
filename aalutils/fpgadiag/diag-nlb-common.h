// Copyright (c) 2013-2015, Intel Corporation
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
// @file nlb-common.h
// @brief Functionality common to all NLB utils.
// @ingroup
// @verbatim
// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
//
// AUTHORS: Tim Whisonant, Intel Corporation
//
// HISTORY:
// WHEN:          WHO:     WHAT:
// 06/09/2013     TSW      Initial version.@endverbatim
//****************************************************************************
#ifndef __NLB_COMMON_H__
#define __NLB_COMMON_H__
#include <aalsdk/aalclp/aalclp.h>

#include "utils.h"
//#include "nlb-inner.h"
BEGIN_C_DECLS

struct NLBDefaults
{
   wkspc_size_type mincls;
   wkspc_size_type maxcls;
   wkspc_size_type begincls;
   wkspc_size_type endcls;
   phys_type       dsmphys;
   phys_type       srcphys;
   phys_type       dstphys;
   freq_type       clkfreq;
   const char     *prefillhits;
   const char     *prefillmiss;
   const char     *nobw;
   const char     *tabular;
   const char     *suppresshdr;
   const char     *wt;
   const char     *wb;
   const char     *pwr;
   const char     *cont;
#if   defined( __AAL_WINDOWS__ )
# error TODO
#elif defined( __AAL_LINUX__ )
   timespec_type   to_nsec;
   timespec_type   to_usec;
   timespec_type   to_msec;
   timespec_type   to_sec;
   timespec_type   to_min;
   timespec_type   to_hour;
#endif // OS
   const char     *nogui;
   const char     *demo;
   const char     *nohist;
   const char     *histdata;
};

struct NLBBandwidth
{
   int             dirty;
   uint_type       iter;
   uint_type       sample;
   wkspc_size_type bytes;
   double          bw;
#define NLB_MAX_BW_STR 20
   char            bwstr[NLB_MAX_BW_STR];
};
#define NLB_BW_CUR_RD 0
#define NLB_BW_CUR_WR 1
#define NLB_BW_MIN_RD 2
#define NLB_BW_MIN_WR 3
#define NLB_BW_MAX_RD 4
#define NLB_BW_MAX_WR 5
#define NLB_BW_AVG_RD 6
#define NLB_BW_AVG_WR 7

struct NLBCmdLine
{
   const char              *copyright;
   const char              *mode;    // "LPBK1", "READ", "WRITE", "TRPUT", "LPBK2", "LPBK3", "SW1"
   const char              *title;
   u64_type                 cmdflags;
#define NLB_CMD_PARSE_ERROR       (u64_type)0x00000001  /* Command parser error, invalid option, etc.                       */
#define NLB_CMD_FLAG_HELP         (u64_type)0x00000002  /* --help                                                           */
#define NLB_CMD_FLAG_VERSION      (u64_type)0x00000004  /* --version                                                        */

#define NLB_CMD_FLAG_TABULAR      (u64_type)0x00000008  /* --tabular         (show tabular output, like legacy NLBTest)     */
#define NLB_CMD_FLAG_SUPPRESSHDR  (u64_type)0x00000010  /* --suppress-hdr    (don't show column headers in text mode)       */
#define NLB_CMD_FLAG_BANDWIDTH    (u64_type)0x00000020  /* --no-bw           (don't calculate bandwidth numbers when clear) */

#define NLB_CMD_FLAG_DSM_PHYS     (u64_type)0x00000040  /* --dsm-phys  X     (physical address of device status workspace)  */
#define NLB_CMD_FLAG_SRC_PHYS     (u64_type)0x00000080  /* --src-phys  X     (physical address of source workspace)         */
#define NLB_CMD_FLAG_DST_PHYS     (u64_type)0x00000100  /* --dest-phys X     (physical address of destination workspace)    */

#define NLB_CMD_FLAG_SRC_CAPCM    (u64_type)0x00000200  /* --src-capcm  X    (CA PCM offset of source workspace)            */
#define NLB_CMD_FLAG_DST_CAPCM    (u64_type)0x00000400  /* --dest-capcm X    (CA PCM offset of destination workspace)       */

#define NLB_CMD_FLAG_BEGINCL      (u64_type)0x00000800  /* --begin X         (number of cache lines)                        */
#define NLB_CMD_FLAG_ENDCL        (u64_type)0x00001000  /* --end X           (number of cache lines)                        */
#define NLB_CMD_FLAG_WT           (u64_type)0x00002000  /* --wt              (write-through caching)                        */
#define NLB_CMD_FLAG_WB           (u64_type)0x00004000  /* --wb              (write-back caching)                           */
#define NLB_CMD_FLAG_PWR          (u64_type)0x00008000  /* --pwr             (posted writes)                                */
#define NLB_CMD_FLAG_CONT         (u64_type)0x00010000  /* --cont            (continuous mode)                              */
#define NLB_CMD_FLAG_TONSEC       (u64_type)0x00020000  /* --timeout-nsec X  (nanosecond timeout for continuous mode)       */
#define NLB_CMD_FLAG_TOUSEC       (u64_type)0x00040000  /* --timeout-usec X  (microsecond timeout for continuous mode)      */
#define NLB_CMD_FLAG_TOMSEC       (u64_type)0x00080000  /* --timeout-msec X  (millisecond timeout for continuous mode)      */
#define NLB_CMD_FLAG_TOSEC        (u64_type)0x00100000  /* --timeout-sec  X  (second timeout for continuous mode)           */
#define NLB_CMD_FLAG_TOMIN        (u64_type)0x00200000  /* --timeout-min  X  (minute timeout for continuous mode)           */
#define NLB_CMD_FLAG_TOHOUR       (u64_type)0x00400000  /* --timeout-hour X  (hour timeout for continuous mode)             */
#define NLB_CMD_FLAG_CLKFREQ      (u64_type)0x00800000  /* --clock-freq X    (fpga clock frequency, default=200 MHz)        */

#define NLB_CMD_FLAGS_TO          (NLB_CMD_FLAG_TONSEC | \
                                   NLB_CMD_FLAG_TOUSEC | \
                                   NLB_CMD_FLAG_TOMSEC | \
                                   NLB_CMD_FLAG_TOSEC  | \
                                   NLB_CMD_FLAG_TOMIN  | \
                                   NLB_CMD_FLAG_TOHOUR)

#define NLB_CMD_FLAG_CSRS         (u64_type)0x01000000  /* --csrs            (show CSR read/write)                          */

#define NLB_CMD_FLAG_PREFILL_HITS (u64_type)0x02000000  /* --prefill-hits    (attempt to load the cache with hits at init)  */
#define NLB_CMD_FLAG_PREFILL_MISS (u64_type)0x04000000  /* --prefill-misses  (" misses)                                     */

#define NLB_CMD_FLAG_NOGUI        (u64_type)0x08000000  /* --no-gui          (force text mode)                              */
#define NLB_CMD_FLAG_DEMO         (u64_type)0x10000000  /* --demo            (use settings desirable for GUI demos)         */
#define NLB_CMD_FLAG_NOHIST       (u64_type)0x20000000  /* --no-hist         (don't display the histogram)                  */
#define NLB_CMD_FLAG_HISTDATA     (u64_type)0x40000000  /* --hist-data       (dump the histogram data points, post test)    */

#define NLB_CMD_FLAG_FEATURE0     (u64_type)0x80000000  /* --0 */
#define NLB_CMD_FLAG_FEATURE1     (u64_type)0x100000000 /* --1 */

   uint_type                dispflags;
   uint_type                iter;
   wkspc_size_type          begincls;
   wkspc_size_type          endcls;
   phys_type                dsmphys;
   phys_type                srcphys;
   phys_type                dstphys;
   freq_type                clkfreq;
#if   defined( __AAL_WINDOWS__ )
# error TODO
#elif defined( __AAL_LINUX__ )
   struct timespec          timeout;
   timespec_type            to_nsec;
   timespec_type            to_usec;
   timespec_type            to_msec;
   timespec_type            to_sec;
   timespec_type            to_min;
   timespec_type            to_hour;
#endif // OS
   const struct NLBDefaults defaults;
   int                      bwdirty;
   struct NLBBandwidth      bw[8];

   //Added from fpgasane - CMyCmdLine
   AAL::btUIntPtr   flags;
   #define MY_CMD_FLAG_HELP    0x00000001
   #define MY_CMD_FLAG_VERSION 0x00000002
   std::string      AFUTarget;
   AAL::btInt       LogLevel;
};

#define NLB_DISP_FLAG_TITLE         (uint_type)0x00000001  /* NLB test title / description                 */
#define NLB_DISP_FLAG_HEADERS       (uint_type)0x00000002  /* (text mode) column headers                   */
#define NLB_DISP_FLAG_ITERS         (uint_type)0x00000004  /* current test iteration                       */
#define NLB_DISP_FLAG_CLS           (uint_type)0x00000008  /* current number of cache lines                */
#define NLB_DISP_FLAG_FREQ          (uint_type)0x00000010  /* FPGA clock frequency                         */
#define NLB_DISP_FLAG_EVICT         (uint_type)0x00000020  /* cache evictions perf counter                 */
#define NLB_DISP_FLAG_CACHE_INIT    (uint_type)0x00000040  /* cache initialization (prefill hits / misses) */
#define NLB_DISP_FLAG_WR_TYPE       (uint_type)0x00000080  /* write type (write-back or write-through)     */
#define NLB_DISP_FLAG_POSTED_WR     (uint_type)0x00000100  /* whether using posted writes                  */
#define NLB_DISP_FLAG_READS         (uint_type)0x00000200  /* number of reads                              */
#define NLB_DISP_FLAG_CACHE_RD_HITS (uint_type)0x00000400  /* cache read hits                              */
#define NLB_DISP_FLAG_CACHE_RD_MISS (uint_type)0x00000800  /* cache read misses                            */
#define NLB_DISP_FLAG_RD_BW         (uint_type)0x00001000  /* read bandwidth                               */
#define NLB_DISP_FLAG_PEAK_RD_BW    (uint_type)0x00002000  /* peak read bandwidth                          */
#define NLB_DISP_FLAG_AVG_RD_BW     (uint_type)0x00004000  /* average read bandwidth                       */
#define NLB_DISP_FLAG_WRITES        (uint_type)0x00008000  /* number of writes                             */
#define NLB_DISP_FLAG_CACHE_WR_HITS (uint_type)0x00010000  /* cache write hits                             */
#define NLB_DISP_FLAG_CACHE_WR_MISS (uint_type)0x00020000  /* cache write misses                           */
#define NLB_DISP_FLAG_WR_BW         (uint_type)0x00040000  /* write bandwidth                              */
#define NLB_DISP_FLAG_PEAK_WR_BW    (uint_type)0x00080000  /* peak write bandwidth                         */
#define NLB_DISP_FLAG_AVG_WR_BW     (uint_type)0x00100000  /* average write bandwidth                      */
#define NLB_DISP_FLAG_CONT          (uint_type)0x00200000  /* whether continuous mode is enabled           */
#define NLB_DISP_FLAG_CSRS          (uint_type)0x00400000  /* CSRs and read/write values                   */
#define NLB_DISP_FLAG_QLP_VERSION   (uint_type)0x00800000  /* QPI Link Protocol version                    */
#define NLB_DISP_FLAG_NLB_VERSION   (uint_type)0x01000000  /* NLB version                                  */
#define NLB_DISP_FLAG_VERSION       (uint_type)0x02000000  /* program version                              */
#define NLB_DISP_FLAG_COPYRIGHT     (uint_type)0x04000000  /* copyright notice                             */
#define NLB_DISP_FLAG_SRC_WKSPC     (uint_type)0x08000000  /* source workspace                             */
#define NLB_DISP_FLAG_DST_WKSPC     (uint_type)0x10000000  /* destination workspace                        */

#define NLB_DISP_FLAG_ALL           (NLB_DISP_FLAG_TITLE         | \
                                     NLB_DISP_FLAG_HEADERS       | \
                                     NLB_DISP_FLAG_ITERS         | \
                                     NLB_DISP_FLAG_CLS           | \
                                     NLB_DISP_FLAG_FREQ          | \
                                     NLB_DISP_FLAG_EVICT         | \
                                     NLB_DISP_FLAG_CACHE_INIT    | \
                                     NLB_DISP_FLAG_WR_TYPE       | \
                                     NLB_DISP_FLAG_POSTED_WR     | \
                                     NLB_DISP_FLAG_READS         | \
                                     NLB_DISP_FLAG_CACHE_RD_HITS | \
                                     NLB_DISP_FLAG_CACHE_RD_MISS | \
                                     NLB_DISP_FLAG_RD_BW         | \
                                     NLB_DISP_FLAG_PEAK_RD_BW    | \
                                     /*NLB_DISP_FLAG_AVG_RD_BW     |*/ \
                                     NLB_DISP_FLAG_WRITES        | \
                                     NLB_DISP_FLAG_CACHE_WR_HITS | \
                                     NLB_DISP_FLAG_CACHE_WR_MISS | \
                                     NLB_DISP_FLAG_WR_BW         | \
                                     NLB_DISP_FLAG_PEAK_WR_BW    | \
                                     /*NLB_DISP_FLAG_AVG_WR_BW     |*/ \
                                     NLB_DISP_FLAG_CONT          | \
                                     NLB_DISP_FLAG_CSRS          | \
                                     NLB_DISP_FLAG_QLP_VERSION   | \
                                     NLB_DISP_FLAG_NLB_VERSION   | \
                                     NLB_DISP_FLAG_VERSION       | \
                                     NLB_DISP_FLAG_COPYRIGHT     | \
                                     NLB_DISP_FLAG_SRC_WKSPC     | \
                                     NLB_DISP_FLAG_DST_WKSPC)


void NLBSetupCmdLineParser(aalclp * , struct NLBCmdLine * );
void nlb_help_message_callback(FILE * , struct _aalclp_gcs_compliance_data * );
void MyNLBShowHelp(FILE * , aalclp_gcs_compliance_data * );

END_C_DECLS

/*class NLBConfig
{
public:
   NLBConfig(bool                    bAsynchronous=true,
             wkspc_size_type         SizeInBytes=DEFAULT_NLB_WKSPC_SIZE,
             Workspace               SrcWkspc=NULLWorkspace,
             Workspace               DestWkspc=NULLWorkspace,
             Workspace               DevStatusWkspc=NULLWorkspace,
             INLBVAFU::eNLBCacheInit CacheInit=DEFAULT_NLB_CACHE_INIT,
             INLBVAFU::eNLBWriteType WriteType=DEFAULT_NLB_WRITE_TYPE,
             bool                    bPostedWrites=DEFAULT_NLB_POSTED_WRITES,
             bool                    bContinuousMode=false,
             freq_type               FPGAClkFreqHz=200000000ULL/*TODO DEFAULT_FPGA_CLK_FREQ.Hertz()*//*) throw() :
      m_bAsynchronous(bAsynchronous),
      m_SizeInBytes(SizeInBytes),
      m_SrcWkspc(SrcWkspc),
      m_DestWkspc(DestWkspc),
      m_DevStatusWkspc(DevStatusWkspc),
      m_CacheInit(CacheInit),
      m_WriteType(WriteType),
      m_bPostedWrites(bPostedWrites),
      m_bContinuousMode(bContinuousMode),
      m_FPGAClkFreqHz(FPGAClkFreqHz)
   {
#if   defined( __AAL_WINDOWS__ )
# error TODO
#elif defined( __AAL_LINUX__ )
      struct timespec ts = { DEFAULT_NLB_CONT_TIMEOUT_SEC, DEFAULT_NLB_CONT_TIMEOUT_NS };
      m_ContModeTimeout  = ts;
#endif // OS
   }

   void SetAsynchronous(bool bAsync)                   throw() { m_bAsynchronous = bAsync;    }
   bool GetAsynchronous()                        const throw() { return m_bAsynchronous;      }

   void            SetSizeInBytes(wkspc_size_type sz)  throw() { m_SizeInBytes = sz;          }
   wkspc_size_type GetSizeInBytes()              const throw() { return m_SizeInBytes;        }

   void      SetSrcWkspc(const Workspace &wkspc)       throw()
   {
      m_SrcWkspc = wkspc;
      if ( 0 != m_SrcWkspc.TagName().compare(NULL_WKSPC_TAGNAME) ) {
         m_SrcWkspc.TagName() = "NLB Src";
      }
   }
   Workspace GetSrcWkspc()                       const throw() { return m_SrcWkspc;           }

   void      SetDestWkspc(const Workspace &wkspc)      throw()
   {
      m_DestWkspc = wkspc;
      if ( 0 != m_DestWkspc.TagName().compare(NULL_WKSPC_TAGNAME) ) {
         m_DestWkspc.TagName() = "NLB Dst";
      }
   }
   Workspace GetDestWkspc()                      const throw() { return m_DestWkspc;          }

   void      SetDevStatusWkspc(const Workspace &wkspc) throw()
   {
      m_DevStatusWkspc = wkspc;
      if ( 0 != m_DevStatusWkspc.TagName().compare(NULL_WKSPC_TAGNAME) ) {
         m_DevStatusWkspc.TagName() = "NLB DSM";
      }
   }
   Workspace GetDevStatusWkspc()                 const throw() { return m_DevStatusWkspc;     }

   void SetCacheInit(INLBVAFU::eNLBCacheInit init)     throw() { m_CacheInit = init;          }
   INLBVAFU::eNLBCacheInit GetCacheInit()        const throw() { return m_CacheInit;          }

   void SetWriteType(INLBVAFU::eNLBWriteType type)     throw() { m_WriteType = type;          }
   INLBVAFU::eNLBWriteType GetWriteType()        const throw() { return m_WriteType;          }

   void SetPostedWrites(bool bPostedWr)                throw() { m_bPostedWrites = bPostedWr; }
   bool GetPostedWrites()                        const throw() { return m_bPostedWrites;      }

   void SetContinuous(bool bCont)                      throw() { m_bContinuousMode = bCont;   }
   bool GetContinuous()                          const throw() { return m_bContinuousMode;    }

   void SetFPGAClkFreqHz(freq_type FreqHz)             throw() { m_FPGAClkFreqHz = FreqHz;    }
   freq_type GetFPGAClkFreqHz()                  const throw() { return m_FPGAClkFreqHz;      }

   void SetContModeTimeout(const Timer &t)             throw() { m_ContModeTimeout = t;       }
   Timer GetContModeTimeout()                    const throw() { return m_ContModeTimeout;    }

protected:
   bool                    m_bAsynchronous;
   wkspc_size_type         m_SizeInBytes;    // size of Src / Dest Workspaces
   Workspace               m_SrcWkspc;
   Workspace               m_DestWkspc;
   Workspace               m_DevStatusWkspc;
   INLBVAFU::eNLBCacheInit m_CacheInit;
   INLBVAFU::eNLBWriteType m_WriteType;
   bool                    m_bPostedWrites;
   bool                    m_bContinuousMode;
   freq_type               m_FPGAClkFreqHz;
   Timer                   m_ContModeTimeout;
};*/

bool         NLBVerifyCmdLine(NLBCmdLine       & ,
                              std::ostream     & ) throw();
std::string     NLBCmdLineFor(const NLBCmdLine & ) throw();


BEGIN_C_DECLS

struct MemCpyTestDefaults
{
   phys_type       srcphys;
   phys_type       dstphys;
   wkspc_size_type wkspc_size_bytes;
};

struct MemCpyTestCmdLine
{
#define MCT_CMD_FLAG_SRC_PHYS  (u64_type)0x10000000000  /* --cpu-src-phys   X     (physical address of source workspace)      */
#define MCT_CMD_FLAG_DST_PHYS  (u64_type)0x20000000000  /* --cpu-dest-phys  X     (physical address of destination workspace) */
#define MCT_CMD_WKSPC_SIZE     (u64_type)0x40000000000  /* --cpu-wkspc-size X     (size of workspaces in bytes)               */

   phys_type                       srcphys;
   phys_type                       dstphys;
   wkspc_size_type                 wkspc_size_bytes;

   const struct MemCpyTestDefaults defaults;
};

struct NLBandMemCpyTestCmdLine
{
   struct NLBCmdLine        nlb;
   struct MemCpyTestCmdLine mct;
};

void      HACALpbk1SetupCmdLineParser(aalclp * , struct NLBandMemCpyTestCmdLine * );
void haca_lpbk1_help_message_callback(FILE   * , struct _aalclp_gcs_compliance_data * );
void                HACALpbk1ShowHelp(FILE   * , aalclp_gcs_compliance_data * );

END_C_DECLS



#endif // __NLB_COMMON_H__


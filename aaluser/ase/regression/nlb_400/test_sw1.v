// ***************************************************************************
//
//        Copyright (C) 2008-2013 Intel Corporation All Rights Reserved.
//
// Engineer:            Pratik Marolia
// Create Date:         Thu Jul 28 20:31:17 PDT 2011
// Module Name:         test_sw1.v
// Project:             NLB AFU 
// Description:         hw + sw ping pong test. FPGA initializes a location X,
//                      flag the SW. The SW in turn copies the data to location
//                      Y and flag the FPGA. FPGA reads the data from location Y.
// ***************************************************************************
// ---------------------------------------------------------------------------------------------------------------------------------------------------
//                                         SW test 1
// ---------------------------------------------------------------------------------------------------------------------------------------------------
// Goal:
// Characterize 3 methods of notification from CPU to FPGA:
// 1. polling from AFU
// 2. UMsg without Data
// 3. UMsg with Data
// 3. CSR Write
//
// Test flow:
// 1. Wait on test_goErrorValid
// 2. Start timer. Write N cache lines. WrData= {16{32'h0000_0001}}
// 3. Write Fence.
// 4. FPGA -> CPU Message. Write to address N+1. WrData = {{14{32'h0000_0000}},{64{1'b1}}}
// 5. CPU -> FPGA Message. Configure one of the following methods:
//   a. Poll on Addr N+1. Expected Data [63:32]==32'hffff_ffff
//   b. CSR write to Address 0xB00. Data= Dont Care
//   c. UMsg Mode 0 (with data). UMsg ID = 0
//   d. UMsgH Mode 1 (without data). UMsg ID = 0
// 7. Read N cache lines. Wait for all read completions.
// 6. Stop timer Send test completion.
// 
// test mode selection:
// re2xy_test_cfg[7:6]  Description
// ----------------------------------------
// 2'h0                 Polling method
// 2'h1                 CSR Write
// 2'h2                 UMsg Mode 0 with data
// 2'h3                 UMsgH Mode 1, i.e. hint without data.
//
// Determine test overheads for latency measurements

module test_sw1 #(parameter PEND_THRESH=1, ADDR_LMT=20, MDATA=14)
(
   //---------------------------global signals-------------------------------------------------
   Clk_400,                // input -- Core clock
        
   s12ab_WrAddr,            // output   [ADDR_LMT-1:0]    
   s12ab_WrTID,             // output   [ADDR_LMT-1:0]      
   s12ab_WrDin,             // output   [511:0]             
   s12ab_WrFence,           // output   write fence.
   s12ab_WrEn,              // output   write enable        
   ab2s1_WrSent,            // input                          
   ab2s1_WrAlmFull,         // input                          
  
   s12ab_RdAddr,            // output   [ADDR_LMT-1:0]
   s12ab_RdTID,             // output   [15:0]
   s12ab_RdEn,              // output 
   ab2s1_RdSent,            // input
   
   ab2s1_RdRspValid,        // input                    
   ab2s1_UMsgValid,         // input                    
   ab2s1_CfgValid,          // input    arb:               Cfg valid
   ab2s1_RdRsp,             // input    [15:0]          
   ab2s1_RdRspAddr,         // input    [ADDR_LMT-1:0]  
   ab2s1_RdData,            // input    [511:0]         
    
   ab2s1_WrRspValid,        // input                  
   ab2s1_WrRsp,             // input    [15:0]            
   ab2s1_WrRspAddr,         // input    [ADDR_LMT-1:0]    

   re2xy_go,                // input                 
   re2xy_NumLines,          // input    [31:0]            
   re2xy_Cont,              // input             
   re2xy_test_cfg,          // input    [7:0]  
    
   s12ab_TestCmp,           // output           
   s12ab_ErrorInfo,         // output   [255:0] 
   s12ab_ErrorValid,        // output
   cr2s1_csr_write,
   test_Resetb              // input            
);

   input                      Clk_400;               // csi_top:    Clk_400
   
   output   [ADDR_LMT-1:0]    s12ab_WrAddr;           // arb:        write address
   output   [15:0]            s12ab_WrTID;            // arb:        meta data
   output   [511:0]           s12ab_WrDin;            // arb:        Cache line data
   output                     s12ab_WrFence;          // arb:        write fence
   output                     s12ab_WrEn;             // arb:        write enable.
   input                      ab2s1_WrSent;           // arb:        write issued
   input                      ab2s1_WrAlmFull;        // arb:        write fifo almost full
   
   output   [ADDR_LMT-1:0]    s12ab_RdAddr;           // arb:        Reads may yield to writes
   output   [15:0]            s12ab_RdTID;            // arb:        meta data
   output                     s12ab_RdEn;             // arb:        read enable
   input                      ab2s1_RdSent;           // arb:        read issued
   
   input                      ab2s1_RdRspValid;       // arb:        read response valid
   input                      ab2s1_UMsgValid;        // arb:        UMsg valid
   input                      ab2s1_CfgValid;         // arb:        Cfg valid
   input    [15:0]            ab2s1_RdRsp;            // arb:        read response header
   input    [ADDR_LMT-1:0]    ab2s1_RdRspAddr;        // arb:        read response address
   input    [511:0]           ab2s1_RdData;           // arb:        read data
   
   input                      ab2s1_WrRspValid;       // arb:        write response valid
   input    [15:0]            ab2s1_WrRsp;            // arb:        write response header
   input    [ADDR_LMT-1:0]    ab2s1_WrRspAddr;        // arb:        write response address

   input                      re2xy_go;               // requestor:  start of frame recvd
   input    [31:0]            re2xy_NumLines;         // requestor:  number of cache lines
   input                      re2xy_Cont;             // requestor:  continuous mode
   input    [7:0]             re2xy_test_cfg;         // requestor:  8-bit test cfg register.

   output                     s12ab_TestCmp;          // arb:        Test completion flag
   output   [255:0]           s12ab_ErrorInfo;        // arb:        error information
   output                     s12ab_ErrorValid;       // arb:        test has detected an error
   input                      cr2s1_csr_write;
   input                      test_Resetb;
   
   //------------------------------------------------------------------------------------------------------------------------
   // Rd FSM states
   localparam Vrdfsm_WAIT = 2'h0;
   localparam Vrdfsm_RESP = 2'h1;
   localparam Vrdfsm_READ = 2'h2;
   localparam Vrdfsm_DONE = 2'h3;
   // Wr FSM states
   localparam Vwrfsm_WAIT = 3'h0;
   localparam Vwrfsm_WRITE = 3'h1;
   localparam Vwrfsm_WRFENCE = 3'h2;
   localparam Vwrfsm_UPDTFLAG = 3'h3;
   localparam Vwrfsm_DONE = 3'h4;

   // Rd Poll FSM
   localparam Vpollfsm_WAIT = 2'h0;
   localparam Vpollfsm_READ = 2'h1;
   localparam Vpollfsm_RESP = 2'h2;
   localparam Vpollfsm_DONE = 2'h3;


   reg      [ADDR_LMT-1:0]    s12ab_WrAddr;           // arb:        Writes are guaranteed to be accepted
   wire     [15:0]            s12ab_WrTID;            // arb:        meta data
   reg      [511:0]           wrDin;            	  // arb:        Cache line data
   wire     [511:0]           s12ab_WrDin;            // arb:        Cache line data
   reg                        s12ab_WrEn;             // arb:        write enable
   reg                        s12ab_WrFence;          // arb:        write fence
   reg      [ADDR_LMT-1:0]    s12ab_RdAddr;           // arb:        Reads may yield to writes
   wire     [15:0]            s12ab_RdTID;            // arb:        meta data
   reg                        s12ab_RdEn;             // arb:        read enable
   reg                        s12ab_TestCmp_c;        // arb:        Test completion flag
   reg                        s12ab_TestCmp;          // arb:        Test completion flag
   reg      [255:0]           s12ab_ErrorInfo;        // arb:        error information
   reg                        s12ab_ErrorValid;       // arb:        test has detected an error
     
   reg      [31:0]            Num_RdReqs;
   reg      [31:0]            Num_RdRsp;
   reg      [1:0]             RdFSM;
   reg      [1:0]             PollFSM;
   reg      [31:0]            Num_RdCycle;

   reg      [31:0]            Num_WrReqs;
   reg      [2:0]             WrFSM;
   reg      [31:0]            Num_WrCycle;
   reg                        rd_go;
   reg 						      ErrorValid;         

   wire     [MDATA-2:0]       Wrmdata = s12ab_WrAddr[MDATA-2:0];
   wire     [MDATA-2:0]       Rdmdata = s12ab_RdAddr[MDATA-2:0];
   assign                     s12ab_WrDin  = (WrFSM == Vwrfsm_UPDTFLAG) ? {wrDin[511:0]}  : {wrDin[511:17],s12ab_WrAddr[16:0]};

   assign s12ab_RdTID = {1'b1, Rdmdata};
   assign s12ab_WrTID = {1'b0, Wrmdata};
   
   always @(*)
   begin
	      s12ab_ErrorValid = 1'b0;
		   if (ErrorValid ==1) s12ab_ErrorValid = 1'b1;
//         s12ab_ErrorInfo  = {192'h0, Num_RdCycle, Num_WrCycle};   
         s12ab_TestCmp_c  =   WrFSM==Vwrfsm_DONE
                           && RdFSM==Vrdfsm_DONE;
         s12ab_RdEn       = ( RdFSM == Vrdfsm_READ 
                            ||PollFSM == Vpollfsm_READ );
         s12ab_WrEn       = ( WrFSM == Vwrfsm_WRITE 
                            ||WrFSM == Vwrfsm_UPDTFLAG );
         s12ab_WrFence    = WrFSM == Vwrfsm_WRFENCE;
   end

// Write FSM   
   always @(posedge Clk_400)
   begin
         s12ab_TestCmp  <= s12ab_TestCmp_c;

         case(WrFSM)       /* synthesis parallel_case */
            Vwrfsm_WAIT:            // Wait for CPU to start the test
            begin
               s12ab_WrAddr   <= 0;
               Num_WrReqs     <= 1'b1;
               wrDin    <= {16{32'h0000_0001}};

               if(re2xy_go)
               begin
                   if(re2xy_NumLines != 0)
                       WrFSM   <= Vwrfsm_WRITE;
                   else
                   begin
                       WrFSM   <= Vwrfsm_UPDTFLAG;
                       wrDin <= {{14{32'h0000_0000}},{64{1'b1}}};
                   end
               end
            end

            Vwrfsm_WRITE:           // Move data from FPGA to CPU
            begin
              if(ab2s1_WrSent)
              begin
                  s12ab_WrAddr        <= s12ab_WrAddr + 1'b1;
                  Num_WrReqs          <= Num_WrReqs + 1'b1;
                          
                  if(Num_WrReqs == re2xy_NumLines)
                  begin
                      WrFSM     <= Vwrfsm_WRFENCE;
                  end
              end
            end

            Vwrfsm_WRFENCE:         // Fence- guarantees data is written
            begin
              if(ab2s1_WrSent)
              begin
                    WrFSM       <= Vwrfsm_UPDTFLAG;
                    wrDin <= {{14{32'h0000_0000}},{64{1'b1}}};
              end
            end
           
            Vwrfsm_UPDTFLAG:        // FPGA -> CPU Message saying data is available
            begin
              if(ab2s1_WrSent)
              begin
                      WrFSM        <= Vwrfsm_DONE;
              end
            end
            
            default:
            begin
                WrFSM     <= WrFSM;
            end
         endcase
        

         if(WrFSM==Vwrfsm_WRITE || WrFSM==Vwrfsm_UPDTFLAG || WrFSM==Vwrfsm_WRFENCE )
           Num_WrCycle <= Num_WrCycle + 1'b1;

     if (!test_Resetb)
     begin
         Num_WrCycle    <= 0;
         WrFSM          <= Vwrfsm_WAIT;
         s12ab_TestCmp  <= 0;
     end

   end

// Read FSM   
   always @(posedge Clk_400)
   begin
       
       case(re2xy_test_cfg[7:6])
           2'h0:            // polling method
           begin
               case(PollFSM)
                   Vpollfsm_WAIT:
                   begin
                        s12ab_RdAddr <= re2xy_NumLines[ADDR_LMT-1:0];
                        if(WrFSM==Vwrfsm_DONE)
                            PollFSM <= Vpollfsm_READ;
                   end
                   Vpollfsm_READ:
                   begin
                        if(ab2s1_RdSent)
                           PollFSM <= Vpollfsm_RESP;
                   end
                   Vpollfsm_RESP:
                   begin
                        if(ab2s1_RdRspValid)
                        begin
                            if(ab2s1_RdData[63:32]==32'hffff_ffff)
                            begin
                                rd_go <= 1;
                                PollFSM <= Vpollfsm_DONE;
                            end
                            else
                                PollFSM <= Vpollfsm_READ;
                        end
                   end
                   default: //Vpollfsm_DONE
                   begin
                       PollFSM <= PollFSM;
                   end
               endcase
           end
           2'h1:            // CSR Write
               rd_go   <= cr2s1_csr_write;
           2'h2:            // UMsg Mode 0 (with Data)
               rd_go    <= ab2s1_UMsgValid && ab2s1_RdRsp[15]==1'b0 && ab2s1_RdRsp[5:0]==1'b0;
           2'h3:            // UMsg Mode 1 (with Hint+Data)
               rd_go    <= ab2s1_UMsgValid && ab2s1_RdRsp[15]==1'b1 && ab2s1_RdRsp[5:0]==1'b0;
       endcase

       case(RdFSM)       /* synthesis parallel_case */
            Vrdfsm_WAIT:                            // Read Data payload
            begin
                Num_RdReqs   <= 1'b1;
                Num_RdRsp    <= 0;
                if(rd_go)
                begin
                    s12ab_RdAddr <= 0;
                    if(re2xy_NumLines!=0)
                        RdFSM <= Vrdfsm_READ;
                    else
                        RdFSM <= Vrdfsm_DONE;
                end
            end
 
            Vrdfsm_READ:                             // Read N cache lines
            begin
                if(ab2s1_RdSent)
                begin
                    s12ab_RdAddr        <= s12ab_RdAddr + 1'b1;
                    Num_RdReqs          <= Num_RdReqs + 1'b1;        
                  
                    if(Num_RdReqs == re2xy_NumLines)
                        RdFSM     <= Vrdfsm_RESP;
                end
            end
            Vrdfsm_RESP:                            // Wait untill all reads complete
            begin
                if(Num_RdRsp==re2xy_NumLines)
                        RdFSM     <= Vrdfsm_DONE;
            end
            
            default:
            begin
              RdFSM     <= RdFSM;
            end
       endcase         

         if(ab2s1_RdRspValid)
           Num_RdRsp        <= Num_RdRsp + 1'b1;
		   
			 if(ab2s1_RdRspValid && (RdFSM == Vrdfsm_READ || RdFSM == Vrdfsm_RESP))
			 begin		 
			    if (ab2s1_RdData[16:0] != ab2s1_RdRspAddr[16:0]) 
                begin
			        ErrorValid <= 1'b1;
                    s12ab_ErrorInfo[31:0] <= ab2s1_RdData[16:0];
                    s12ab_ErrorInfo[63:32] <= ab2s1_RdRspAddr[16:0];
                    s12ab_ErrorInfo[95:64] <= ab2s1_RdRsp;
                    s12ab_ErrorInfo[127:96] <= Num_RdRsp;
                end
			    else 
                begin
			        ErrorValid <= 0;
                end
			 end
			
         if(RdFSM == Vrdfsm_READ || RdFSM == Vrdfsm_RESP)
           Num_RdCycle <= Num_RdCycle + 1'b1;


      if (!test_Resetb)
       begin
		   ErrorValid     <= 0;
         s12ab_RdAddr   <= 0;
         Num_RdCycle    <= 0;
         RdFSM          <= Vrdfsm_WAIT;          
         PollFSM        <= Vpollfsm_WAIT;
         rd_go          <= 0;
       end
       
   end

   
endmodule

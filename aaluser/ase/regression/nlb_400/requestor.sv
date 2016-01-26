// ***************************************************************************
//
//        Copyright (C) 2008-2014 Intel Corporation All Rights Reserved.
//
// ***************************************************************************
//
// Engineer :           Pratik Marolia
// Create Date:         Thu Jul 28 20:31:17 PDT 2011
// Last Modified :	Fri 03 Oct 2014 10:38:55 AM PDT
// Module Name:         requestor.v
// Project:             NLB AFU v1.1
//                      Compliant with CCI v2.1
// Description:         accepts requests from arbiter and formats it per cci
//                      spec. It also implements the flow control.
// ***************************************************************************
//
// The requestor accepts the address index from the arbiter, appends that to the source/destination base address and 
// sends out the request to the CCI module. It arbitrates between the read and the write requests, peforms the flow control,
// implements all the CSRs for source address, destination address, status address, wrthru enable, start and stop the test.
//
//
//

import ccip_if_pkg::*;
module requestor #(parameter PEND_THRESH=1, ADDR_LMT=20, TXHDR_WIDTH=61, RXHDR_WIDTH=18, DATA_WIDTH=512)
(

//      ---------------------------global signals-------------------------------------------------
    Clk_16UI               , // in    std_logic;  -- Core clock
    Resetb                 , // in    std_logic;  -- Use SPARINGLY only for control
       // ---------------------------CCI IF signals between CCI and requestor  ---------------------
    cf2ci_sTxPort,
    ci2cf_sRxPort,

    cr2re_src_address,
    cr2re_dst_address,
    cr2re_num_lines,
    cr2re_inact_thresh,
    cr2re_interrupt0,
    cr2re_cfg,
    cr2re_ctl,
    cr2re_dsm_base,
    cr2re_dsm_base_valid,

    ab2re_WrAddr,            // [ADDR_LMT-1:0]      arbiter:        Writes are guaranteed to be accepted
    ab2re_WrTID,             // [15:0]              arbiter:        meta data
    ab2re_WrDin,             // [511:0]             arbiter:        Cache line data
    ab2re_WrFence,           //                     arbiter:        write fence.
    ab2re_WrEn,              //                     arbiter:        write enable
    re2ab_WrSent,            //                     arbiter:        can accept writes. Qualify with write enable
    re2ab_WrAlmFull,         //                     arbiter:        write fifo almost full

    ab2re_RdAddr,            // [ADDR_LMT-1:0]      arbiter:        Reads may yield to writes
    ab2re_RdTID,             // [15:0]              arbiter:        meta data
    ab2re_RdEn,              //                     arbiter:        read enable
    re2ab_RdSent,            //                     arbiter:        read issued

    re2ab_RdRspValid,        //                     arbiter:        read response valid
    re2ab_UMsgValid,         //                     arbiter:        UMsg valid
    re2ab_CfgValid,          //                     arbiter:        Cfg Valid
    re2ab_RdRsp,             // [ADDR_LMT-1:0]      arbiter:        read response header
    re2ab_RdData,            // [511:0]             arbiter:        read data
    re2ab_stallRd,           //                     arbiter:        stall read requests FOR LPBK1

    re2ab_WrRspValid,        //                     arbiter:        write response valid
    re2ab_WrRsp,             // [ADDR_LMT-1:0]      arbiter:        write response header
    re2xy_go,                //                     requestor:      start the test
    re2xy_NumLines,          // [31:0]              requestor:      number of cache lines
    re2xy_Cont,              //                     requestor:      continuous mode
    re2xy_src_addr,          // [31:0]              requestor:      src address
    re2xy_dst_addr,          // [31:0]              requestor:      destination address
    re2xy_test_cfg,          // [7:0]               requestor:      8-bit test cfg register.
    re2ab_Mode,              // [2:0]               requestor:      test mode

    ab2re_TestCmp,           //                     arbiter:        Test completion flag
    ab2re_ErrorInfo,         // [255:0]             arbiter:        error information
    ab2re_ErrorValid,        //                     arbiter:        test has detected an error
    test_Resetb,             //                     requestor:      rest the app
    re2cr_wrlock_n,          //                     requestor:      when low, block csr writes
	
    ab2re_RdLen,
    ab2re_RdSop,
    ab2re_WrLen,
    ab2re_WrSop,
	   
    re2ab_RdRspFormat,
    re2ab_RdRspCLnum,
    re2ab_WrRspFormat,
    re2ab_WrRspCLnum,
	
    re2xy_multiCL_len
);
    //--------------------------------------------------------------------------------------------------------------
    input                   Clk_16UI;               //                      csi_top:        Clk_16UI
    input                   Resetb;                 //                      csi_top:        system Resetb
    
    output t_if_ccip_Tx      cf2ci_sTxPort;
    input  t_if_ccip_Rx      ci2cf_sRxPort;

    input  [63:0]           cr2re_src_address;
    input  [63:0]           cr2re_dst_address;
    input  [31:0]           cr2re_num_lines;
    input  [31:0]           cr2re_inact_thresh;
    input  [31:0]           cr2re_interrupt0;
    input  [31:0]           cr2re_cfg;
    input  [31:0]           cr2re_ctl;
    input  [63:0]           cr2re_dsm_base;
    input                   cr2re_dsm_base_valid;
    
    input  [ADDR_LMT-1:0]   ab2re_WrAddr;           // [ADDR_LMT-1:0]        arbiter:       Writes are guaranteed to be accepted
    input  [15:0]           ab2re_WrTID;            // [15:0]                arbiter:       meta data
    input  [DATA_WIDTH-1:0] ab2re_WrDin;            // [511:0]               arbiter:       Cache line data
    input                   ab2re_WrFence;          //                       arbiter:       write fence 
    input                   ab2re_WrEn;             //                       arbiter:       write enable
    output                  re2ab_WrSent;           //                       arbiter:       write issued
    output                  re2ab_WrAlmFull;        //                       arbiter:       write fifo almost full
    
    input  [ADDR_LMT-1:0]   ab2re_RdAddr;           // [ADDR_LMT-1:0]        arbiter:       Reads may yield to writes
    input  [15:0]           ab2re_RdTID;            // [15:0]                arbiter:       meta data
    input                   ab2re_RdEn;             //                       arbiter:       read enable
    output                  re2ab_RdSent;           //                       arbiter:       read issued
    
    output                  re2ab_RdRspValid;       //                       arbiter:       read response valid
    output                  re2ab_UMsgValid;        //                       arbiter:       UMsg valid
    output                  re2ab_CfgValid;         //                       arbiter:       Cfg valid
    output [15:0]           re2ab_RdRsp;            // [15:0]                arbiter:       read response header
    output [DATA_WIDTH-1:0] re2ab_RdData;           // [511:0]               arbiter:       read data
    output                  re2ab_stallRd;          //                       arbiter:       stall read requests FOR LPBK1
    
    output                  re2ab_WrRspValid;       //                       arbiter:       write response valid
    output [15:0]           re2ab_WrRsp;            // [15:0]                arbiter:       write response header
    
    output                  re2xy_go;               //                       requestor:     start of frame recvd
    output [31:0]           re2xy_NumLines;         // [31:0]                requestor:     number of cache lines
    output                  re2xy_Cont;             //                       requestor:     continuous mode
    output [31:0]           re2xy_src_addr;         // [31:0]                requestor:     src address
    output [31:0]           re2xy_dst_addr;         // [31:0]                requestor:     destination address
    output [7:0]            re2xy_test_cfg;         // [7:0]                 requestor:     8-bit test cfg register.
    output [2:0]            re2ab_Mode;             // [2:0]                 requestor:     test mode
    input                   ab2re_TestCmp;          //                       arbiter:       Test completion flag
    input  [255:0]          ab2re_ErrorInfo;        // [255:0]               arbiter:       error information
    input                   ab2re_ErrorValid;       //                       arbiter:       test has detected an error
    
    output                  test_Resetb;
    output                  re2cr_wrlock_n;
	
    input [1:0]              ab2re_RdLen;
    input                    ab2re_RdSop;
    input [1:0]              ab2re_WrLen;
    input                    ab2re_WrSop;
                           
    output                   re2ab_RdRspFormat;
    output [1:0]             re2ab_RdRspCLnum;
    output                   re2ab_WrRspFormat;
    output [1:0]             re2ab_WrRspCLnum;
	
    output [1:0]             re2xy_multiCL_len;

    //----------------------------------------------------------------------------------------------------------------------
    
    //---------------------------------------------------------
    // CCI-S Request Encodings  ***** DO NOT MODIFY ******
    //---------------------------------------------------------
    localparam       WrThru              = 4'h1;
    localparam       WrLine              = 4'h2;
    localparam       RdLine_S            = 4'h4;
    localparam       WrFence             = 4'h5;
    localparam       RdLine_I            = 4'h6;
    localparam       RdLine_O            = 4'h7;
    localparam       Intr                = 4'h8;    // FPGA to CPU interrupt
    
    //--------------------------------------------------------
    // CCI-S Response Encodings  ***** DO NOT MODIFY ******
    //--------------------------------------------------------
    localparam      RSP_READ             = 4'h0;
    localparam      RSP_CSR              = 4'h1;
    localparam      RSP_WRITE            = 4'h2;
    
    //---------------------------------------------------------
    // Default Values ****** May be MODIFIED ******* 
    //---------------------------------------------------------
    localparam      DEF_SRC_ADDR         = 32'h0400_0000;           // Read data starting from here. Cache aligned Address
    localparam      DEF_DST_ADDR         = 32'h0500_0000;           // Copy data to here. Cache aligned Address
    localparam      DEF_DSM_BASE         = 32'h04ff_ffff;           // default status address
    
   
    //----------------------------------------------------------------------------------
    // Device Status Memory (DSM) Address Map ***** DO NOT MODIFY *****
    // Physical address = value at CSR_AFU_DSM_BASE + Byte offset
    //----------------------------------------------------------------------------------
    //                                     Byte Offset                 Attribute    Width   Comments
    localparam      DSM_STATUS           = 32'h40;                  // RO           512b    test status and error info
    
    //----------------------------------------------------------------------------------------------------------------------
    
    reg  [31:0]             ErrorVector;
    reg  [31:0]             Num_Reads;                              // Number of reads performed
    reg  [31:0]             Num_Writes;                             // Number of writes performed
    reg  [31:0]             Num_ticks_low, Num_ticks_high;
    reg  [9-1:0]            Num_Pend;                               // Number of pending requests  // TODO: was PEND_THRESH (7)
    reg  [31:0]             Num_C0stall;                            // Number of clocks for which Channel0 was throttled
    reg  [31:0]             Num_C1stall;                            // Number of clocks for which channel1 was throttled
    reg  signed [31:0]      Num_RdCredits;                          // For LPBK1: number of read credits
    reg                     re2ab_stallRd;
    reg                     RdHdr_valid;
    reg                     WrHdr_valid_T1, WrHdr_valid_T2;
    reg  [31:0]             wrfifo_addr;
    reg  [DATA_WIDTH-1:0]   wrfifo_data;
    reg                     txFifo_RdAck;
    wire                    txFifo_Dout_v;
    reg                     tx_c0_req_valid, tx_c1_req_valid;
    reg                     rx_c0_resp_valid, rx_c1_resp_valid;
    
    t_if_ccip_Rx            ci2cf_sRxPort_T1;

    reg                     re2ab_CfgValid_d;
    reg                     re2ab_RdSent;
    reg                     status_write;
    reg                     interrupt_sent;
    reg                     send_interrupt;
    
    reg   [31:0]            inact_cnt;
    reg                     inact_timeout;
    reg   [5:0]             delay_lfsr;
    reg   [31:0]            cr_inact_thresh;
    reg                     penalty_start_f;
    reg   [7:0]             penalty_start;
    reg   [7:0]             penalty_end;
(* maxfan=256 *) reg        dsm_status_wren;
    t_ccip_req              rdreq_type;
    t_ccip_req              rnd_rdreq_type;
    reg                     rnd_rdreq_sel;
    
    integer                 i;
    reg   [63:0]            cr_dsm_base;                            // a00h, a04h - DSM base address
    reg   [63:0]            cr_src_address;                         // a20h - source buffer address
    reg   [63:0]            cr_dst_address;                         // a24h - destn buffer address
    reg   [31:0]            cr_num_lines;                           // a28h - Number of cache lines
	reg   [1:0]             cr_multiCL_len;   
    reg   [31:0]            cr_ctl = 0;                             // a2ch - control register to start and stop the test
    reg                     cr_wrthru_en;                           // a34h - [0]    : test configuration- wrthru_en
    reg                     cr_cont;                                // a34h - [1]    : repeats the test sequence, NO end condition
    reg   [2:0]             cr_mode;                                // a34h - [4:2]  : selects test mode
    reg                     cr_delay_en;                            // a34h - [8]    : use start delay
    reg   [1:0]             cr_rdsel, cr_rdsel_q;                   // a34h - [10:9] : read request type
    reg   [7:0]             cr_test_cfg;                            // a34h - [27:0] : configuration within a selected test mode
    reg   [31:0]            cr_interrupt0;                          // a3ch - SW allocates apic id & interrupt vector
    reg                     cr_interrupt_testmode;
    reg                     cr_interrupt_on_error;
    reg   [1:0]             cr_chsel;
    reg   [41:0]            ds_stat_address;                        // 040h - test status is written to this address
    wire  [31:0]            re2xy_src_addr  = cr_src_address;
    wire  [31:0]            re2xy_dst_addr  = cr_dst_address;
    
    
	
    wire                    txFifo_Full;
    wire                    txFifo_AlmFull;
    wire [18:0]             rxfifo_Din      = {ci2cf_sRxPort.C1Hdr.cl_num[1:0],ci2cf_sRxPort.C1Hdr.fmt,ci2cf_sRxPort.C1Hdr.mdata[15:0]};
    wire                    rxfifo_WrEn     = ci2cf_sRxPort.C1WrValid;
    wire                    rxfifo_Full;
    
    wire [18:0]             rxfifo_Dout;
    wire                    rxfifo_Dout_v;
    wire                    test_Resetb     = cr_ctl[0];                // Clears all the states. Either is one then test is out of Reset.
    wire                    test_go         = cr_ctl[1];                // When 0, it allows reconfiguration of test parameters.
    wire [2:0]              re2ab_Mode      = cr_mode;
    wire                    re2ab_WrSent    = !txFifo_Full;             // stop accepting new requests, after status write=1
    wire                    txFifo_WrEn     = (ab2re_WrEn| ab2re_WrFence) && ~txFifo_Full;
    wire [15:0]             txFifo_WrTID;
    wire [ADDR_LMT-1:0]     txFifo_WrAddr;
    wire                    txFifo_WrFence;
	wire                    txFifo_WrSop;
	wire [1:0]              txFifo_WrLen;
    
    wire [41:0]             RdAddr      = cr_src_address[41:0] ^ ab2re_RdAddr;
    wire [41:0]             WrAddr      = cr_dst_address[41:0] ^ txFifo_WrAddr;
    t_ccip_req              wrreq_type;  
    wire [DATA_WIDTH-1:0]   txFifo_WrDin;
    reg  [DATA_WIDTH-1:0]   WrData_dsm;

    reg                     re2xy_go;
    wire   [31:0]           re2xy_NumLines   = cr_num_lines;
    wire   [1:0]            re2xy_multiCL_len= cr_multiCL_len;
    wire                    re2xy_Cont       = cr_cont;
    wire                    re2ab_WrAlmFull  = txFifo_AlmFull;
    wire                    rnd_delay        = ~cr_delay_en || (delay_lfsr[0] || delay_lfsr[2] || delay_lfsr[3]);
    wire   [7:0]            re2xy_test_cfg   = cr_test_cfg;
    wire                    tx_errorValid    = ErrorVector!=0;
    wire                    re2cr_wrlock_n   = test_Resetb & ~test_go;
    reg    [15:0]           dsm_number=0;

    logic                   re2ab_RdRspValid;
    logic                   re2ab_UMsgValid;
    logic  [15:0]           re2ab_RdRsp;
    logic  [DATA_WIDTH-1:0] re2ab_RdData;
    logic                   re2ab_WrRspValid;
    logic  [15:0]           re2ab_WrRsp;
    logic                   re2ab_CfgValid;
	
    logic                    ab2re_RdSop;
    logic [1:0]              ab2re_WrLen;
    logic [1:0]              ab2re_RdLen;
    logic                    ab2re_WrSop;
                        
    logic                    re2ab_RdRspFormat;
    logic [1:0]              re2ab_RdRspCLnum;
    logic                    re2ab_WrRspFormat;
    logic [1:0]              re2ab_WrRspCLnum;

	(* noprune *) logic      test_stop;
	(* noprune *) logic      test_stop_q;
	
	always_comb 
	begin
        re2ab_RdRspValid = ci2cf_sRxPort_T1.C0RdValid;
        re2ab_UMsgValid  = ci2cf_sRxPort_T1.C0UMsgValid;
        re2ab_RdRsp      = ci2cf_sRxPort_T1.C0Hdr.mdata[15:0];
        re2ab_RdRspFormat= ci2cf_sRxPort_T1.C0Hdr.fmt; 
        re2ab_RdRspCLnum = ci2cf_sRxPort_T1.C0Hdr.cl_num[1:0]; 
        re2ab_RdData     = ci2cf_sRxPort_T1.C0Data;
        re2ab_WrRspValid = rxfifo_Dout_v | ci2cf_sRxPort_T1.C0WrValid;
        re2ab_WrRsp      = ci2cf_sRxPort_T1.C0WrValid ? ci2cf_sRxPort_T1.C0Hdr.mdata[15:0] : rxfifo_Dout[15:0];
		re2ab_WrRspFormat= ci2cf_sRxPort_T1.C0WrValid ? ci2cf_sRxPort_T1.C0Hdr.fmt         : rxfifo_Dout[16];
		re2ab_WrRspCLnum = ci2cf_sRxPort_T1.C0WrValid ? ci2cf_sRxPort_T1.C0Hdr.cl_num[1:0] : rxfifo_Dout[18:17];
        re2ab_CfgValid   = re2ab_CfgValid_d;
	end

    always @(*)
    begin
        cr_ctl              = cr2re_ctl;
        cr_dsm_base         = cr2re_dsm_base;
        cr_src_address      = cr2re_src_address;
        cr_dst_address      = cr2re_dst_address;
        cr_num_lines        = cr2re_num_lines;
		cr_inact_thresh     = cr2re_inact_thresh;
        cr_interrupt0       = cr2re_interrupt0;
        
        cr_wrthru_en        = cr2re_cfg[0];
        cr_cont             = cr2re_cfg[1];
        cr_mode             = cr2re_cfg[4:2];
        cr_multiCL_len      = cr2re_cfg[6:5];          // TODO: For Multi CL support
        cr_delay_en         = 1'b0;// FIXME: cr2re_cfg[8];
        cr_rdsel            = cr2re_cfg[10:9];
        cr_test_cfg         = cr2re_cfg[27:20];
        cr_interrupt_on_error = cr2re_cfg[28];
        cr_interrupt_testmode = cr2re_cfg[29];
        cr_chsel              = cr2re_cfg[13:12]; 
    end

    always @(posedge Clk_16UI)
    begin		
        ds_stat_address  <= dsm_offset2addr(DSM_STATUS,cr_dsm_base);
        cr_rdsel_q       <= cr_rdsel;
        delay_lfsr <= {delay_lfsr[4:0], (delay_lfsr[5] ^ delay_lfsr[4]) };

        case(cr_rdsel_q)
            2'h0:   rdreq_type <= eREQ_RDLINE_S;
            2'h1:   rdreq_type <= eREQ_RDLINE_I;
            2'h2:   rdreq_type <= eREQ_RDLINE_I;
            2'h3:   rdreq_type <= rnd_rdreq_type;
        endcase
        rnd_rdreq_sel  <= 0;// FIXME: delay_lfsr%3;
        if(rnd_rdreq_sel)
            rnd_rdreq_type <= eREQ_RDLINE_I;
        else
            rnd_rdreq_type <= eREQ_RDLINE_S;

        if(test_go )                                             
            re2xy_go    <= 1'b1;
        if(status_write)
            re2xy_go    <= 1'b0;
        
        send_interrupt <= status_write && ((cr_interrupt_on_error & tx_errorValid) | cr_interrupt_testmode);
        dsm_status_wren<= ab2re_TestCmp | test_stop;              // Update Status upon test completion

        //Tx Path
        //--------------------------------------------------------------------------
        cf2ci_sTxPort.C1Hdr           <= 0;
        cf2ci_sTxPort.C1WrValid       <= 0;
        cf2ci_sTxPort.C1IntrValid     <= 0;
        cf2ci_sTxPort.C0Hdr           <= 0;
        cf2ci_sTxPort.C0RdValid       <= 0;
		
		// Wait for multi CL request to complete 
		// If Error detected or SW forced test termination
	    // Make sure that multiCL request is completed before sending out DSM Write
		test_stop_q                   <= test_stop;
		if ( ((cr_ctl[2] | tx_errorValid) & !(|txFifo_WrLen) & WrHdr_valid_T2) | test_stop_q )
		test_stop                     <= 1;
		
        // Only supports 1 CL requests
        // Implies SOP=1, cl_len=0
        // Channel 1
        WrData_dsm        <= {     ab2re_ErrorInfo,               // [511:256] upper half cache line
                                   24'h00_0000,penalty_end,       // [255:224] test end overhead in # clks
                                   24'h00_0000,penalty_start,     // [223:192] test start overhead in # clks
                                   Num_Writes,                    // [191:160] Total number of Writes sent
                                   Num_Reads,                     // [159:128] Total number of Reads sent
                                   Num_ticks_high, Num_ticks_low, // [127:64]  number of clks
                                   ErrorVector,                   // [63:32]   errors detected            
                                   15'h0,                         // [31:17]   rsvd
                                   dsm_number,                    // [16:1]   unique id for each dsm status write
                                   1'h1                           // [0]    test completion flag
                             };

            cf2ci_sTxPort.C1Data    <= txFifo_WrDin;

            if ( send_interrupt
                 & !interrupt_sent
                 & !ci2cf_sRxPort_T1.C1TxAlmFull
               )
            begin
                interrupt_sent          <= 1'b1;

                cf2ci_sTxPort.C1Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
                cf2ci_sTxPort.C1Hdr.req_type      <= eREQ_INTR;
                cf2ci_sTxPort.C1Hdr.address[31:0] <= cr_interrupt0;
                cf2ci_sTxPort.C1Hdr.mdata[15:0]   <= 16'hfffc;
 
                cf2ci_sTxPort.C1IntrValid     <= 1'b1;
            end
            else if (re2xy_go & rnd_delay )
            begin
			    /*
                if( dsm_status_wren                                             // Write Fence
                  & !ci2cf_sRxPort_T1.C1TxAlmFull
                  )
                begin                                                           //-----------------------------------
                    if(WrFence_sent==0)
                    begin
                        cf2ci_sTxPort.C1WrValid       <= 1'b1;
                    end
					WrFence_sent                      <= 1'b1;
                    cf2ci_sTxPort.C1Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
                    cf2ci_sTxPort.C1Hdr.req_type      <= eREQ_WRFENCE;        
                    cf2ci_sTxPort.C1Hdr.address[41:0] <= '0;
                    cf2ci_sTxPort.C1Hdr.mdata[15:0]   <= '0;
					cf2ci_sTxPort.C1Hdr.sop           <= 1'b1;                 
                    cf2ci_sTxPort.C1Hdr.cl_len        <= 2'b00;
                    cf2ci_sTxPort.C1Data              <= '0;

                end
				
				if(                                                             // Write DSM Status
                  !ci2cf_sRxPort_T1.C1TxAlmFull
				  & WrFence_sent
                  )
                begin                                                           //-----------------------------------
                    if(status_write==0)
                    begin
                        dsm_number                    <= dsm_number + 1'b1;
                        cf2ci_sTxPort.C1WrValid       <= 1'b1;
                    end
					status_write                      <= 1'b1;

                    cf2ci_sTxPort.C1Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
                    cf2ci_sTxPort.C1Hdr.req_type      <= eREQ_WRLINE_M;
                    cf2ci_sTxPort.C1Hdr.address[41:0] <= ds_stat_address;
                    cf2ci_sTxPort.C1Hdr.mdata[15:0]   <= 16'hffff;
					cf2ci_sTxPort.C1Hdr.sop           <= 1'b1;                 // DSM Write is single CL write
                    cf2ci_sTxPort.C1Hdr.cl_len        <= 2'b00;
                    cf2ci_sTxPort.C1Data              <= WrData_dsm;

                end
				*/
				
				if( dsm_status_wren                                             // Write DSM Status
                  & !ci2cf_sRxPort_T1.C1TxAlmFull
                  )
                begin                                                           //-----------------------------------
                    status_write       <= 1'b1;
                    if(status_write==0)
                    begin
                        dsm_number         <= dsm_number + 1'b1;
                        cf2ci_sTxPort.C1WrValid  <= 1'b1;
                    end

                    cf2ci_sTxPort.C1Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
                    cf2ci_sTxPort.C1Hdr.req_type      <= eREQ_WRLINE_M;
                    cf2ci_sTxPort.C1Hdr.address[41:0] <= ds_stat_address;
                    cf2ci_sTxPort.C1Hdr.mdata[15:0]   <= 16'hffff;
					cf2ci_sTxPort.C1Hdr.sop           <= 1'b1;                 // DSM Write is single CL write
                    cf2ci_sTxPort.C1Hdr.cl_len        <= 2'b00;
                    cf2ci_sTxPort.C1Data              <= WrData_dsm;

                end
				
                else if( WrHdr_valid_T2 & !test_stop )                         // Write to Destination Workspace
                begin                                                          //-------------------------------------
                    cf2ci_sTxPort.C1Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
                    cf2ci_sTxPort.C1Hdr.req_type      <= wrreq_type;
                    cf2ci_sTxPort.C1Hdr.address[41:0] <= WrAddr;
                    cf2ci_sTxPort.C1Hdr.mdata[15:0]   <= txFifo_WrTID;
					cf2ci_sTxPort.C1Hdr.sop           <= txFifo_WrSop;
                    cf2ci_sTxPort.C1Hdr.cl_len        <= txFifo_WrLen;

                    cf2ci_sTxPort.C1WrValid <= 1'b1;
                    Num_Writes        <= Num_Writes + 1'b1;
                end
            end // re2xy_go
 		
		// Channel 0
        if(  re2xy_go && rnd_delay 
          && RdHdr_valid)                                                       // Read from Source Workspace
        begin                                                                   //----------------------------------
            cf2ci_sTxPort.C0Hdr.vc_sel        <= t_ccip_vc'(cr_chsel);
            cf2ci_sTxPort.C0Hdr.req_type      <= rdreq_type;
            cf2ci_sTxPort.C0Hdr.address[41:0] <= RdAddr;
            cf2ci_sTxPort.C0Hdr.mdata[15:0]   <= ab2re_RdTID;
            cf2ci_sTxPort.C0RdValid           <= 1'b1;
			cf2ci_sTxPort.C0Hdr.sop           <= ab2re_RdSop;            // Multi CL support 
            cf2ci_sTxPort.C0Hdr.cl_len        <= ab2re_RdLen;
            Num_Reads                         <= Num_Reads + re2xy_multiCL_len + 1'b1;   
        end

        //--------------------------------------------------------------------------
        //Rx Response Path
        //--------------------------------------------------------------------------
        ci2cf_sRxPort_T1       <= ci2cf_sRxPort;

        // Counters
        //--------------------------------------------------------------------------

        if(re2xy_go)                                                // Count #clks after test start
        begin
            Num_ticks_low   <= Num_ticks_low + 1'b1;
            if(&Num_ticks_low)
                Num_ticks_high  <= Num_ticks_high + 1'b1;
        end

        if(re2xy_go & ci2cf_sRxPort.C0TxAlmFull )                   // TODO: Check Timing
            Num_C0stall     <= Num_C0stall + 1'b1;

        if(re2xy_go & ci2cf_sRxPort.C1TxAlmFull)
            Num_C1stall     <= Num_C1stall + 1'b1;

        tx_c0_req_valid  <= cf2ci_sTxPort.C0RdValid;
        tx_c1_req_valid  <= cf2ci_sTxPort.C1WrValid;
        rx_c0_resp_valid <= ci2cf_sRxPort.C0RdValid | ci2cf_sRxPort.C0WrValid;
        rx_c1_resp_valid <= ci2cf_sRxPort.C1WrValid;

        case({tx_c0_req_valid , tx_c1_req_valid , rx_c0_resp_valid , rx_c1_resp_valid })
            4'b0001:    Num_Pend    <= Num_Pend - 2'h1;
            4'b0010:    Num_Pend    <= Num_Pend - 2'h1;
            4'b0011:    Num_Pend    <= Num_Pend - 2'h2;
            4'b0100:    Num_Pend    <= Num_Pend + 2'h1;
            //4'b0101:    
            //4'b0110:
            4'b0111:    Num_Pend    <= Num_Pend - 2'h1;
            4'b1000:    Num_Pend    <= Num_Pend + 2'h1;
            //4'b1001:    
            //4'b1010:
            4'b1011:    Num_Pend    <= Num_Pend - 2'h1;
            4'b1100:    Num_Pend    <= Num_Pend + 2'h2;
            4'b1101:    Num_Pend    <= Num_Pend + 2'h1;
            4'b1110:    Num_Pend    <= Num_Pend + 2'h1;
            //4'b1111:
        endcase                

        // For LPBK1 (memory copy): stall reads  if Num_RdCredits less than 0. Read credits are limited by the depth of Write fifo
        // Wr fifo depth in requestor is 128. Therefore max num write pending should be less than 128.
        
		/*
		if(cf2ci_sTxPort.C0RdValid && !cf2ci_sTxPort.C1WrValid)
            Num_RdCredits <= Num_RdCredits - 1'b1;
        if(!cf2ci_sTxPort.C0RdValid && (cf2ci_sTxPort.C1WrValid ) )
            Num_RdCredits <= Num_RdCredits + 1'b1;
		*/
		
	    case ({cf2ci_sTxPort.C0RdValid,cf2ci_sTxPort.C1WrValid})               
		  2'b01: 
		  begin
		    if (cf2ci_sTxPort.C1Hdr.sop) 
			Num_RdCredits <= Num_RdCredits + 1'b1;                                // 1Wr sent
		  end
		  
		  2'b10:
          begin		  
		    Num_RdCredits <= Num_RdCredits - 1'b1;                                // 1Rd sent
		  end
		  
		  2'b11: 
		  begin
    		if (!cf2ci_sTxPort.C1Hdr.sop) 
			Num_RdCredits <= Num_RdCredits - 1'b1;                                // 1Rd + 1Wr sent
		  end
		  
		  default: 
		  begin  
		    Num_RdCredits <= Num_RdCredits;
          end
		endcase

        re2ab_stallRd     <= ($signed(Num_RdCredits)<=0);
        
        // Error Detection Logic
        //--------------------------
        // synthesis translate_off
        if(|ErrorVector)
            $finish();
        // synthesis translate_on

        if(Num_Pend<0)
        begin
            ErrorVector[0]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: unexpected Rx response");
            /*synthesis translate_on */
        end

        if(rxfifo_Full & rxfifo_WrEn)
        begin
            ErrorVector[1]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: WrRx fifo overflow");
            /*synthesis translate_on */
        end

        if(txFifo_Full & txFifo_WrEn)
        begin
            ErrorVector[2]  <= 1;
            /*synthesis translate_off */
            $display("nlb_lpbk: Error: wr fifo overflow");
            /*synthesis translate_on */
        end

        if(ErrorVector[3]==0)
            ErrorVector[3]  <= ab2re_ErrorValid;

        /* synthesis translate_off */
        if(cf2ci_sTxPort.C1WrValid)
            $display("*Req Type: %x \t Addr: %x \n Data: %x", cf2ci_sTxPort.C1Hdr.req_type, cf2ci_sTxPort.C1Hdr.address, cf2ci_sTxPort.C1Data);

        if(cf2ci_sTxPort.C0RdValid)
            $display("*Req Type: %x \t Addr: %x", cf2ci_sTxPort.C0Hdr.req_type, cf2ci_sTxPort.C0Hdr.address);

        /* synthesis translate_on */


        // Use for Debug- if no transactions going across the CCI interface # clks > inactivity threshold 
        // than set the flag. You may use this as a trigger signal in logic analyzer
        if(cf2ci_sTxPort.C1WrValid || cf2ci_sTxPort.C0RdValid)
            inact_cnt  <= cr_inact_thresh;
        else if(re2xy_go)
            inact_cnt  <= inact_cnt - 1'b1;

        if(inact_timeout==0)
        begin
            if(inact_cnt==0)
                inact_timeout   <= 1'b1;
        end
        else if(cf2ci_sTxPort.C1WrValid || cf2ci_sTxPort.C0RdValid)
        begin
            inact_timeout   <= 0;
        end

        if(!test_Resetb)
        begin
            Num_Reads               <= 0;
            Num_Writes              <= 0;
            Num_Pend                <= 0;
            Num_ticks_low           <= 0;
            Num_ticks_high          <= 0;
            re2xy_go                <= 0;
//            ci2cf_sRxPort_T1        <= 0;
            re2ab_CfgValid_d        <= 0;
            ErrorVector             <= 0;
            status_write            <= 0;
            interrupt_sent          <= 0;
            send_interrupt          <= 0;
            inact_cnt               <= 0;
            inact_timeout           <= 0;
            delay_lfsr              <= 1;
            Num_C0stall             <= 0;
            Num_C1stall             <= 0;
            Num_RdCredits           <= (2**PEND_THRESH-8);     // TODO:  This is still 128 credits for Rds. But 128 multiCL Reads could in turn lead to 512 writes
			                                                   // So, TxWriteFIFO is made 512 deep and RdCredit return is adjusted accordingly
            dsm_status_wren         <= 0;     
            test_stop               <= 0;
            test_stop_q             <= 0;			
        end
    end

    always @(posedge Clk_16UI)                                                      // Computes NLB start and end overheads
    begin                                                                           //-------------------------------------
        if(!test_go)
        begin
            penalty_start   <= 0;
            penalty_start_f <= 0;
            penalty_end     <= 8'h2;
        end
        else
        begin
            if(!penalty_start_f & (cf2ci_sTxPort.C0RdValid | cf2ci_sTxPort.C1WrValid ))
            begin
                penalty_start_f   <= 1'b1;
                penalty_start     <= Num_ticks_low[7:0];                    /* synthesis translate_off */
                $display ("NLB_INFO : start penalty = %d ", Num_ticks_low); /* synthesis translate_on */
            end

            penalty_end <= penalty_end + 1'b1;
            if( ci2cf_sRxPort.C0WrValid
              | ci2cf_sRxPort.C0RdValid 
              | ci2cf_sRxPort.C0UMsgValid
              | ci2cf_sRxPort.C1WrValid
              )
            begin
                penalty_end     <= 8'h2;
            end

            if(ab2re_TestCmp
              && !ci2cf_sRxPort.C1TxAlmFull
              && !status_write)
            begin                                                       /* synthesis translate_off */
                $display ("NLB_INFO : end penalty = %d ", penalty_end); /* synthesis translate_on */
            end

        end
    end

    always @(*)
    begin
        RdHdr_valid = re2xy_go
        && !status_write
        && rnd_delay
        && !ci2cf_sRxPort.C0TxAlmFull		// TODO: Check Timing 
        && ab2re_RdEn;

        re2ab_RdSent= RdHdr_valid;

        txFifo_RdAck = re2xy_go && rnd_delay  && !ci2cf_sRxPort.C1TxAlmFull && txFifo_Dout_v;
        wrreq_type   = txFifo_WrFence ? eREQ_WRFENCE
                      :cr_wrthru_en   ? eREQ_WRLINE_I
                                      : eREQ_WRLINE_M;

    end
    always @(posedge Clk_16UI)
    begin
        WrHdr_valid_T1 <= txFifo_RdAck;
        WrHdr_valid_T2 <= WrHdr_valid_T1 & re2xy_go;
		if(!test_Resetb)
        begin
            WrHdr_valid_T1 <= 0;
            WrHdr_valid_T2 <= 0;
        end
    end

    //----------------------------------------------------------------------------------------------------------------------------------------------
    //                                                              Instances
    //----------------------------------------------------------------------------------------------------------------------------------------------
    // Tx Write request fifo. Some tests may have writes dependent on reads, i.e. a read response will generate a write request
    // If the CCI-S write channel is stalled, then the write requests will be queued up in this Tx fifo.

    wire [2+1+1+512+ADDR_LMT+15:0]txFifo_Din= {ab2re_WrLen,
	                                           ab2re_WrSop,
	                                           ab2re_WrFence,
                                               ab2re_WrDin,
                                               ab2re_WrAddr, 
                                               ab2re_WrTID
                                              };
    wire [2+1+1+512+ADDR_LMT+15:0]txFifo_Dout;
    assign                  txFifo_WrLen    = txFifo_Dout[2+1+1+DATA_WIDTH+ADDR_LMT+16-1: 1+1+1+DATA_WIDTH+ADDR_LMT+16-1];
	assign                  txFifo_WrSop    = txFifo_Dout[1+1+DATA_WIDTH+ADDR_LMT+16-1];
	assign                  txFifo_WrFence  = txFifo_Dout[1+DATA_WIDTH+ADDR_LMT+16-1];
    assign                  txFifo_WrDin    = txFifo_Dout[ADDR_LMT+16+:DATA_WIDTH];
    assign                  txFifo_WrAddr   = txFifo_Dout[16+:ADDR_LMT];
    assign                  txFifo_WrTID    = txFifo_Dout[15:0];
	
    wire  [9-1:0] txFifo_count;                // TODO: was PEND_THRESH (7)
    sync_C1Tx_fifo #(.DATA_WIDTH  (2+1+1+DATA_WIDTH+ADDR_LMT+16),
                     .CTL_WIDTH   (0),
                     .DEPTH_BASE2 (9),         // TODO: was PEND_THRESH (7)
                     .GRAM_MODE   (3),
                     .FULL_THRESH (2**9-3)     // TODO: was PEND_THRESH (7)  
    )nlb_writeTx_fifo
    (                                       //--------------------- Input  ------------------
        .Resetb         (test_Resetb),
        .Clk            (Clk_16UI),    
        .fifo_din       (txFifo_Din),          
        .fifo_ctlin     (),
        .fifo_wen       (txFifo_WrEn),      
        .fifo_rdack     (txFifo_RdAck),
                                           //--------------------- Output  ------------------
        .T2_fifo_dout      (txFifo_Dout),        
        .T0_fifo_ctlout    (),
        .T0_fifo_dout_v    (txFifo_Dout_v),
        .T0_fifo_empty     (),
        .T0_fifo_full      (txFifo_Full),
        .T0_fifo_count     (txFifo_count),
        .T0_fifo_almFull   (txFifo_AlmFull),
        .T0_fifo_underflow (),
        .T0_fifo_overflow  ()
    ); 

    wire    rxfifo_RdAck    = rxfifo_Dout_v & ~ci2cf_sRxPort_T1.C0WrValid;
    
    // CCI-S could return two write responses per clock, but arbiter can accept only 1 write response per clock. 
    // This fifo will store the second write response
    sbv_gfifo_v2  #(.DATA_WIDTH  ('d19),   // CLnum(2) + FMT(1) MDATA(16)   
                    .DEPTH_BASE2 (PEND_THRESH)   // TODO: This can still be 128 deep 
    )nlb_writeRx_fifo  
    (                                      //--------------------- Input  ------------------
        .Resetb         (test_Resetb),
        .Clk            (Clk_16UI),
        .fifo_din       (rxfifo_Din),          
        .fifo_wen       (rxfifo_WrEn),      
        .fifo_rdack     (rxfifo_RdAck),        
                                           //--------------------- Output  ------------------
        .fifo_dout      (rxfifo_Dout),
        .fifo_dout_v    (rxfifo_Dout_v),
        .fifo_empty     (),
        .fifo_full      (rxfifo_Full),
        .fifo_count     (),
        .fifo_almFull   ()
    );


    // Function: Returns physical address for a DSM register
    function automatic [41:0] dsm_offset2addr;
        input    [9:0]  offset_b;
        input    [63:0] base_b;
        begin
            dsm_offset2addr = base_b[47:6] + offset_b[9:6];
        end
    endfunction

    //----------------------------------------------------------------
    // For signal tap
    //----------------------------------------------------------------
//    `ifdef DEBUG_NLB

        (* noprune *) reg [3:0]                 DEBUG_nlb_error;
        (* noprune *) reg [31:0]                DEBUG_Num_Reads;
        (* noprune *) reg [31:0]                DEBUG_Num_Writes;
        (* noprune *) reg                       DEBUG_inact_timeout;
        (* noprune *) reg [9:0]                 DEBUG_C0TxHdrID;
        (* noprune *) reg [31:0]                DEBUG_C0TxHdrAddr;
        (* noprune *) reg [9:0]                 DEBUG_C1TxHdrID;
        (* noprune *) reg [31:0]                DEBUG_C1TxHdrAddr;
        (* noprune *) reg [16:0]                 DEBUG_C1TxData;
        (* noprune *) reg [9:0]                 DEBUG_C0RxHdrID;
        (* noprune *) reg [8:0]                 DEBUG_C0RxData;
        (* noprune *) reg [9:0]                 DEBUG_C1RxHdrID;
        (* noprune *) reg                       DEBUG_C0TxRdValid;
        (* noprune *) reg                       DEBUG_C0RxRdValid;
        (* noprune *) reg                       DEBUG_C1TxWrValid;
        (* noprune *) reg                       DEBUG_C0RxWrValid;
        (* noprune *) reg                       DEBUG_C1RxWrValid;
        (* noprune *) reg                       DEBUG_txFifo_Dout_v;
        (* noprune *) reg                       DEBUG_txFifo_RdAck;
        (* noprune *) reg                       DEBUG_txFifo_WrEn;
        (* noprune *) reg                       DEBUG_txFifo_Full;
        (* noprune *) reg [4:0]                 DEBUG_txFifo_Din, DEBUG_txFifo_Dout;
        (* noprune *) reg [15:0]                DEBUG_txFifo_WrCount, DEBUG_txFifo_RdCount;
        (* noprune *) reg [9-1:0]               DEBUG_txFifo_count;                            // TODO: was PEND_THRESH (7)


        always @(posedge Clk_16UI)
        begin
            DEBUG_nlb_error[3:0]    <= ErrorVector[3:0];
            DEBUG_Num_Reads         <= Num_Reads;
            DEBUG_Num_Writes        <= Num_Writes;
            DEBUG_inact_timeout     <= inact_timeout;
            DEBUG_C0TxHdrID         <= cf2ci_sTxPort.C0Hdr.mdata[9:0];
            DEBUG_C0TxHdrAddr       <= cf2ci_sTxPort.C0Hdr.address[31:0];
            DEBUG_C1TxHdrID         <= cf2ci_sTxPort.C1Hdr.mdata[9:0];
            DEBUG_C1TxHdrAddr       <= cf2ci_sTxPort.C1Hdr.address[31:0];
            DEBUG_C1TxData          <= cf2ci_sTxPort.C1Data[16:0];
            DEBUG_C0RxHdrID         <= ci2cf_sRxPort.C0Hdr.mdata[9:0];
            DEBUG_C0RxData          <= ci2cf_sRxPort.C0Data[8:0];
            DEBUG_C1RxHdrID         <= ci2cf_sRxPort.C1Hdr.mdata[9:0];
            DEBUG_C0TxRdValid       <= cf2ci_sTxPort.C0RdValid;
            DEBUG_C0RxRdValid       <= ci2cf_sRxPort.C0RdValid;
            DEBUG_C1TxWrValid       <= cf2ci_sTxPort.C1WrValid;
            DEBUG_C0RxWrValid       <= ci2cf_sRxPort.C0WrValid;
            DEBUG_C1RxWrValid       <= ci2cf_sRxPort.C1WrValid;

            DEBUG_txFifo_Dout_v     <= txFifo_Dout_v;
            DEBUG_txFifo_RdAck      <= txFifo_RdAck;
            DEBUG_txFifo_WrEn       <= txFifo_WrEn;
            DEBUG_txFifo_Full       <= txFifo_Full;
            DEBUG_txFifo_Din        <= txFifo_Din[4:0];
            DEBUG_txFifo_Dout       <= txFifo_Dout[4:0];
            DEBUG_txFifo_count      <= txFifo_count;
            if(txFifo_WrEn)
                DEBUG_txFifo_WrCount <= DEBUG_txFifo_WrCount+1'b1;
            if(txFifo_RdAck)
                DEBUG_txFifo_RdCount <= DEBUG_txFifo_RdCount+1'b1;

            if(!test_Resetb)
            begin
                DEBUG_txFifo_WrCount<= 0;
                DEBUG_txFifo_RdCount<= 0;
            end
        end
//    `endif // DEBUG_NLB

endmodule

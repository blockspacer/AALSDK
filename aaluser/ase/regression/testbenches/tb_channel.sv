import ase_pkg::*;

`include "platform.vh"

module tb_channel();

   logic rst, clk, wr_en, rd_en, empty;
   TxHdr_t hdr_in;

   TxHdr_t txhdr_out;
   RxHdr_t rxhdr_out;
   logic valid_out;
   logic [CCIP_DATA_WIDTH-1:0] data_out;

   localparam MAX_ITEMS = 100000;
      
   outoforder_wrf_channel outoforder_wrf_channel
     (clk, rst, hdr_in, 512'b0, wr_en, txhdr_out, rxhdr_out, data_out, valid_out, rd_en, empty, full );

   initial begin
      clk = 0;
      forever begin
	 clk = ~clk;
	 #10;
      end
   end

   int wr_i;

   initial begin
      rst = 1;
      #200;
      rst = 0;
      wait (wr_i == MAX_ITEMS);
      #50000;
      `ifdef ASE_DEBUG
      $display("------- Dropped Transactions -------");
      $display(tb_channel.outoforder_wrf_channel.checkunit.check_array);
      `endif
      $finish;
   end


   always @(posedge clk) begin
      if (rst) begin
	 wr_en <= 0;
	 wr_i  <= 0;
      end
      else if (~full && (wr_i < MAX_ITEMS)) begin
	 hdr_in.vc <= VC_VA;
	 hdr_in.sop <= 1;
	 hdr_in.len <= 2'b11;
	 hdr_in.reqtype <= CCIP_RDLINE_S;
	 hdr_in.addr <= 32'h8400_0000 + wr_i;
	 hdr_in.mdata <= wr_i;
	 wr_i      <= wr_i + 1;
	 wr_en <= 1;
	 hdr_in.rsvd70 <= 0;
	 hdr_in.rsvd63_58 <= 0;
      end
      else begin
	 wr_en <= 0;
      end
   end

   int rd_i;
      
   always @(posedge clk) begin
      if (rst)
	rd_en <= 0;
      else if (~empty) 
      	rd_en <= 1;
      else
	rd_en <= 0;      
   end
   
   always @(posedge clk) begin
      if (rst) begin
	 rd_i <= 0;	 
      end
      else if (valid_out) begin
	 rd_i <= rd_i + 1;	 
      end
   end
   

endmodule
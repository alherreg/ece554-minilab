// Copyright (c) 2020 University of Florida
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Greg Stitt
// University of Florida
//
// Description: This application demonstrates a DMA AFU where the FPGA transfers
// data from an input array into an output array.
// 
// The example demonstrates an extension of the AFU wrapper class that uses
// AFU::malloc() to dynamically allocate virtually contiguous memory that can
// be accessed by both software and the AFU.

// INSTRUCTIONS: Change the configuration settings in config.h to test 
// different types and amounts of data.

#include <cstdlib>
#include <iostream>

#include <opae/utils.h>

#include "AFU.h"
// Contains application-specific information
#include "config.h"
// Auto-generated by OPAE's afu_json_mgr script
#include "afu_json_info.h"

using namespace std;

#define NUM_TESTS 1


int main(int argc, char *argv[]) {

  try {
    // Create an AFU object to provide basic services for the FPGA. The 
    // constructor searchers available FPGAs for one with an AFU with the
    // the specified ID
    AFU afu(AFU_ACCEL_UUID); 
    //afu.reset();
    //std::this_thread::sleep_for(std::chrono::milliseconds(10000));

    for (unsigned test=0; test < NUM_TESTS; test++) {

      // Allocate memory for the FPGA. Any memory used by the FPGA must be 
      // allocated with AFU::malloc(), or AFU::mallocNonvolatile() if you
      // want to pass the pointer to a function that does not have the volatile
      // qualifier. Use of non-volatile pointers is not guaranteed to work 
      // depending on the compiler.   
      auto input  = afu.malloc<dma_data_t>(DATA_AMOUNT);
      auto output  = afu.malloc<dma_data_t>(DATA_AMOUNT);  

      cout << hex << (long) input << endl;
      cout << hex << (long) output << endl;

      cout << "Test " << test << "..." << endl;

      // Initialize the input and output memory.
      for (unsigned i=0; i < DATA_AMOUNT; i++) {
	input[i] = (dma_data_t) rand();
	output[i] = 0;
      }
    
      // Inform the FPGA of the starting read and write address of the arrays.
      afu.write(MMIO_RD_ADDR, (uint64_t) input);
      afu.write(MMIO_WR_ADDR, (uint64_t) output);

      // The FPGA DMA only handles cache-line transfers, so we need to convert
      // the array size to cache lines.
      unsigned total_bytes = DATA_AMOUNT*sizeof(dma_data_t);
      unsigned num_cls = ceil((float) total_bytes / (float) AFU::CL_BYTES);
      afu.write(MMIO_SIZE, num_cls);

      // Start the FPGA DMA transfer.
      afu.write(MMIO_GO, 1);  

      // Wait until the FPGA is done.
      while (afu.read(MMIO_DONE) == 0) {
	if (SLEEP_WHILE_WAITING)
	  std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));
      }
        
      // Verify correct output.
      // NOTE: This could be replaced with memcp, but that only possible
      // when not using volatile data (i.e. AFU::mallocNonvolatile()). 
      unsigned errors = 0;
      for (unsigned i=0; i < DATA_AMOUNT; i++) {
	if (output[i] != input[i]) {
	  errors++;
	}
      }

      cout << "# of resets: " << afu.read(0x0060) << endl;  

    
      if (errors > 0) {
	cout << "FAILURE: DMA Test Failed With " << errors << " errors!!!!" << endl;
	//return EXIT_FAILURE;      
      }
    
      cout << "DMA Test Successful!!!" << endl;
      //return EXIT_SUCCESS;
      afu.free(input);
      afu.free(output);
    } 

    return EXIT_SUCCESS;
  }
  // Exception handling for all the runtime errors that can occur within 
  // the AFU wrapper class.
  catch (const fpga_result& e) {    
    
    // Provide more meaningful error messages for each exception.
    if (e == FPGA_BUSY) {
      cerr << "ERROR: All FPGAs busy." << endl;
    }
    else if (e == FPGA_NOT_FOUND) { 
      cerr << "ERROR: FPGA with accelerator " << AFU_ACCEL_UUID 
	   << " not found." << endl;
    }
    else {
      // Print the default error string for the remaining fpga_result types.
      cerr << "ERROR: " << fpgaErrStr(e) << endl;    
    }
  }
  catch (const runtime_error& e) {    
    cerr << e.what() << endl;
  }
  catch (const opae::fpga::types::no_driver& e) {
    cerr << "ERROR: No FPGA driver found." << endl;
  }

  return EXIT_FAILURE;
}

#include <unistd.h>
#include <signal.h>
#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <vector>
#include <string>
#include <cstring>

#include "sha1.h"
#include "cmdlineparser.h"
#include "logger.h"
#include "sha1_app.h"


using namespace std;
using namespace sda;
using namespace sda::utils;

//#define SHA1_PUB 1
typedef unsigned char u8;

//graceful close
volatile sig_atomic_t g_done = 0;

#ifdef SHA1_PUB
	#include "zmq.hpp"
	#include <jsonxx.h>
#endif

void term(int signum)
{
    g_done = 1;
}

void init_mds(unsigned int *mds) {
	for (unsigned i = 0; i < CHANNELS; i++) {
		mds[i * 16 + 0] = 0x67452301;
		mds[i * 16 + 1] = 0xEFCDAB89;
		mds[i * 16 + 2] = 0x98BADCFE;
		mds[i * 16 + 3] = 0x10325476;
		mds[i * 16 + 4] = 0xC3D2E1F0;
		//for(unsigned j = 0; j < 16-5; j++) {
		//  mds[i*16 + 5 + j] = 0x0;
		//}
	}
}

void init_buf_0(unsigned int *buf) {
	unsigned long int ml = 64;

	unsigned long int ml0 = (ml & 0x00000000000000FF) << 56;
	unsigned long int ml1 = (ml & 0x000000000000FF00) << 40;
	unsigned long int ml2 = (ml & 0x0000000000FF0000) << 24;
	unsigned long int ml3 = (ml & 0x00000000FF000000) << 8;
	unsigned long int ml4 = (ml & 0x000000FF00000000) >> 8;
	unsigned long int ml5 = (ml & 0x0000FF0000000000) >> 24;
	unsigned long int ml6 = (ml & 0x00FF000000000000) >> 40;
	unsigned long int ml7 = (ml & 0xFF00000000000000) >> 56;

	unsigned long int mlbe = ml0 | ml1 | ml2 | ml3 | ml4 | ml5 | ml6 | ml7;

	unsigned int mlbe0 = (mlbe & 0xFFFFFFFF00000000) >> 32;
	unsigned int mlbe1 = (mlbe & 0x00000000FFFFFFFF) >> 0;

	for (unsigned i = 0; i < CHANNELS; i++) {
		buf[i * 16 + 0] = 0;
		buf[i * 16 + 1] = 0;
		buf[i * 16 + 2] = 0x80;
		buf[i * 16 + 3] = 0;
		buf[i * 16 + 4] = 0;
		buf[i * 16 + 5] = 0;
		buf[i * 16 + 6] = 0;
		buf[i * 16 + 7] = 0;
		buf[i * 16 + 8] = 0;
		buf[i * 16 + 9] = 0;
		buf[i * 16 + 10] = 0;
		buf[i * 16 + 11] = 0;
		buf[i * 16 + 12] = 0;
		buf[i * 16 + 13] = 0;
		buf[i * 16 + 14] = mlbe1;
		buf[i * 16 + 15] = mlbe0;
	}
}

void init_buf_count(unsigned int *buf, unsigned int start) {

	unsigned long int ml = 512 * (BLOCKS - 1) + 64;

	unsigned long int ml0 = (ml & 0x00000000000000FF) << 56;
	unsigned long int ml1 = (ml & 0x000000000000FF00) << 40;
	unsigned long int ml2 = (ml & 0x0000000000FF0000) << 24;
	unsigned long int ml3 = (ml & 0x00000000FF000000) << 8;
	unsigned long int ml4 = (ml & 0x000000FF00000000) >> 8;
	unsigned long int ml5 = (ml & 0x0000FF0000000000) >> 24;
	unsigned long int ml6 = (ml & 0x00FF000000000000) >> 40;
	unsigned long int ml7 = (ml & 0xFF00000000000000) >> 56;

	unsigned long int mlbe = ml0 | ml1 | ml2 | ml3 | ml4 | ml5 | ml6 | ml7;

	unsigned int mlbe0 = (mlbe & 0xFFFFFFFF00000000) >> 32;
	unsigned int mlbe1 = (mlbe & 0x00000000FFFFFFFF) >> 0;

	memset(buf, 0, CHANNELS * (BLOCKS - 1) * 64);

	for (unsigned i = 0; i < CHANNELS; i++) {
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 0] = i;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 1] = start;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 2] = 0x80;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 3] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 4] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 5] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 6] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 7] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 8] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 9] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 10] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 11] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 12] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 13] = 0;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 14] = mlbe1;
		buf[CHANNELS * (BLOCKS - 1) * 16 + i * 16 + 15] = mlbe0;
	}
}

void verify_sha1_0(unsigned int *mds) {
	unsigned char ibuf[8];
	std::cout << "VERIFYING" << std::endl;

	for (int i = 0; i < CHANNELS; i++) {
		memset(ibuf, 0, 8);

		unsigned char obuf[20];
		SHA1(ibuf, 8, obuf);

		for (int j = 0; j < 5; j++) {
			unsigned int ref_mds = (unsigned) obuf[j * 4 + 0] << 24
					| (unsigned) obuf[j * 4 + 1] << 16
					| (unsigned) obuf[j * 4 + 2] << 8
					| (unsigned) obuf[j * 4 + 3] << 0;

			if (ref_mds != mds[i * 16 + j]) {
				std::cout << "ERROR: Mismatch!: " << std::hex << std::setw(8)
						<< std::setfill('0') << (uint) ref_mds << " != "
						<< (uint) mds[i * 16 + j] << std::endl;

				std::cout << "mds = ";
				for (unsigned k = 0; k < 5; k++) {
					std::cout << std::hex << std::setw(8) << std::setfill('0')
							<< (uint) mds[i * 16 + k];
				}
				std::cout << std::dec << std::endl;

				std::cout << "ref = ";
				for (unsigned k = 0; k < 20; k++) {
					std::cout << std::hex << std::setw(2) << std::setfill('0')
							<< (uint) obuf[k];
				}
				std::cout << std::dec << std::endl;

				/* Stop */
				j = 5;
			}
		}

	}
}

void verify_sha1(unsigned int start, unsigned int *mds) {
	unsigned char ibuf[(BLOCKS - 1) * 64 + 8];
	std::cout << "VERIFYING" << std::endl;

	for (int i = 0; i < CHANNELS; i++) {
		memset(ibuf, 0, (BLOCKS - 1) * 64);
		memcpy(&ibuf[(BLOCKS - 1) * 64 + 0], &i, 4);
		memcpy(&ibuf[(BLOCKS - 1) * 64 + 4], &start, 4);

		unsigned char obuf[20];
		SHA1(ibuf, (BLOCKS - 1) * 64 + 8, obuf);

		for (int j = 0; j < 5; j++) {
			unsigned int ref_mds = (unsigned) obuf[j * 4 + 0] << 24
					| (unsigned) obuf[j * 4 + 1] << 16
					| (unsigned) obuf[j * 4 + 2] << 8
					| (unsigned) obuf[j * 4 + 3] << 0;

			if (ref_mds != mds[i * 16 + j]) {
				std::cout << "ERROR: Mismatch!: " << std::hex << std::setw(8)
						<< std::setfill('0') << (uint) ref_mds << " != "
						<< (uint) mds[i * 16 + j] << std::endl;

				std::cout << "mds = ";
				for (unsigned k = 0; k < 5; k++) {
					std::cout << std::hex << std::setw(8) << std::setfill('0')
							<< (uint) mds[i * 16 + k];
				}
				std::cout << std::dec << std::endl;

				std::cout << "ref = ";
				for (unsigned k = 0; k < 20; k++) {
					std::cout << std::hex << std::setw(2) << std::setfill('0')
							<< (uint) obuf[k];
				}
				std::cout << std::dec << std::endl;

				/* Stop */
				j = 5;
			}
		}

	}
}

/* sha1_single - run SHA1 once 
 */
void sha1_single(sha1 &host) {
	unsigned int *buf = (unsigned int*) malloc(CHANNELS * BLOCKS * 64);
	unsigned int *mds = (unsigned int*) malloc(CHANNELS * 64);

	if (!buf || !mds) {
		std::cout << "ERROR: Out of Memory!" << std::endl;
		abort();
	}

	init_mds(mds);
	init_buf_count(buf, 0);
	//init_buf_0(buf);

	sha1Runner runner = host.createRunner();

	cl_event event = runner.run(buf, mds);

	clWaitForEvents(1, &event);

	//verify_sha1_0(mds);
	verify_sha1(0, mds);

	free(mds);
	free(buf);
}

/* Call back used for keeping the update kernel constantly active */
void CL_CALLBACK single_callback(cl_event event, cl_int status,
		void *user_data) {
	long long unsigned *blocks = (long long unsigned *) user_data;

	//clReleaseEvent(event);
	__sync_fetch_and_add(blocks, CHANNELS * BLOCKS); /* Atomic add */
}

/* sha1_parallel - run SHA1 for timelimit seconds check performance
 */
double sha1_parallel(sha1 &host, double timelimit, const string& zmq_port) {
	int size = 64;
	unsigned int **buf = (unsigned int**) malloc(sizeof(unsigned int*) * 20);
	unsigned int **mds = (unsigned int**) malloc(sizeof(unsigned int*) * 20);

#ifdef SHA1_PUB
	std::cout << "Setup zeromq publisher" << std::endl;
	zmq::context_t context(1);
	zmq::socket_t publisher(context, ZMQ_PUB);
	string zmq_url = "tcp://*:" + zmq_port;
	publisher.bind(zmq_url);
#endif

	if (!buf || !mds) {
		std::cout << "ERROR: Out of Memory!" << std::endl;
		abort();
	}

	for (int i = 0; i < 20; i++) {
		buf[i] = (unsigned int*) malloc(CHANNELS * BLOCKS * 64);
		mds[i] = (unsigned int*) malloc(CHANNELS * 64);

		init_mds(mds[i]);
		init_buf_count(buf[i], i * CHANNELS);

		if (!buf[i] || !mds[i]) {
			std::cout << "ERROR: Out of Memory!" << std::endl;
			abort();
		}
	}

	long long unsigned blocks = 0;
	long long unsigned blocks_complete = 0;

	std::clock_t timeout = std::clock() + (double) CLOCKS_PER_SEC * timelimit;

	cl_event events[20];
	std::vector<sha1Runner> runners(20, host.createRunner());

	size_t i;
	for (i = 0;; i++) {

		//break gracefully
		if(g_done) {
			LogInfo("Caught SIGTERM...");
			break;
		}
		usleep(10);

		if (i % 10 == 0 && i >= 20) {
			size_t j = ((i - 20) / 10) % 2;
			clWaitForEvents(10, &events[j * 10]);

			//no verify for now
			/*
			 for(int k = 0; k < 10; k++) {
			 verify_sha1((i-20+k) * CHANNELS, mds[j*10 + k]);
			 }
			 */

			//output to zmq for web viz
			u8 zmq_output[CHANNELS * 10];
			for (int k = 0; k < CHANNELS * 10; k++) {
				zmq_output[k] = (unsigned char) (mds[j * 10 + (k / CHANNELS)][(k
						% CHANNELS) * 16] & 0x1F);
			}

#ifdef SHA1_PUB
			jsonxx::Array a;
			for (int k = 0; k < CHANNELS * 10; k++)
				a << zmq_output[k];

			std::string str = a.json();
			std::cout << "output: " << str << std::endl;
			publisher.send((const void*) str.c_str(), str.length());
#endif

			/* Re-init with new data */
			for (int k = 0; k < 10; k++) {
				init_mds(mds[j * 10 + k]);
				init_buf_count(buf[j * 10 + k], (i + k) * CHANNELS);
			}
		}

		 /* After timeout stop */		
		 if (std::clock() > timeout && timelimit > 0.0) {
			 //Stop counting blocks immediately
			 LogInfo("Timeout reached! Stopping...");
			 blocks_complete = blocks;
			 g_done = 1;
			 break;
		 }
		
		blocks_complete = blocks;

		events[i % 20] = runners[i % 20].run(buf[i % 20], mds[i % 20]);

		int err = clSetEventCallback(events[i % 20], CL_COMPLETE,
				single_callback, &blocks);
		if (err != CL_SUCCESS) {
			std::cout << "ERROR: Could not create callback" << std::endl;
			abort();
		}
	}

	if (i >= 10) {
		size_t j = (i / 10 - 2) % 2;
		if (j == 0) {
			clWaitForEvents(10, &events[10]);
			clWaitForEvents(i % 10, &events[0]);
		} else {
			clWaitForEvents(10, &events[0]);
			clWaitForEvents(i % 10, &events[10]);
		}
	} else {
		clWaitForEvents(i, &events[0]);
	}

	double dblocks = blocks_complete * 1.0;
	double dsize = size * 1.0;

	double rate = dblocks * dsize / timelimit * (8.0 / 1024.0 / 1024.0);
	std::cout << "INFO: Block Size: " << size * 8 << " bits" << std::endl;
	std::cout << "INFO: Blocks Processed: " << blocks << std::endl;

	std::cout << "INFO: Rate = " << rate << " MBit/s" << std::endl;

	free(mds);
	free(buf);

	return rate;
}

/* Run SHA1 in a single and parallel mode */
int main(int argc, char** argv) {
	//parse commandline
	LogInfo("Xilinx SHA1 Example Application");
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	sigaction(SIGTERM, &action, NULL);


	string strKernelFullPath = sda::GetApplicationPath() + "/";

	//parse commandline
	CmdLineParser parser;
	parser.addSwitch("--platform-name", "-p", "OpenCl platform vendor name", "Xilinx");
	parser.addSwitch("--device-name", "-d", "OpenCl device name", "fpga0");
	parser.addSwitch("--kernel-file", "-k", "OpenCl kernel file to use");
	parser.addSwitch("--select-device", "-s", "Select from multiple matched devices [0-based index]", "0");
	parser.addSwitch("--time-limit", "-t", "Time limit in seconds, -1 means run forever", "20");
	parser.addSwitch("--zmq-pub-port", "-z", "ZeroMQ publisher port for web visualization", "5010");
	parser.setDefaultKey("--kernel-file");
	
	//parse all command line options
	parser.parse(argc, argv);

	double timelimit = parser.value_to_double("time-limit");
	string str_platform = parser.value("platform-name");
	string str_device = parser.value("device-name");
	string str_kernel = parser.value("kernel-file");
	string str_zmq_port = parser.value("zmq-pub-port");

	LogInfo("Platform: %s, Device: %s", str_platform.c_str(), str_device.c_str());
	LogInfo("Kernel FP: %s", str_kernel.c_str());
	LogInfo("ZMQ PORT: %s", str_zmq_port.c_str());	
	LogInfo("Running for [%f] seconds...", timelimit);

	{
		sha1 fpga(str_platform, str_device, str_kernel.c_str());
		sha1_single(fpga);
		std::cout << "INFO: FPGA Start" << std::endl;
		sha1_parallel(fpga, timelimit, str_zmq_port);
		std::cout << "INFO: FPGA Done" << std::endl;
	}

	std::cout << "INFO: DONE" << std::endl;

}

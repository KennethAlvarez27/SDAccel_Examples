/*******************************************************************************
Vendor: Xilinx
Associated Filename: huffmancodec.cpp
Purpose: SDAccel huffman codec example
Revision History: January 29, 2016

*******************************************************************************
Copyright (C) 2015 XILINX, Inc.

This file contains confidential and proprietary information of Xilinx, Inc. and
is protected under U.S. and international copyright and other intellectual
property laws.

DISCLAIMER
This disclaimer is not a license and does not grant any rights to the materials
distributed herewith. Except as otherwise provided in a valid license issued to
you by Xilinx, and to the maximum extent permitted by applicable law:
(1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, AND XILINX
HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY,
INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR
FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether
in contract or tort, including negligence, or under any other theory of
liability) for any loss or damage of any kind or nature related to, arising under
or in connection with these materials, including for any direct, or any indirect,
special, incidental, or consequential loss or damage (including loss of data,
profits, goodwill, or any type of loss or damage suffered as a result of any
action brought by a third party) even if such damage or loss was reasonably
foreseeable or Xilinx had been advised of the possibility of the same.

CRITICAL APPLICATIONS
Xilinx products are not designed or intended to be fail-safe, or for use in any
application requiring fail-safe performance, such as life-support or safety
devices or systems, Class III medical devices, nuclear facilities, applications
related to the deployment of airbags, or any other applications that could lead
to death, personal injury, or severe property or environmental damage
(individually and collectively, "Critical Applications"). Customer assumes the
sole risk and liability of any use of Xilinx products in Critical Applications,
subject only to applicable laws and regulations governing limitations on product
liability.

THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT
ALL TIMES.

*******************************************************************************/

#include <limits.h>
#include <assert.h>
#include <bitset>
#include <queue>
#include <stack>
#include <sstream>
#include <stdio.h>
#include "huffmancodec.h"
#include "huffmancodec_opencl_cpu.h"
#include "logger.h"
#include "bit_io.h"

using namespace std;

namespace sda {

HuffmanCodec::HuffmanCodec() {
	// TODO Auto-generated constructor stub
	m_verbose = false;
}

HuffmanCodec::~HuffmanCodec() {
	// TODO Auto-generated destructor stub
}

int HuffmanCodec::compute_huffmancodes_recursive(HTreeNode* parent) {
	if(!parent)
		return false;

	HTreeNode* lc = parent->left;
	HTreeNode* rc = parent->right;
	int parent_code = parent->bitcode.code;
	int parent_bitlen = parent->bitcode.bitlen;

	int ctUpdated = 0;

	if(lc && rc) {
		lc->bitcode.code = (parent_code << 1);
		lc->bitcode.bitlen = parent_bitlen + 1;
		ctUpdated += compute_huffmancodes_recursive(lc);

		rc->bitcode.code = (parent_code << 1) + 1;
		rc->bitcode.bitlen = parent_bitlen + 1;
		ctUpdated += compute_huffmancodes_recursive(rc);

		ctUpdated  += 2;
	}

	return ctUpdated;
}

string HuffmanCodec::htree_symbols_tostring(const HTreeNode* pnode, bool inhex) {
	if(!pnode)
		return string("");

	string str;

	if(inhex) {
		char buffer[512];
		for(u32 i=0; i < pnode->symbols.size(); i++) {
			snprintf(buffer, 512, "%x", pnode->symbols[i]);
			str = str + string(buffer);
		}

		str = "0x" + str;
	}
	else {
		vector_to_string(pnode->symbols, str);
	}

	return str;
}

void HuffmanCodec::print_codebook(const CodeBook& book) {
	//print codebook
	int counter = 0;
	for(CodeBook::const_iterator it = book.begin();
		it != book.end(); ++it) {

		counter ++;

		printf("Symbol [%d] 0x%x = [%c] --> %s\n", counter, it->first, it->first, bitcode_to_string(it->second).c_str());
	}
}

string HuffmanCodec::bitcode_to_string(const BitCode& code) {
	string str = "";

	for(u32 i=0; i < code.bitlen; i++) {

		u8 bit = (code.code >> i) & 0x01;
		if(bit)
			str = "1" + str;
		else
			str = "0" + str;
	}
	return str;
}

int HuffmanCodec::bit_length(u8 value) {
	int count = 0;
	for(int i=0; i < 8; i++) {
		 if( (value & (1 << i)) != 0)
			 count = i;
	}

	return count + 1;
}

string HuffmanCodec::binary_string(u8 value) {
	std::bitset<8> x(value);
	stringstream ss;
	ss << x;
	return ss.str();
}


int HuffmanCodec::encode_naive(const string& in_str, vector<u8>& out_data) {
	vector<u8> in_data;
	string_to_vector(in_str, in_data);

	return encode_naive(in_data, out_data);
}

int HuffmanCodec::encode_naive(const vector<u8>& in_data, vector<u8>& out_data) {

	if(in_data.size() == 0)
		return false;

	//1. Visit all symbols and create the mapping of the alphabet and their
	//associated weights
	map<u8, u32> alphabet;
	for(u32 i=0; i<in_data.size(); i++) {
		u8 c = in_data[i];
		if(alphabet.find(c) == alphabet.end()) {
			alphabet[c] = 1;
		}
		else {
			alphabet[c] ++;
		}
	}

	if(m_verbose)
		LogInfo("There are %u unique symbols", alphabet.size());


	//2. Create Huffman tree using priority queue
	std::priority_queue<HTreeNode*, std::vector<HTreeNode*>, GreaterThanByWeight> pq;

	for(map<u8, u32>::const_iterator it = alphabet.begin(); it != alphabet.end(); ++it) {
		HTreeNode* pnode = new HTreeNode();
		pnode->symbols.push_back(it->first);
		pnode->weight = (float)it->second / (float)(in_data.size());
		pnode->left = pnode->right = NULL;
		pnode->bitcode.code = 0;
		pnode->bitcode.bitlen = 0;

		pq.push(pnode);
	}


	HTreeNode* top = pq.top();
	if(top) {
		string strData = htree_symbols_tostring(top);
		if(m_verbose)
			LogInfo("Current front item is: %s, %f", strData.c_str(), top->weight);
	}

	//build the tree
	while(pq.size() > 1) {
		//pop 2
		HTreeNode* s0 = pq.top();
		pq.pop();

		HTreeNode* s1 = pq.top();
		pq.pop();

		//create a combined node
		HTreeNode* pnode = new HTreeNode();
		pnode->symbols.insert(pnode->symbols.end(), s0->symbols.begin(), s0->symbols.end());
		pnode->symbols.insert(pnode->symbols.end(), s1->symbols.begin(), s1->symbols.end());
		pnode->weight = s0->weight + s1->weight;
		pnode->left = s0;
		pnode->right = s1;

		//update codes
		pnode->bitcode.code = 0;
		pnode->bitcode.bitlen = 0;

		//push to q
		pq.push(pnode);
	}

	//now compute the codes
	HTreeNode* root = pq.top();
	compute_huffmancodes_recursive(root);

	//print the tree
	if(m_verbose)
		print_huffman_tree(root);

	//storage for all HTreeNode pointers
	vector<HTreeNode*> storage;
	storage.reserve(2 * alphabet.size());

	//collect all codes for leaves and put them on map
	stack<HTreeNode*> stkNodes;
	stkNodes.push(root);

	//build the dictionary
	CodeBook codebook;
	u32 max_bitlen = 0;
	while(!stkNodes.empty()) {
		HTreeNode* p0 = stkNodes.top();
		stkNodes.pop();
		storage.push_back(p0);

		//if is leaf then capture the node in codebook
		if(p0->left == NULL && p0->right == NULL)
		{
			codebook[ p0->symbols[0] ] = p0->bitcode;
			if(p0->bitcode.bitlen > max_bitlen)
				max_bitlen = p0->bitcode.bitlen;
		}
		else {
			if(p0->left)
				stkNodes.push(p0->left);

			if(p0->right)
				stkNodes.push(p0->right);
		}
	}

	//check the coding
	int depth = htree_depth_recursive(root, 0);
	if(m_verbose) {
		LogInfo("Max Tree Depth = %d, Max BitLength = %d", depth, max_bitlen);
	}


	//print book
	if(m_verbose)
		print_codebook(codebook);


	//3. Encode the entire data using the dictionary
	BitStorage bits;
	for(u32 i=0; i < in_data.size(); i++) {
		assert(codebook[ in_data[i] ].bitlen <= 32);

		u32 bitlen = codebook[ in_data[i] ].bitlen;
		u32 mask = (1 << bitlen) - 1;
		u32 code = (u32) (codebook[ in_data[i] ].code & mask);
		bits.write_multiple_bits(code, bitlen);
	}

	//output
	//header: count of bitlengths + count of codes + count of alphabets + msg len
	//bits data
	out_data.reserve( 8 + 3*codebook.size() + bits.cdata().size());

	//out[0] = number of bit-lengths fields
	out_data.push_back(codebook.size());

	//out[1] = number of codes
	out_data.push_back(codebook.size());

	//out[2] = number of alphabet
	out_data.push_back(codebook.size());

	//out[3] = partial bits in payload
	out_data.push_back(bits.count_total_bits() % 8);

	//message length 4 bytes
	u32 msg_len = in_data.size();
	for(int i=0; i < 4; i++) {
		u8 tmp = (u8)(msg_len >> (i * 8)) & 0xffff;
		out_data.push_back(tmp);
	}

	//write bit-lengths
	for(CodeBook::const_iterator it = codebook.begin(); it != codebook.end(); it++) {
		out_data.push_back(it->second.bitlen); //code
	}

	//write bit codes
	for(CodeBook::const_iterator it = codebook.begin(); it != codebook.end(); it++) {
		out_data.push_back(it->second.code); //code
	}


	//write alphabet
	for(CodeBook::const_iterator it = codebook.begin(); it != codebook.end(); it++) {
		out_data.push_back(it->first);
	}

	//write payload
	for(u32 i = 0; i < bits.cdata().size(); i++) {
		u8 tmp = bits.cdata().at(i);
		out_data.push_back(tmp);
	}

	//cleanup
	for(u32 i=0; i < storage.size(); i++) {
		delete storage[i];
		storage[i] = NULL;
	}

	return true;
}

int HuffmanCodec::decode_naive(const vector<u8>& in_data, string& out_str) {

	vector<u8> out_data;
	if(!decode_naive(in_data, out_data)) {
		LogError("Unable to decode the string!");
	}

	vector_to_string(out_data, out_str);
	return true;
}

int HuffmanCodec::decode_naive(const vector<u8>& in_data, vector<u8>& out_data) {
	vector<u8>::const_iterator it = in_data.begin();

	//in[0] = number of bit-lengths fields
	u8 ctBitLengths = *it;
	it++;

	//in[1] = number of codes
	u8 ctCodes = *it;
	it++;

	//in[2] = number of alphabet
	u8 ctAlphabet = *it;
	it++;

	//in[3] = partial bits
	u8 rem = *it;
	it++;

	//msg length
	u32 msg_len = 0;
	for(int i=0; i < 4; i++) {
		u8 tmp = *it;
		msg_len |= (tmp << (i * 8));
		it++;
	}

	//read bit-lengths
	vector<u8> bitlengths;
	bitlengths.resize(ctBitLengths);
	for(u32 i=0; i < ctBitLengths; i++) {
		bitlengths[i] = *it;
		it++;
	}

	//read bit-codes
	vector<u8> codes;
	codes.resize(ctCodes);
	for(u32 i=0; i < ctCodes; i++) {
		codes[i] = *it;
		it++;
	}

	//read alphabets
	vector<u8> alphabets;
	alphabets.resize(ctAlphabet);
	for(u32 i=0; i < ctAlphabet; i++) {
		alphabets[i] = *it;
		it++;
	}

	u32 bit_count = 0;
	vector<u8> payload;
	payload.reserve(in_data.size());

	while(it != in_data.end()) {
		payload.push_back(*it);
		it++;
	}

	//bit counts
	bit_count = (payload.size() - 1) * 8 + rem;

	//read payload
	BitStorage bits(payload, bit_count);
	bits.begin();


	//Create a codebook from the input
	CodeBook codebook;

	//root node
	HTreeNode* root = new HTreeNode();
	root->left = root->right = NULL;
	root->weight = 0.0f;
	root->bitcode.bitlen = 0;
	root->bitcode.code = 0;

	//storage
	vector<HTreeNode*> storage;
	storage.push_back(root);

	//loop over codes
	for(u32 i=0; i < ctCodes; i++) {
		BitCode bitcode;
		bitcode.bitlen = bitlengths[i];
		bitcode.code = codes[i];

		u8 alpha = alphabets[i];
		if(codebook.find(alphabets[i]) == codebook.end())
			codebook[ alpha ] = bitcode;

		//create all missing tree nodes
		HTreeNode* current = root;

		u8 processed_bitlen = 0;
		u8 processed_code = 0;
		for(u32 j = 0; j < bitcode.bitlen; j++) {
			u8 msb = (bitcode.code >> (bitcode.bitlen - j - 1)) & 0x1;
			processed_code |= (processed_code << 1) + msb;
			processed_bitlen++;

			bool is_leaf = (j == (u32)(bitcode.bitlen - 1));
			bool need_new_node = false;
			bool new_node_on_left = false;

			if((msb == 0) && (current->left == NULL)) {
				need_new_node = true;
				new_node_on_left = true;
			}
			else if((msb == 1) && (current->right == NULL)) {
				need_new_node = true;
			}

			//add missing node
			if(need_new_node) {
				HTreeNode* pnode = new HTreeNode();
				pnode->left = pnode->right = NULL;
				pnode->weight = 0.0f;
				pnode->bitcode.bitlen = processed_bitlen;
				pnode->bitcode.code = processed_code;

				storage.push_back(pnode);
				if(new_node_on_left)
					current->left = pnode;
				else
					current->right = pnode;
			}

			//if 0 goto left otherwise right
			if(msb)
				current = current->right;
			else
				current = current->left;


			//if leaf
			if(is_leaf) {
				current->symbols.push_back(alpha);
				current->left = current->right = NULL;
				current->bitcode.bitlen = processed_bitlen;
				current->bitcode.code = processed_code;
				break;
			}
		}
	}


	//print codebook
	if(m_verbose)
		print_codebook(codebook);

	//read payload and output symbol
	out_data.reserve(msg_len);
	HTreeNode* current = root;
	while(!bits.is_end()) {
		u8 bit = bits.read(1);
		if(bit)
			current = current->right;
		else
			current = current->left;


		//if is leaf
		bool is_leaf = (current->left == NULL && current->right == NULL);
		if(is_leaf) {
			if(current->symbols.size() > 0)
				out_data.push_back(current->symbols[0]);

			//now reset from root again
			current = root;
		}
	}


	//cleanup
	for(u32 i=0; i < storage.size(); i++) {
		delete storage[i];
		storage[i] = NULL;
	}

	return true;
}

void HuffmanCodec::vector_to_string(const vector<u8>& in_vec, string& out_str) {
	stringstream ss;
	for(u32 i=0; i < in_vec.size(); i++) {
		ss << in_vec[i];
	}

	out_str = ss.str();
}

void HuffmanCodec::string_to_vector(const string& in_str, vector<u8>& out_vec) {
	out_vec.resize(in_str.length());

	for(u8 i=0; i < out_vec.size(); i++) {
		out_vec[i] = static_cast<u8>(in_str[i]);
	}
}

void HuffmanCodec::print_huffman_tree(const HTreeNode* root) {
	queue<HTreeNode*> q;
	queue<int> qlevels;
	queue<int> qtabs;
	q.push(const_cast<HTreeNode*>(root));
	qlevels.push(0);

	//int depth = htree_depth_recursive(root, 0);
	qtabs.push(0);
	printf("==========================================\n");

	string str = "";
	int cur_level = 0;
	int prev_level = 0;
	int cur_tab = 0;
	int tab_offset = 0;
	while(!q.empty()) {
		HTreeNode* pc = q.front();
		q.pop();

		//level
		cur_level = qlevels.front();
		qlevels.pop();

		//tabs
		cur_tab = qtabs.front();
		qtabs.pop();

		//print newline
		if(cur_level != prev_level) {
			printf("\n");
			prev_level = cur_level;
			tab_offset = 0;
		}

		//print tabs
		int applied_tabs = max<int>(cur_tab - tab_offset, 0);
		for(int i=0; i < applied_tabs; i++) {
			printf("\t");
		}
		tab_offset += applied_tabs;

		str = htree_symbols_tostring(pc, false);
		//string strbin = binary_string(pc->code);
		//print bitcode based on its length in binary
		string strBitCode = bitcode_to_string(pc->bitcode);
		printf("[%s, c=%s, w=%.3f] ", str.c_str(), strBitCode.c_str(), pc->weight * 100.0);

		if(pc->left && pc->right) {
			q.push(pc->left);
			q.push(pc->right);

			qlevels.push(cur_level + 1);
			int tabs_left_child = max<int>(cur_tab, 0);
			qtabs.push(tabs_left_child);

			qlevels.push(cur_level + 1);
			qtabs.push(tabs_left_child);
		}
	}

	printf("\n");
	printf("==========================================\n");
}




int HuffmanCodec::kernel_encode_string(const string& in_str, vector<u8>& out_data) {
	vector<u8> in_data;
	string_to_vector(in_str, in_data);

	u32 size_out_data = 0;
	encode(&in_data[0], in_data.size(), &out_data[0], &size_out_data, true);

	//resize output array
	out_data.resize(size_out_data);
	encode(&in_data[0], in_data.size(), &out_data[0], &size_out_data, false);

	return (int)size_out_data;
}

int HuffmanCodec::kernel_decode_string(const vector<u8>& in_data, string& out_str) {
	vector<u8> out_data;

	u32 size_out_data = 0;
	decode(const_cast<u8*>(&in_data[0]), in_data.size(), &out_data[0], &size_out_data, true);

	//resize output array
	out_data.resize(size_out_data);
	decode(const_cast<u8*>(&in_data[0]), in_data.size(), &out_data[0], &size_out_data, false);

	vector_to_string(out_data, out_str);

	return (int)size_out_data;
}

int HuffmanCodec::htree_depth_recursive(const HTreeNode* root, int current_depth) {
	if(root->left && root->right) {
		int depth_left = htree_depth_recursive(root->left, current_depth + 1);
		int depth_right = htree_depth_recursive(root->right, current_depth + 1);

		return max<int>(depth_left, depth_right);
	}
	else
		return current_depth;
}

bool HuffmanCodec::write_binary_file(const vector<u8>& data, const string& strFP) {
	return false;
}

}

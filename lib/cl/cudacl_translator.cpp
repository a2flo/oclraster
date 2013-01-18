/*
 *  Flexible OpenCL Rasterizer (oclraster)
 *  Copyright (C) 2012 - 2013 Florian Ziesche
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License only.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if defined(OCLRASTER_CUDA_CL)

#include "cudacl_translator.h"
#include "core/core.h"
#include "core/gl_support.h"
#include <regex>

#if defined(__APPLE__)
#include <CUDA/cuda.h>
#include <CUDA/cudaGL.h>
#else
#include <cuda.h>
#include <cudaGL.h>
#endif

// parameter mappings
struct param_address_space_type_map { const char* type_str; CUDACL_PARAM_ADDRESS_SPACE type; };
static constexpr array<const param_address_space_type_map, 6> address_space_mapping {
	{
		{ "__global", CUDACL_PARAM_ADDRESS_SPACE::GLOBAL },
		{ "global", CUDACL_PARAM_ADDRESS_SPACE::GLOBAL },
		{ "__local", CUDACL_PARAM_ADDRESS_SPACE::LOCAL },
		{ "local", CUDACL_PARAM_ADDRESS_SPACE::LOCAL },
		{ "__constant", CUDACL_PARAM_ADDRESS_SPACE::CONSTANT },
		{ "constant", CUDACL_PARAM_ADDRESS_SPACE::CONSTANT },
	}
};
struct param_type_type_map { const char* type_str; CUDACL_PARAM_TYPE type; };
static constexpr array<const param_type_type_map, 4> type_mapping {
	{
		{ "*", CUDACL_PARAM_TYPE::BUFFER },
		{ "image2d_t", CUDACL_PARAM_TYPE::IMAGE_2D },
		{ "image3d_t", CUDACL_PARAM_TYPE::IMAGE_3D },
		{ "sampler_t", CUDACL_PARAM_TYPE::SAMPLER },
	}
};
struct param_access_type_map { const char* type_str; CUDACL_PARAM_ACCESS type; };
static constexpr array<const param_access_type_map, 6> access_mapping {
	{
		{"__read_only", CUDACL_PARAM_ACCESS::READ_ONLY},
		{ "read_only", CUDACL_PARAM_ACCESS::READ_ONLY },
		{ "__write_only", CUDACL_PARAM_ACCESS::WRITE_ONLY },
		{ "write_only", CUDACL_PARAM_ACCESS::WRITE_ONLY },
		{ "__read_write", CUDACL_PARAM_ACCESS::READ_WRITE },
		{ "read_write", CUDACL_PARAM_ACCESS::READ_WRITE },
	}
};

void cudacl_translate(const string& tmp_name,
					  const string& cl_source,
					  const string& preprocess_options,
					  string& cuda_source,
					  vector<cudacl_kernel_info>& kernels) {
	static oclr_constexpr const char* cuda_preprocess_header = "#include \"oclr_cudacl.h\"\n";
	static oclr_constexpr const char* cuda_header = "#include <cuda_runtime.h>\n#include \"cutil_math.h\"\n";
	
	cuda_source = cuda_preprocess_header + cl_source;
	
	// get kernel signatures:
	// to do this, copy the currently processed "cuda source",
	// preprocess the source with clang,
	// replace all whitespace by a single ' ' (basicly putting everything in one line),
	// and do some regex magic to get the final info
	
	// preprocess cl source with cc/gcc/clang
	fstream c_file(tmp_name+".c", fstream::out);
	c_file << cuda_source << endl;
	c_file.close();
	
	//
	string kernel_source = "";
	string preprocess_call = "clang -E -I /usr/local/cuda/include/ "+preprocess_options+" -pipe "+tmp_name+".c";
	//oclr_msg("preprocess_call: %s", preprocess_call);
	core::system(preprocess_call, kernel_source);
	cuda_source = cuda_header + kernel_source; // use preprocessed source
	
	//
	string rm_output = "";
	core::system("rm "+tmp_name+".c", rm_output);
	
	// replace "__kernel" by "kernel"
	static const regex rx_kernel_name("__kernel");
	kernel_source = regex_replace(kernel_source, rx_kernel_name, "kernel");
	
	static const regex rx_space("\\s+");
	static const regex rx_comment("#(.*)");
	static const regex rx_kernel("kernel([\\w ]+)\\(([^\\)]*)\\)");
	static const regex rx_identifier("[_a-zA-Z]\\w*");
	static const regex rx_identifier_neg("\\W");
	static const regex rx_attributes("__attribute__([\\s]*)\\(\\((.*)\\)\\)");
	
	kernel_source = regex_replace(kernel_source, rx_attributes, "");
	kernel_source = regex_replace(kernel_source, rx_comment, "");
	kernel_source = regex_replace(kernel_source, rx_space, " ");
	
	size_t ws_pos = 0;
	for(sregex_iterator iter(kernel_source.begin(), kernel_source.end(), rx_kernel), end; iter != end; iter++) {
		if(iter->size() == 3) {
			const string params_str = (*iter)[2];
			
			string name_str = core::trim((*iter)[1]);
			if((ws_pos = name_str.find_last_of(" ")) != string::npos) {
				name_str = name_str.substr(ws_pos+1, name_str.size()-ws_pos-1);
				name_str = regex_replace(name_str, rx_identifier_neg, "");
			}
			
			const vector<string> params { core::tokenize(params_str, ',') };
			size_t param_counter = 0;
			vector<cudacl_kernel_info::kernel_param> kernel_parameters;
			for(const auto& param_str : params) {
				string param_name = core::trim(param_str);
				if((ws_pos = param_name.find_last_of(" ")) != string::npos) {
					param_name = param_name.substr(ws_pos+1, param_name.size()-ws_pos-1);
					param_name = regex_replace(param_name, rx_identifier_neg, "");
				}
				else param_name = "<unknown>";
				
				cudacl_kernel_info::kernel_param param(param_name, CUDACL_PARAM_ADDRESS_SPACE::NONE, CUDACL_PARAM_TYPE::NONE, CUDACL_PARAM_ACCESS::NONE);
				for(size_t i = 0; i < address_space_mapping.size(); i++) {
					if(param_str.find(address_space_mapping[i].type_str) != string::npos) {
						get<1>(param) = address_space_mapping[i].type;
					}
				}
				for(size_t i = 0; i < type_mapping.size(); i++) {
					if(param_str.find(type_mapping[i].type_str) != string::npos) {
						get<2>(param) = type_mapping[i].type;
					}
				}
				for(size_t i = 0; i < access_mapping.size(); i++) {
					if(param_str.find(access_mapping[i].type_str) != string::npos) {
						get<3>(param) = access_mapping[i].type;
					}
				}
				
				kernel_parameters.push_back(param);
				
				param_counter++;
			}
			
			kernels.emplace_back(name_str, kernel_parameters);
		}
	}
	
	// replace opencl keywords with cuda keywords
	static const regex rx_cl2cuda_0("# ");
	static const regex rx_cl2cuda_1("(__)?global ");
	static const regex rx_cl2cuda_2("(__)?local ");
	static const regex rx_cl2cuda_3("(__)?private ");
	static const regex rx_cl2cuda_4("(__)?constant ");
	static const regex rx_cl2cuda_5("#pragma");
	static const regex rx_cl2cuda_6("(__)?read_only ");
	static const regex rx_cl2cuda_7("(__)?write_only ");
	static const regex rx_cl2cuda_8("image2d_t");
	static const regex rx_cl2cuda_9("image3d_t");
	static const regex rx_shared_in_inline_func("(inline __device__ )([\\w ]+)\\(([^\\)]*)(__shared__ )([^\\)]*)\\)");
	
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_0, "// ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_1, " ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_2, "__shared__ ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_3, " ");
	//cuda_source = regex_replace(cuda_source, rx_cl2cuda_4, "__constant__ ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_4, " ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_5, "// #pragma");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_6, " ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_7, " ");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_8, "texture<uchar, 4, 0>"); // TODO
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_9, "texture<uchar, 4, 0>"); // TODO
	cuda_source = regex_replace(cuda_source, rx_attributes, "");
	
	// replace as long as there are changes/matches (-> better method for this?)
	size_t src_size = 0;
	do {
		src_size = cuda_source.size();
		cuda_source = regex_replace(cuda_source, rx_shared_in_inline_func, "$1$2($3$5)");
	} while(src_size != cuda_source.size());
	
	// replace "kernel" function by "__global__"
	static const regex rx_cl2cuda_kernel("(__)?kernel");
	cuda_source = regex_replace(cuda_source, rx_cl2cuda_kernel, "__global__");
	
	// mark all kernels as extern "C" to prevent name mangling
	const string find_str = "__global__", insert_str = "extern \"C\" ";
	const size_t find_len = find_str.size(), insert_len = insert_str.size();
	size_t pos, old_pos = 0;
	while((pos = cuda_source.find(find_str, old_pos)) != string::npos) {
		cuda_source.insert(pos, insert_str);
		old_pos = pos + find_len + insert_len;
	}
	
	// replace all vector constructors
	static const vector<pair<const regex, const string>> rx_vec_types {
		{
			{ regex("\\(float2\\)"), "make_float2" },
			{ regex("\\(float3\\)"), "make_float3" },
			{ regex("\\(float4\\)"), "make_float4" },
			{ regex("\\(int2\\)"), "make_int2" },
			{ regex("\\(int3\\)"), "make_int3" },
			{ regex("\\(int4\\)"), "make_int4" },
			{ regex("\\(uint2\\)"), "make_uint2" },
			{ regex("\\(uint3\\)"), "make_uint3" },
			{ regex("\\(uint4\\)"), "make_uint4" },
		}
	};
	for(const auto& rx_vec_type : rx_vec_types) {
		cuda_source = regex_replace(cuda_source, rx_vec_type.first, rx_vec_type.second);
	}
	
	// replace vector accesses
	static const vector<pair<const regex, const string>> rx_vec_accessors {
		{
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+).xyz ([\\+\\-\\*/]*)="), "*((float3*)&$1) $2=" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+).xy ([\\+\\-\\*/]*)="), "*((float2*)&$1) $2=" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+).xyz"), "make_float3($1)" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+).xy"), "make_float2($1)" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+)\\((.*)\\).xyz"), "make_float3($1($2))" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+)\\((.*)\\).xy"), "make_float2($1($2))" },
			{ regex("([A-Za-z0-9_\\[\\]\\.\\->]+).zw"), "make_float2($1.z, $1.w)" },
		}
	};
	for(const auto& rx_vec_access : rx_vec_accessors) {
		cuda_source = regex_replace(cuda_source, rx_vec_access.first, rx_vec_access.second);
	}
}

#endif

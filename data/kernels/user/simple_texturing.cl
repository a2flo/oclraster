
// this is the same for both programs
oclraster_out simple_output {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} output_attributes;

#if defined(OCLRASTER_TRANSFORM_PROGRAM)
//////////////////////////////////////////////////////////////////
// transform program

oclraster_in simple_input {
	float4 vertex;
	float4 normal;
	float4 binormal; // unused
	float4 tangent; // unused
	float2 tex_coord;
} input_attributes;

oclraster_uniforms transform_uniforms {
	mat4 modelview_matrix;
	mat4 rotation_matrix;
} tp_uniforms;

float4 transform_main() {
	const float4 mv_vertex = mat4_mul_vec4(tp_uniforms->modelview_matrix,
										   input_attributes->vertex);
	output_attributes->vertex = mv_vertex;
	output_attributes->normal = mat4_mul_vec4(tp_uniforms->rotation_matrix,
											  input_attributes->normal);
	output_attributes->tex_coord = input_attributes->tex_coord;
	
	return mv_vertex;
}

#elif defined(OCLRASTER_RASTERIZATION_PROGRAM)
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_uniforms rasterize_uniforms {
	float4 camera_position;
	float4 light_position; // .w = light radius ^ 2
	float4 light_color;
} rp_uniforms;

oclraster_images {
	// NOTE: hints are optional (will default to <UINT_8, RGBA>)
	read_only image2d<UINT_8, RGBA> diffuse_texture;
	read_only image2d<FLOAT_32, R> fp_noise;
};
oclraster_framebuffer {
	image2d color;
	depth_image depth;
};

bool rasterize_main() {
	const oclr_sampler_t linear_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_LINEAR;
	const oclr_sampler_t point_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | CLK_FILTER_NEAREST;
	
	const float noise_offset = image_read(fp_noise, point_sampler, output_attributes->tex_coord).x;
	const float noise = image_read(fp_noise, linear_sampler, fragment_coord/100.0f + noise_offset).x;
	
	// check if lit by light (compute attenuation)
	float3 light_dir = rp_uniforms->light_position.xyz - output_attributes->vertex.xyz;
	light_dir /= rp_uniforms->light_position.w;
	float attenuation = 1.0f - dot(light_dir, light_dir) * rp_uniforms->light_position.w;
	if(attenuation > 0.0f) {
		light_dir = normalize(light_dir);
		
		// phong lighting
		float3 diff_color = (float3)(0.0f, 0.0f, 0.0f);
		float3 spec_color = (float3)(0.0f, 0.0f, 0.0f);
		
		const float lambert_term = dot(output_attributes->normal.xyz, light_dir);
		if(lambert_term > 0.0f) {
			diff_color = rp_uniforms->light_color.xyz * lambert_term * attenuation;
			
			float3 view_dir = normalize(rp_uniforms->camera_position.xyz - output_attributes->vertex.xyz);
			float3 R = reflect(-light_dir, output_attributes->normal.xyz);
			float specular = pow(max(dot(R, view_dir), 0.0f), 16.0f);
			spec_color = rp_uniforms->light_color.xyz * attenuation * specular;
		}
		
		float4 color = (float4)(diff_color + spec_color, 1.0f);
		color.xyz *= image_read(diffuse_texture, linear_sampler, output_attributes->tex_coord).xyz;
		color.xyz *= 0.5f + (noise * 0.5f);
		framebuffer->color = color;
	}
	return true;
}
#endif

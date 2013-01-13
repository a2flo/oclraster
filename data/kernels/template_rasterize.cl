
#include "oclr_global.h"
#include "oclr_math.h"
#include "oclr_matrix.h"

typedef struct __attribute__((packed)) {
	unsigned int triangle_count;
} constant_data;

typedef struct __attribute__((packed)) {
	float4 VV0;
	float4 VV1;
	float4 VV2;
	float4 W;
} transformed_data;

//###OCLRASTER_USER_CODE###
//////////////////////////////////////////////////////////////////
// rasterization program
oclraster_out {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} simple_output;

oclraster_uniforms {
	float4 camera_position;
	float4 light_position; // .w = light radius ^ 2
	float4 light_color;
} rasterize_uniforms;

void user_main(float4* pixel_color,
			   const simple_output* output_attributes,
			   const rasterize_uniforms* uniforms,
			   float* depth) {
	// check if lit by light (compute attenuation)
	float3 light_dir = uniforms->light_position.xyz - output_attributes->vertex.xyz;
	light_dir /= uniforms->light_position.w;
	const float attenuation = 1.0f - dot(light_dir, light_dir) * uniforms->light_position.w;
	if(attenuation > 0.0f) {
		light_dir = normalize(light_dir);
		
		// phong lighting
		float3 diff_color = (float3)(0.0f, 0.0f, 0.0f);
		float3 spec_color = (float3)(0.0f, 0.0f, 0.0f);
		
		const float lambert_term = dot(output_attributes->normal.xyz, light_dir);
		if(lambert_term > 0.0f) {
			diff_color = uniforms->light_color.xyz * lambert_term * attenuation;
			
			float3 view_dir = normalize(uniforms->camera_position.xyz - output_attributes->vertex.xyz);
			float3 R = reflect(-light_dir, output_attributes->normal.xyz);
			float specular = pow(max(dot(R, view_dir), 0.0f), 16.0f);
			spec_color = uniforms->light_color.xyz * attenuation * specular;
		}
		
		pixel_color->xyz += diff_color + spec_color;
	}
}

//
kernel void _template_rasterize(//###OCLRASTER_USER_STRUCTS###
								global const unsigned int* index_buffer, // TODO: necessary for now, output should be packed later on
								global simple_output* vertex_output_buffer,
								constant rasterize_uniforms* input_uniforms,
								
								global const transformed_data* transformed_buffer,
								global const unsigned int* triangle_queues_buffer,
								global const unsigned int* queue_sizes_buffer,
								const uint2 tile_size,
								const uint2 bin_count,
								const unsigned int queue_size,
								constant constant_data* cdata,
								const uint2 framebuffer_size,
								write_only image2d_t color_framebuffer,
								write_only image2d_t depth_framebuffer) {
	const unsigned int x = get_global_id(0);
	const unsigned int y = get_global_id(1);
	if(x >= framebuffer_size.x) return;
	if(y >= framebuffer_size.y) return;
	
	float4 pixel_color = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
	
	const float far_plane = 1000.0f; // TODO: get this from somewhere else
	float pixel_depth = far_plane; // TODO: read depth
	
	const unsigned int bin_index = (y / tile_size.y) * bin_count.x + (x / tile_size.x);
	const unsigned int queue_entries = queue_sizes_buffer[bin_index];
	const unsigned int queue_offset = (queue_size * bin_index);
	for(unsigned int queue_entry = 0; queue_entry < queue_entries; queue_entry++) {
		const unsigned int triangle_id = triangle_queues_buffer[queue_offset + queue_entry];
		const float4 VV0 = transformed_buffer[triangle_id].VV0;
		const float4 VV1 = transformed_buffer[triangle_id].VV1;
		const float4 VV2 = transformed_buffer[triangle_id].VV2;
		
		//
		const float3 xy1 = (float3)((float)x, (float)y, 1.0f);
		float4 gad = (float4)(dot(xy1, VV0.xyz),
							  dot(xy1, VV1.xyz),
							  dot(xy1, VV2.xyz),
							  VV0.w);
		if(gad.x >= 0.0f || gad.y >= 0.0f || gad.z >= 0.0f) continue;
		
		// simplified:
		gad /= gad.x + gad.y + gad.z;
		
		// depth test:
		if(gad.w >= pixel_depth) continue;
		
		// reset depth and color
		pixel_depth = gad.w;
		pixel_color = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
		
		//###OCLRASTER_USER_MAIN_CALL###
		const unsigned int indices[3] = {
			index_buffer[triangle_id*3],
			index_buffer[triangle_id*3 + 1],
			index_buffer[triangle_id*3 + 2]
		};
		const simple_output output[3] = {
			vertex_output_buffer[indices[0]],
			vertex_output_buffer[indices[1]],
			vertex_output_buffer[indices[2]],
		};
		simple_output interpolated_output;
		interpolated_output.vertex = interpolate(output[0].vertex, output[1].vertex, output[2].vertex, gad.xyz);
		interpolated_output.normal = interpolate(output[0].normal, output[1].normal, output[2].normal, gad.xyz);
		interpolated_output.tex_coord = interpolate(output[0].tex_coord, output[1].tex_coord, output[2].tex_coord, gad.xyz);
		const rasterize_uniforms uniforms = *input_uniforms;
		user_main(&pixel_color,
				  &interpolated_output,
				  &uniforms,
				  &pixel_depth);
	}
	
	// write last depth (if it has changed)
	if(pixel_depth < far_plane) {
		write_imagef(color_framebuffer, (int2)(x, y), pixel_color);
		write_imagef(depth_framebuffer, (int2)(x, y), (float4)(pixel_depth / far_plane, 0.0f, 0.0f, 1.0f));
	}
}

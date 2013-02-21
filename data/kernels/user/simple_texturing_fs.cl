
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_out simple_output {
	float4 vertex;
	float4 view_vec;
	float4 normal;
	float4 binormal;
	float4 tangent;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms rasterize_uniforms {
	float4 camera_position;
	float4 light_position; // .w = light radius ^ 2
	float4 light_color;
} rp_uniforms;

oclraster_images {
	image2d diffuse_texture;
	image2d normal_texture;
	image2d height_texture;
};

void rasterize_main() {
	// parallax mapping
	const float parallax = 0.03f; // determines the "deepness"
	const float3 view_vec = normalize(output_attributes->view_vec.xyz);
	
	float height = 0.0f;
	float offset = 0.0f;
	float2 parallax_tex_coord = output_attributes->tex_coord;
	for(unsigned int i = 1; i < 4; i++) {
		height += oclr_read_image_lin(height_texture, parallax_tex_coord).x;
		offset = parallax * ((2.0f / convert_float(i)) * height - 1.0f);
		parallax_tex_coord = output_attributes->tex_coord + offset * view_vec.xy;
	}
	
	//
	float3 normal = oclr_read_image_lin(normal_texture, parallax_tex_coord).xyz * 2.0f - 1.0f;
	mat3 nbt_mat;
	nbt_mat.m[0] = output_attributes->tangent.xyz;
	nbt_mat.m[1] = output_attributes->binormal.xyz;
	nbt_mat.m[2] = output_attributes->normal.xyz;
	//normal = normalize(mat3_mul_vec3(nbt_mat, normal));
	normal = normalize(-normal);
	
	// check if lit by light (compute attenuation)
	float3 light_dir = rp_uniforms->light_position.xyz - output_attributes->vertex.xyz;
	light_dir /= rp_uniforms->light_position.w;
	float attenuation = 1.0f - dot(light_dir, light_dir) * rp_uniforms->light_position.w;
	if(attenuation > 0.0f) {
		light_dir = normalize(light_dir);
		
		// phong lighting
		float3 diff_color = (float3)(0.0f, 0.0f, 0.0f);
		float3 spec_color = (float3)(0.0f, 0.0f, 0.0f);
		
		const float lambert_term = dot(normal, light_dir);
		if(lambert_term > 0.0f) {
			//diff_color = rp_uniforms->light_color.xyz * lambert_term * attenuation;
			diff_color = lambert_term * attenuation;
			
			float3 view_dir = normalize(rp_uniforms->camera_position.xyz - output_attributes->vertex.xyz);
			float3 R = reflect(-light_dir, normal);
			float specular = pow(max(dot(R, view_dir), 0.0f), 16.0f);
			//spec_color = rp_uniforms->light_color.xyz * attenuation * specular;
			spec_color = attenuation * specular;
		}
		
		*fragment_color = (float4)(diff_color + spec_color, 1.0f);
		(*fragment_color).xyz *= oclr_read_image_lin(diffuse_texture, parallax_tex_coord).xyz;
	}
}

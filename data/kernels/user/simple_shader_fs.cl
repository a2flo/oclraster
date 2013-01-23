
//////////////////////////////////////////////////////////////////
// rasterization program

oclraster_out simple_output {
	float4 vertex;
	float4 normal;
	float2 tex_coord;
} output_attributes;

oclraster_uniforms rasterize_uniforms {
	float4 camera_position;
	float4 light_position; // .w = light radius ^ 2
	float4 light_color;
} rp_uniforms;

void main() {
	// check if lit by light (compute attenuation)
	float3 light_dir = rp_uniforms->light_position.xyz - output_attributes->vertex.xyz;
	light_dir /= rp_uniforms->light_position.w;
	const float attenuation = 1.0f - dot(light_dir, light_dir) * rp_uniforms->light_position.w;
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
		
		*fragment_color = (float4)(diff_color + spec_color, 1.0f);
	}
}


#ifndef __OCLRASTER_PRIMITIVE_ASSEMBLY_H__
#define __OCLRASTER_PRIMITIVE_ASSEMBLY_H__

#define MAKE_PRIMITIVE_INDICES(indices_var_name)									\
unsigned int index_ids[3];															\
const unsigned int instance_primitive_id = primitive_id % instance_primitive_count;	\
switch(primitive_type) {															\
	case PT_TRIANGLE:																\
		index_ids[0] = instance_primitive_id * 3;									\
		index_ids[1] = index_ids[0] + 1;											\
		index_ids[2] = index_ids[0] + 2;											\
		break;																		\
	case PT_TRIANGLE_STRIP:															\
		index_ids[0] = instance_primitive_id + (1 - (instance_primitive_id % 2));	\
		index_ids[1] = instance_primitive_id + (instance_primitive_id % 2);			\
		index_ids[2] = instance_primitive_id + 2;									\
		break;																		\
	case PT_TRIANGLE_FAN:															\
		index_ids[0] = 0;															\
		index_ids[1] = instance_primitive_id + 1;									\
		index_ids[2] = instance_primitive_id + 2;									\
		break;																		\
}																					\
const unsigned int indices_var_name[3] = {											\
	index_buffer[index_ids[0]] + instance_index_offset,								\
	index_buffer[index_ids[1]] + instance_index_offset,								\
	index_buffer[index_ids[2]] + instance_index_offset								\
};

#endif

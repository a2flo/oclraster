
#ifndef __OCLRASTER_PRIMITIVE_ASSEMBLY_H__
#define __OCLRASTER_PRIMITIVE_ASSEMBLY_H__

#define MAKE_PRIMITIVE_INDICES(indices_var_name)				\
unsigned int index_ids[3];										\
switch(primitive_type) {										\
	case PT_TRIANGLE:											\
		index_ids[0] = triangle_id * 3;							\
		index_ids[1] = index_ids[0] + 1;						\
		index_ids[2] = index_ids[0] + 2;						\
		break;													\
	case PT_TRIANGLE_STRIP:										\
		index_ids[0] = triangle_id + (1 - (triangle_id % 2));	\
		index_ids[1] = triangle_id + (triangle_id % 2);			\
		index_ids[2] = triangle_id + 2;							\
		break;													\
	case PT_TRIANGLE_FAN:										\
		index_ids[0] = 0;										\
		index_ids[1] = triangle_id + 1;							\
		index_ids[2] = triangle_id + 2;							\
		break;													\
}																\
const unsigned int indices_var_name[3] = {						\
	index_buffer[index_ids[0]],									\
	index_buffer[index_ids[1]],									\
	index_buffer[index_ids[2]]									\
};

#endif

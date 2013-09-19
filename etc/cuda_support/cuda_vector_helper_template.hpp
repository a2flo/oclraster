
#define MAKE_VECN_CONCAT(single_type, n) single_type##n
#define MAKE_VECN_EVAL(single_type, n) MAKE_VECN_CONCAT(single_type, n)
#define MAKE_VECN(n) MAKE_VECN_EVAL(SINGLE_TYPE, n)

template<> struct vector_mapping<VEC_TYPE, 1> { typedef SINGLE_TYPE type; typedef VEC_TYPE src_type; static const size_t vec_size = 1; static const size_t src_vec_size = COMPONENT_COUNT; };
template<> struct vector_mapping<VEC_TYPE, 2> { typedef MAKE_VECN(2) type; typedef VEC_TYPE src_type; static const size_t vec_size = 2; static const size_t src_vec_size = COMPONENT_COUNT; };
template<> struct vector_mapping<VEC_TYPE, 3> { typedef MAKE_VECN(3) type; typedef VEC_TYPE src_type; static const size_t vec_size = 3; static const size_t src_vec_size = COMPONENT_COUNT; };
template<> struct vector_mapping<VEC_TYPE, 4> { typedef MAKE_VECN(4) type; typedef VEC_TYPE src_type; static const size_t vec_size = 4; static const size_t src_vec_size = COMPONENT_COUNT; };


// undef previously declared macros
#undef CUDA_VEC_NAME_CONCAT
#undef CUDA_VEC_NAME_EVAL
#undef CUDA_VEC_NAME
#undef CUDA_COMMA
#undef CUDA_SEMICOLON
#undef CUDA_VEC_UNARY_OP_EXPAND
#undef CUDA_VEC_OP_SINGLE_EXPAND
#undef CUDA_VEC_OP_VEC_EXPAND
#undef CUDA_VEC_OP

// vector macros
#define CUDA_VEC_NAME_CONCAT(vec_width) vector##vec_width
#define CUDA_VEC_NAME_EVAL(vec_width) CUDA_VEC_NAME_CONCAT(vec_width)
#define CUDA_VEC_NAME CUDA_VEC_NAME_EVAL(CUDA_VECTOR_WIDTH)
#define CUDA_COMMA ,
#define CUDA_SEMICOLON ;

// expands ("$vector $op $single $sep ...") and ("$vector $op $vector $sep ...") for the resp. vector width
#if (CUDA_VECTOR_WIDTH == 1)
#define CUDA_VEC_UNARY_OP_EXPAND(op, rhs, sep) op rhs x
#define CUDA_VEC_OP_SINGLE_EXPAND(lhs, op, rhs, sep) lhs x op rhs
#define CUDA_VEC_OP_VEC_EXPAND(lhs, op, rhs, sep) lhs x op rhs x
#elif (CUDA_VECTOR_WIDTH == 2)
#define CUDA_VEC_UNARY_OP_EXPAND(op, rhs, sep) op rhs x sep op rhs y
#define CUDA_VEC_OP_SINGLE_EXPAND(lhs, op, rhs, sep) lhs x op rhs sep lhs y op rhs
#define CUDA_VEC_OP_VEC_EXPAND(lhs, op, rhs, sep) lhs x op rhs x sep lhs y op rhs y
#elif (CUDA_VECTOR_WIDTH == 3)
#define CUDA_VEC_UNARY_OP_EXPAND(op, rhs, sep) op rhs x sep op rhs y sep op rhs z
#define CUDA_VEC_OP_SINGLE_EXPAND(lhs, op, rhs, sep) lhs x op rhs sep lhs y op rhs sep lhs z op rhs
#define CUDA_VEC_OP_VEC_EXPAND(lhs, op, rhs, sep) lhs x op rhs x sep lhs y op rhs y sep lhs z op rhs z
#elif (CUDA_VECTOR_WIDTH == 4)
#define CUDA_VEC_UNARY_OP_EXPAND(op, rhs, sep) op rhs x sep op rhs y sep op rhs z sep op rhs w sep
#define CUDA_VEC_OP_SINGLE_EXPAND(lhs, op, rhs, sep) lhs x op rhs sep lhs y op rhs sep lhs z op rhs sep lhs w op rhs
#define CUDA_VEC_OP_VEC_EXPAND(lhs, op, rhs, sep) lhs x op rhs x sep lhs y op rhs y sep lhs z op rhs z sep lhs w op rhs w
#endif

// simple vector operation that either returns a new vector object or is the resp. assignment operation
#define CUDA_VEC_OP(op) \
	cuda_device CUDA_VEC_NAME operator op (const T& val) const { \
		return CUDA_VEC_NAME(CUDA_VEC_OP_SINGLE_EXPAND(this->, op, val, CUDA_COMMA)); \
	} \
	cuda_device CUDA_VEC_NAME operator op (const CUDA_VEC_NAME& vec) const { \
		return CUDA_VEC_NAME(CUDA_VEC_OP_VEC_EXPAND(this->, op, vec., CUDA_COMMA)); \
	} \
	cuda_device friend CUDA_VEC_NAME operator op (const T& val, const CUDA_VEC_NAME& v) { \
		return CUDA_VEC_NAME(CUDA_VEC_OP_SINGLE_EXPAND(v., op, val, CUDA_COMMA)); \
	} \
	cuda_device void operator op##= (const T& val) { \
		CUDA_VEC_OP_SINGLE_EXPAND(this->, op##=, val, CUDA_SEMICOLON); \
	} \
	cuda_device void operator op##= (const CUDA_VEC_NAME& vec) { \
		CUDA_VEC_OP_VEC_EXPAND(this->, op##=, vec., CUDA_SEMICOLON); \
	}

#define CUDA_VEC_UNARY_OP(op) \
	cuda_device CUDA_VEC_NAME operator op () const { \
		return CUDA_VEC_NAME(CUDA_VEC_UNARY_OP_EXPAND(op, this->, CUDA_COMMA)); \
	}

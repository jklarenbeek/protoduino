#ifndef __CC_H__
#define __CC_H__

#define CC_UNUSED __attribute__ ((unused))
#define CC_INLINE inline

#define CC_INLINEFN(f) inline f  __attribute__((always_inline))
#define CC_WEAKFN(f) f __attribute__((weak))

#define CC_CONCAT2EXT(s1, s2) s1##s2
#define CC_CONCAT2(s1, s2) CC_CONCAT2EXT(s1, s2)


#define CC_CONST_PTYPE_ARRAY(type, varname) const type varname[] PROGMEM

#define CC_EXPORT_CONST_PTYPE_ARRAY(type, varname) extern const type varname[]

#define CC_CONST_PSTR(varname, value) CC_CONST_PTYPE_ARRAY(char, varname) = value;
//#define CC_CONST_PSTR(var, val) const char var[] PROGMEM = val

#define CC_EXPORT_CONST_PSTR(varname) CC_EXPORT_CONST_PTYPE_ARRAY(char, varname)

#define CC_CONST_PSTRUCT_ARR(type, varname) CC_CONST_PTYPE_ARRAY(struct type, varname)
//#define CC_CONST_PSTRUCT_ARR(type, varname) const struct type varname[] PROGMEM

#define CC_NELEM(x) (sizeof (x)/sizeof (x)[0])

#endif
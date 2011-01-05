#ifndef __FWTS_IASL_INTERFACE__
#define __FWTS_IASL_INTERFACE__

int fwts_iasl_disassemble_aml(const char *aml, const char *outputfile);
int fwts_iasl_assemble_aml(const char *source, char **output);

#endif

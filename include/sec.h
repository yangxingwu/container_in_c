#ifndef __SEC_H__
#define __SEC_H__

// Used to represent the result of a seccomp rule.
// #define	EPERM		 1	/* Operation not permitted */
#define SEC_SCMP_FAIL SCMP_ACT_ERRNO(1)

// Setup capabilities for the calling process
// libcap: used to set container capabilities
int sec_set_caps(void);

// Setup seccomp for the calling process
// libseccomp: used to set up seccomp filters
int sec_set_seccomp(void);

#endif

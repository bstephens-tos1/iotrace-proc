#ifndef _IO_PROC_H_
#define _IO_PROC_H_

#define IO_TRACE_BUF_SIZE 2000000
#define IO_TRACE_BUF_LIMIT (IO_TRACE_BUF_SIZE - 1000)

// Always require the user to call getnstimeofday(), or have a version of
// this function which does that for them?
void stamp();

#endif
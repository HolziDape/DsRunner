// SPDX-License-Identifier: Zlib
//
// DsRunner Stream: decoder benchmark with recorded frames of the test page
// (stream/ds/data, made with python -m dsstream.record).

#ifndef DSSTREAM_BENCH_H
#define DSSTREAM_BENCH_H

// Decodes the recorded streams onto the top screen and prints the times on the
// status console. Uses timers 0 and 1.
void bench_run(void);

#endif

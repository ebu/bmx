# Test creating an MXF OP1a file using the ARD ZDF XDF profile, containing XAVC Long GOP essence
# (AVC High 4:2:2, 1080p50, 50 Mbit/s, GOP size 24, 4 slices per frame) and 8 PCM tracks.
# The essence covers 2 complete GOPs (the first is 22 frames due to encoder lookahead, the
# second is a full 24-frame GOP) plus the start of a third GOP, confirming SPS/PPS repeat
# correctly at GOP boundaries.

include("${TEST_SOURCE_DIR}/test_common.cmake")

run_test(avc_high_422 avc_high_422 xavc_longgop_50mbit_1080p50.h264 60)

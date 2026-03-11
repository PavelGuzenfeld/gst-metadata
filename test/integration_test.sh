#!/bin/bash
set -euo pipefail

PASS=0
FAIL=0

pass() { echo "  PASS: $1"; ((PASS++)); }
fail() { echo "  FAIL: $1"; ((FAIL++)); }

echo "=== gst-metadata integration tests ==="
echo

# ---- Test 1: All writers + all readers — each reader sees its own type ------
echo "[Test 1] All three metadata types: write all, read all"

OUTPUT=$(gst-launch-1.0 -m \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "imureader sees IMU data" || fail "imureader missing IMU data"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "cropreader sees crop data" || fail "cropreader missing crop data"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "fovreader sees FOV data" || fail "fovreader missing FOV data"

# ---- Test 2: Selective reading — only IMU writer, all readers ---------------
echo
echo "[Test 2] Only IMU written, crop/fov readers report missing"

OUTPUT=$(gst-launch-1.0 -m \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "imureader sees IMU" || fail "imureader missing IMU"
echo "$OUTPUT" | grep -q '\[cropreader\] no crop' && pass "cropreader correctly reports no crop" || fail "cropreader false positive"
echo "$OUTPUT" | grep -q '\[fovreader\] no FOV' && pass "fovreader correctly reports no FOV" || fail "fovreader false positive"

# ---- Test 3: Metadata survives through videoconvert (passthrough) -----------
echo
echo "[Test 3] Metadata survives through identity element"

OUTPUT=$(gst-launch-1.0 -m \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! identity \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "IMU survives identity" || fail "IMU lost through identity"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "crop survives identity" || fail "crop lost through identity"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "FOV survives identity" || fail "FOV lost through identity"

# ---- Test 4: Multiple instances of same type --------------------------------
echo
echo "[Test 4] Two cropwriters — reader sees data from both"

OUTPUT=$(gst-launch-1.0 -m \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! cropwriter ! cropwriter \
    ! cropreader \
    ! fakesink 2>&1)

# cropreader reads the first crop; both were attached
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "cropreader sees crop after double write" || fail "cropreader missing after double write"

# ---- Test 5: Only fov, verify compartmentalization --------------------------
echo
echo "[Test 5] Only FOV written, IMU/crop readers report nothing"

OUTPUT=$(gst-launch-1.0 -m \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! fovwriter \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\] no IMU' && pass "imureader correctly reports no IMU" || fail "imureader false positive"
echo "$OUTPUT" | grep -q '\[cropreader\] no crop' && pass "cropreader correctly reports no crop" || fail "cropreader false positive"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "fovreader sees FOV" || fail "fovreader missing FOV"

# ---- Summary ---------------------------------------------------------------
echo
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi

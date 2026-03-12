#!/bin/bash
set -euo pipefail

PASS=0
FAIL=0

pass() { echo "  PASS: $1"; PASS=$((PASS + 1)); }
fail() { echo "  FAIL: $1"; FAIL=$((FAIL + 1)); }

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

# ---- Test 6: Metadata survives videoconvert (format change) ----------------
echo
echo "[Test 6] Metadata survives videoconvert (I420 → default)"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,format=I420,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! videoconvert \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "IMU survives videoconvert" || fail "IMU lost through videoconvert"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "crop survives videoconvert" || fail "crop lost through videoconvert"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "FOV survives videoconvert" || fail "FOV lost through videoconvert"

# ---- Test 7: Metadata survives videoscale (resolution change) --------------
echo
echo "[Test 7] Metadata survives videoscale (320x240 → 160x120)"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! videoscale ! video/x-raw,width=160,height=120 \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "IMU survives videoscale" || fail "IMU lost through videoscale"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "crop survives videoscale" || fail "crop lost through videoscale"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "FOV survives videoscale" || fail "FOV lost through videoscale"

# ---- Test 8: Metadata survives tee — both branches receive all meta --------
echo
echo "[Test 8] Tee: both branches receive all metadata"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! tee name=t \
    t. ! queue ! imureader name=imu1 ! cropreader name=crop1 ! fovreader name=fov1 ! fakesink \
    t. ! queue ! imureader name=imu2 ! cropreader name=crop2 ! fovreader name=fov2 ! fakesink 2>&1)

# Each branch must see all three types — count occurrences (2 branches × 3 buffers = 6 per type)
IMU_COUNT=$(echo "$OUTPUT" | grep -c '\[imureader\].*accel=' || true)
CROP_COUNT=$(echo "$OUTPUT" | grep -c '\[cropreader\].*top=' || true)
FOV_COUNT=$(echo "$OUTPUT" | grep -c '\[fovreader\].*h=' || true)

[ "$IMU_COUNT" -eq 6 ] && pass "tee: IMU on both branches ($IMU_COUNT/6)" || fail "tee: IMU count $IMU_COUNT != 6"
[ "$CROP_COUNT" -eq 6 ] && pass "tee: crop on both branches ($CROP_COUNT/6)" || fail "tee: crop count $CROP_COUNT != 6"
[ "$FOV_COUNT" -eq 6 ] && pass "tee: FOV on both branches ($FOV_COUNT/6)" || fail "tee: FOV count $FOV_COUNT != 6"

# ---- Test 9: Compartmentalization through tee — only IMU, no false reads ---
echo
echo "[Test 9] Tee compartmentalization: only IMU written, crop/fov absent on both branches"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter \
    ! tee name=t \
    t. ! queue ! imureader ! cropreader ! fovreader ! fakesink \
    t. ! queue ! imureader ! cropreader ! fovreader ! fakesink 2>&1)

IMU_COUNT=$(echo "$OUTPUT" | grep -c '\[imureader\].*accel=' || true)
CROP_NO=$(echo "$OUTPUT" | grep -c '\[cropreader\] no crop' || true)
FOV_NO=$(echo "$OUTPUT" | grep -c '\[fovreader\] no FOV' || true)

[ "$IMU_COUNT" -eq 6 ] && pass "tee: IMU present on both branches ($IMU_COUNT/6)" || fail "tee: IMU count $IMU_COUNT != 6"
[ "$CROP_NO" -eq 6 ] && pass "tee: crop absent on both branches ($CROP_NO/6)" || fail "tee: crop absence count $CROP_NO != 6"
[ "$FOV_NO" -eq 6 ] && pass "tee: FOV absent on both branches ($FOV_NO/6)" || fail "tee: FOV absence count $FOV_NO != 6"

# ---- Test 10: output-selector (switch) — metadata through selected pad -----
echo
echo "[Test 10] Output-selector: metadata arrives on selected branch"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! output-selector name=sel pad-negotiation-mode=active \
    sel.src_0 ! queue ! imureader ! cropreader ! fovreader ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "output-selector: IMU present" || fail "output-selector: IMU missing"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "output-selector: crop present" || fail "output-selector: crop missing"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "output-selector: FOV present" || fail "output-selector: FOV missing"

# ---- Test 11: Deep chain — queue + videoconvert + identity + videoscale ----
echo
echo "[Test 11] Deep chain: writers ! queue ! videoconvert ! queue ! identity ! videoscale ! queue ! readers"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=3 ! video/x-raw,format=I420,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! queue ! videoconvert ! queue ! identity ! videoscale ! video/x-raw,width=160,height=120 ! queue \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=' && pass "deep chain: IMU survives" || fail "deep chain: IMU lost"
echo "$OUTPUT" | grep -q '\[cropreader\].*top=' && pass "deep chain: crop survives" || fail "deep chain: crop lost"
echo "$OUTPUT" | grep -q '\[fovreader\].*h=' && pass "deep chain: FOV survives" || fail "deep chain: FOV lost"

# ---- Test 12: Data integrity — verify values are not corrupted -------------
echo
echo "[Test 12] Data integrity: verify metadata values match what was written"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=1 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! queue ! videoconvert ! videoscale ! video/x-raw,width=160,height=120 \
    ! imureader ! cropreader ! fovreader \
    ! fakesink 2>&1)

echo "$OUTPUT" | grep -q '\[imureader\].*accel=(0.00,0.00,-9.81).*temp=25.0' && pass "IMU values intact" || fail "IMU values corrupted"
echo "$OUTPUT" | grep -q '\[cropreader\] top=10 bottom=10 left=20 right=20' && pass "crop values intact" || fail "crop values corrupted"
echo "$OUTPUT" | grep -q '\[fovreader\] h=82.0.*v=52.0.*d=97.0' && pass "FOV values intact" || fail "FOV values corrupted"

# ---- Test 13: Data integrity after tee — values identical on both branches -
echo
echo "[Test 13] Data integrity after tee: values identical on both branches"

OUTPUT=$(gst-launch-1.0 \
    videotestsrc num-buffers=1 ! video/x-raw,width=320,height=240 \
    ! imuwriter ! cropwriter ! fovwriter \
    ! tee name=t \
    t. ! queue ! imureader ! cropreader ! fovreader ! fakesink \
    t. ! queue ! imureader ! cropreader ! fovreader ! fakesink 2>&1)

IMU_INTACT=$(echo "$OUTPUT" | grep -c '\[imureader\].*accel=(0.00,0.00,-9.81).*temp=25.0' || true)
CROP_INTACT=$(echo "$OUTPUT" | grep -c '\[cropreader\] top=10 bottom=10 left=20 right=20' || true)
FOV_INTACT=$(echo "$OUTPUT" | grep -c '\[fovreader\] h=82.0' || true)

[ "$IMU_INTACT" -eq 2 ] && pass "tee: IMU values identical on both branches" || fail "tee: IMU diverged ($IMU_INTACT/2)"
[ "$CROP_INTACT" -eq 2 ] && pass "tee: crop values identical on both branches" || fail "tee: crop diverged ($CROP_INTACT/2)"
[ "$FOV_INTACT" -eq 2 ] && pass "tee: FOV values identical on both branches" || fail "tee: FOV diverged ($FOV_INTACT/2)"

# ---- Summary ---------------------------------------------------------------
echo
echo "=== Results: $PASS passed, $FAIL failed ==="

if [ "$FAIL" -gt 0 ]; then
    exit 1
fi

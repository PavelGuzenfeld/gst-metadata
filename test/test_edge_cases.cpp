#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gst-metadata-examples/meta_crop.hpp>
#include <gst-metadata-examples/meta_fov.hpp>
#include <gst-metadata-examples/meta_imu.hpp>

using namespace gstmeta::examples;

namespace {

struct GstInitGuard {
    GstInitGuard() { gst_init(nullptr, nullptr); }
};

static GstInitGuard gst_guard; // NOLINT

GstBuffer* make_buffer()
{
    return gst_buffer_new_allocate(nullptr, 64, nullptr);
}

} // namespace

// ---------------------------------------------------------------------------
// Null buffer handling
// ---------------------------------------------------------------------------

TEST_CASE("null buffer: add returns nullptr")
{
    ImuData imu{};
    CHECK(ImuMeta::add(nullptr, imu) == nullptr);
    CHECK(CropMeta::add(nullptr, CropData{}) == nullptr);
    CHECK(FovMeta::add(nullptr, FovData{}) == nullptr);
}

TEST_CASE("null buffer: get returns nullopt")
{
    CHECK_FALSE(ImuMeta::get(nullptr).has_value());
    CHECK_FALSE(CropMeta::get(nullptr).has_value());
    CHECK_FALSE(FovMeta::get(nullptr).has_value());
}

TEST_CASE("null buffer: get_mut returns nullptr")
{
    CHECK(ImuMeta::get_mut(nullptr) == nullptr);
    CHECK(CropMeta::get_mut(nullptr) == nullptr);
    CHECK(FovMeta::get_mut(nullptr) == nullptr);
}

TEST_CASE("null buffer: remove returns false")
{
    CHECK_FALSE(ImuMeta::remove(nullptr));
    CHECK_FALSE(CropMeta::remove(nullptr));
    CHECK_FALSE(FovMeta::remove(nullptr));
}

TEST_CASE("null buffer: count returns 0")
{
    // count calls for_each which has g_return_if_fail; the lambda never fires
    // so count should return 0
    CHECK(ImuMeta::count(nullptr) == 0);
}

TEST_CASE("null buffer: for_each is a no-op")
{
    int calls = 0;
    ImuMeta::for_each(nullptr, [&](const ImuData&) { ++calls; });
    CHECK(calls == 0);
}

TEST_CASE("null buffer: get_all returns empty vector")
{
    auto v = ImuMeta::get_all(nullptr);
    CHECK(v.empty());
}

// ---------------------------------------------------------------------------
// Empty / tiny buffers
// ---------------------------------------------------------------------------

TEST_CASE("empty buffer (0 bytes) can hold metadata")
{
    GstBuffer* buf = gst_buffer_new_allocate(nullptr, 0, nullptr);
    REQUIRE(buf != nullptr);

    ImuData imu{.timestamp_ns = 7};
    auto* s = ImuMeta::add(buf, imu);
    CHECK(s != nullptr);

    auto got = ImuMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->timestamp_ns == 7);

    gst_buffer_unref(buf);
}

TEST_CASE("tiny buffer (1 byte) can hold metadata")
{
    GstBuffer* buf = gst_buffer_new_allocate(nullptr, 1, nullptr);
    REQUIRE(buf != nullptr);

    CropData crop{.top = 42, .bottom = 43, .left = 44, .right = 45};
    auto* s = CropMeta::add(buf, crop);
    CHECK(s != nullptr);

    auto got = CropMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->top == 42);
    CHECK(got->right == 45);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Read-only buffer
// ---------------------------------------------------------------------------

TEST_CASE("add metadata to a read-only buffer fails gracefully")
{
    // Create a buffer wrapping read-only memory.
    static const guint8 readonly_data[] = {0x00, 0x01, 0x02, 0x03};
    GstBuffer* buf = gst_buffer_new_wrapped_full(
        static_cast<GstMemoryFlags>(GST_MEMORY_FLAG_READONLY),
        const_cast<guint8*>(readonly_data),
        sizeof(readonly_data), // maxsize
        0,                     // offset
        sizeof(readonly_data), // size
        nullptr,               // user_data
        nullptr);              // notify
    REQUIRE(buf != nullptr);

    // Metadata is stored in the GstMeta list, not in the buffer memory itself,
    // so adding metadata to a buffer with read-only memory should still work.
    ImuData imu{.temperature = 1.0F};
    auto* s = ImuMeta::add(buf, imu);
    CHECK(s != nullptr);

    auto got = ImuMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->temperature == doctest::Approx(1.0F));

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Large number of metadata instances
// ---------------------------------------------------------------------------

TEST_CASE("add 1000 metadata instances of the same type")
{
    GstBuffer* buf = make_buffer();

    for (int i = 0; i < 1000; ++i) {
        CropData crop{.top = i, .bottom = i + 1, .left = i + 2, .right = i + 3};
        auto* s = CropMeta::add(buf, crop);
        REQUIRE(s != nullptr);
    }

    CHECK(CropMeta::count(buf) == 1000);

    auto all = CropMeta::get_all(buf);
    REQUIRE(all.size() == 1000);

    // Verify first and last
    CHECK(all[0].top == 0);
    CHECK(all[0].right == 3);
    CHECK(all[999].top == 999);
    CHECK(all[999].right == 1002);

    // Verify ordering is preserved
    for (int i = 0; i < 1000; ++i) {
        CHECK(all[static_cast<std::size_t>(i)].top == i);
    }

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Add / remove / re-add cycle
// ---------------------------------------------------------------------------

TEST_CASE("add, remove, re-add cycle")
{
    GstBuffer* buf = make_buffer();

    ImuData imu1{.timestamp_ns = 100};
    ImuMeta::add(buf, imu1);
    REQUIRE(ImuMeta::get(buf).has_value());
    CHECK(ImuMeta::get(buf)->timestamp_ns == 100);

    CHECK(ImuMeta::remove(buf));
    CHECK_FALSE(ImuMeta::get(buf).has_value());

    ImuData imu2{.timestamp_ns = 200};
    ImuMeta::add(buf, imu2);
    REQUIRE(ImuMeta::get(buf).has_value());
    CHECK(ImuMeta::get(buf)->timestamp_ns == 200);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Interleaved add/remove of different types
// ---------------------------------------------------------------------------

TEST_CASE("interleaved add/remove of different types")
{
    GstBuffer* buf = make_buffer();

    // Add IMU, add Crop, remove IMU, add FOV, remove Crop
    ImuMeta::add(buf, ImuData{.timestamp_ns = 1});
    CropMeta::add(buf, CropData{.top = 2});
    CHECK(ImuMeta::count(buf) == 1);
    CHECK(CropMeta::count(buf) == 1);

    CHECK(ImuMeta::remove(buf));
    CHECK(ImuMeta::count(buf) == 0);
    CHECK(CropMeta::count(buf) == 1); // Crop still there

    FovMeta::add(buf, FovData{.horizontal_deg = 3.0F});
    CHECK(FovMeta::count(buf) == 1);
    CHECK(CropMeta::count(buf) == 1);

    CHECK(CropMeta::remove(buf));
    CHECK(CropMeta::count(buf) == 0);
    CHECK(FovMeta::count(buf) == 1); // FOV still there

    auto fov = FovMeta::get(buf);
    REQUIRE(fov.has_value());
    CHECK(fov->horizontal_deg == doctest::Approx(3.0F));

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// get_mut modification doesn't affect other types
// ---------------------------------------------------------------------------

TEST_CASE("get_mut modification does not affect other metadata types")
{
    GstBuffer* buf = make_buffer();

    ImuMeta::add(buf, ImuData{.temperature = 10.0F});
    CropMeta::add(buf, CropData{.top = 50});

    // Mutate IMU
    auto* imu_mut = ImuMeta::get_mut(buf);
    REQUIRE(imu_mut != nullptr);
    imu_mut->data.temperature = 99.0F;

    // Verify IMU changed
    auto imu = ImuMeta::get(buf);
    REQUIRE(imu.has_value());
    CHECK(imu->temperature == doctest::Approx(99.0F));

    // Verify Crop is unchanged
    auto crop = CropMeta::get(buf);
    REQUIRE(crop.has_value());
    CHECK(crop->top == 50);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// for_each with no instances
// ---------------------------------------------------------------------------

TEST_CASE("for_each with no instances is a no-op")
{
    GstBuffer* buf = make_buffer();

    int calls = 0;
    ImuMeta::for_each(buf, [&](const ImuData&) { ++calls; });
    CHECK(calls == 0);

    CropMeta::for_each(buf, [&](const CropData&) { ++calls; });
    CHECK(calls == 0);

    FovMeta::for_each(buf, [&](const FovData&) { ++calls; });
    CHECK(calls == 0);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// for_each callback that does nothing
// ---------------------------------------------------------------------------

TEST_CASE("for_each with no-op callback")
{
    GstBuffer* buf = make_buffer();

    ImuMeta::add(buf, ImuData{.timestamp_ns = 1});
    ImuMeta::add(buf, ImuData{.timestamp_ns = 2});

    // Callback intentionally does nothing; should not crash
    ImuMeta::for_each(buf, [](const ImuData&) {});

    // Verify data is untouched
    CHECK(ImuMeta::count(buf) == 2);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Remove one type, verify others intact
// ---------------------------------------------------------------------------

TEST_CASE("buffer with all three types, remove one, verify others intact")
{
    GstBuffer* buf = make_buffer();

    ImuMeta::add(buf, ImuData{.timestamp_ns = 10});
    CropMeta::add(buf, CropData{.top = 20});
    FovMeta::add(buf, FovData{.horizontal_deg = 30.0F});

    // Remove crop
    CHECK(CropMeta::remove(buf));
    CHECK_FALSE(CropMeta::get(buf).has_value());

    // IMU and FOV should be intact
    auto imu = ImuMeta::get(buf);
    REQUIRE(imu.has_value());
    CHECK(imu->timestamp_ns == 10);

    auto fov = FovMeta::get(buf);
    REQUIRE(fov.has_value());
    CHECK(fov->horizontal_deg == doctest::Approx(30.0F));

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Zero-initialized data round-trip
// ---------------------------------------------------------------------------

TEST_CASE("zero-initialized data round-trip")
{
    GstBuffer* buf = make_buffer();

    ImuData imu{}; // all zeros
    ImuMeta::add(buf, imu);

    auto got = ImuMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->timestamp_ns == 0);
    CHECK(got->accel_x == doctest::Approx(0.0F));
    CHECK(got->accel_y == doctest::Approx(0.0F));
    CHECK(got->accel_z == doctest::Approx(0.0F));
    CHECK(got->gyro_x == doctest::Approx(0.0F));
    CHECK(got->gyro_y == doctest::Approx(0.0F));
    CHECK(got->gyro_z == doctest::Approx(0.0F));
    CHECK(got->temperature == doctest::Approx(0.0F));
    CHECK(got->pressure == doctest::Approx(0.0F));
    CHECK(got->humidity == doctest::Approx(0.0F));
    CHECK(got->altitude == doctest::Approx(0.0F));

    CropData crop{};
    CropMeta::add(buf, crop);
    auto got_crop = CropMeta::get(buf);
    REQUIRE(got_crop.has_value());
    CHECK(got_crop->top == 0);
    CHECK(got_crop->bottom == 0);
    CHECK(got_crop->left == 0);
    CHECK(got_crop->right == 0);

    FovData fov{};
    FovMeta::add(buf, fov);
    auto got_fov = FovMeta::get(buf);
    REQUIRE(got_fov.has_value());
    CHECK(got_fov->horizontal_deg == doctest::Approx(0.0F));
    CHECK(got_fov->vertical_deg == doctest::Approx(0.0F));
    CHECK(got_fov->diagonal_deg == doctest::Approx(0.0F));

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Multiple add + remove_all pattern
// ---------------------------------------------------------------------------

TEST_CASE("multiple add then remove all one by one")
{
    GstBuffer* buf = make_buffer();

    // Add 10 of each type
    for (int i = 0; i < 10; ++i) {
        ImuMeta::add(buf, ImuData{.timestamp_ns = static_cast<uint64_t>(i)});
        CropMeta::add(buf, CropData{.top = i});
        FovMeta::add(buf, FovData{.horizontal_deg = static_cast<float>(i)});
    }

    CHECK(ImuMeta::count(buf) == 10);
    CHECK(CropMeta::count(buf) == 10);
    CHECK(FovMeta::count(buf) == 10);

    // Remove all IMU
    while (ImuMeta::remove(buf)) {}
    CHECK(ImuMeta::count(buf) == 0);
    CHECK(CropMeta::count(buf) == 10);
    CHECK(FovMeta::count(buf) == 10);

    // Remove all Crop
    while (CropMeta::remove(buf)) {}
    CHECK(CropMeta::count(buf) == 0);
    CHECK(FovMeta::count(buf) == 10);

    // Remove all FOV
    while (FovMeta::remove(buf)) {}
    CHECK(FovMeta::count(buf) == 0);

    gst_buffer_unref(buf);
}

TEST_CASE("add, remove all, re-add pattern")
{
    GstBuffer* buf = make_buffer();

    for (int i = 0; i < 5; ++i) {
        CropMeta::add(buf, CropData{.top = i});
    }
    CHECK(CropMeta::count(buf) == 5);

    while (CropMeta::remove(buf)) {}
    CHECK(CropMeta::count(buf) == 0);

    // Re-add
    for (int i = 100; i < 105; ++i) {
        CropMeta::add(buf, CropData{.top = i});
    }
    CHECK(CropMeta::count(buf) == 5);

    auto all = CropMeta::get_all(buf);
    REQUIRE(all.size() == 5);
    CHECK(all[0].top == 100);
    CHECK(all[4].top == 104);

    gst_buffer_unref(buf);
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gst-metadata-examples/meta_crop.hpp>
#include <gst-metadata-examples/meta_fov.hpp>
#include <gst-metadata-examples/meta_imu.hpp>

#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

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

// Simple seeded PRNG wrapper for reproducibility
class Rng {
public:
    explicit Rng(std::uint32_t seed = 12345) : gen_(seed) {}

    int next_int(int lo, int hi)
    {
        std::uniform_int_distribution<int> dist(lo, hi);
        return dist(gen_);
    }

    float next_float()
    {
        std::uniform_real_distribution<float> dist(-1e6F, 1e6F);
        return dist(gen_);
    }

    std::uint64_t next_u64()
    {
        std::uniform_int_distribution<std::uint64_t> dist(
            0, std::numeric_limits<std::uint64_t>::max());
        return dist(gen_);
    }

private:
    std::mt19937 gen_;
};

// Operation enum for random sequences
enum class Op { AddImu, AddCrop, AddFov, RemoveImu, RemoveCrop, RemoveFov,
                GetImu, GetCrop, GetFov, Count };

} // namespace

// ---------------------------------------------------------------------------
// Random add/get/remove sequences
// ---------------------------------------------------------------------------

TEST_CASE("random add/get/remove sequences (10000 iterations)")
{
    Rng rng(42);
    GstBuffer* buf = make_buffer();

    std::size_t expected_imu = 0;
    std::size_t expected_crop = 0;
    std::size_t expected_fov = 0;

    for (int i = 0; i < 10000; ++i) {
        int op = rng.next_int(0, 8);

        switch (static_cast<Op>(op)) {
        case Op::AddImu: {
            ImuData d{.timestamp_ns = rng.next_u64(), .accel_x = rng.next_float()};
            auto* s = ImuMeta::add(buf, d);
            REQUIRE(s != nullptr);
            ++expected_imu;
            break;
        }
        case Op::AddCrop: {
            CropData d{.top = rng.next_int(-1000, 1000)};
            auto* s = CropMeta::add(buf, d);
            REQUIRE(s != nullptr);
            ++expected_crop;
            break;
        }
        case Op::AddFov: {
            FovData d{.horizontal_deg = rng.next_float()};
            auto* s = FovMeta::add(buf, d);
            REQUIRE(s != nullptr);
            ++expected_fov;
            break;
        }
        case Op::RemoveImu:
            if (expected_imu > 0 && ImuMeta::remove(buf))
                --expected_imu;
            break;
        case Op::RemoveCrop:
            if (expected_crop > 0 && CropMeta::remove(buf))
                --expected_crop;
            break;
        case Op::RemoveFov:
            if (expected_fov > 0 && FovMeta::remove(buf))
                --expected_fov;
            break;
        case Op::GetImu:
            if (expected_imu > 0) {
                CHECK(ImuMeta::get(buf).has_value());
            }
            break;
        case Op::GetCrop:
            if (expected_crop > 0) {
                CHECK(CropMeta::get(buf).has_value());
            }
            break;
        case Op::GetFov:
            if (expected_fov > 0) {
                CHECK(FovMeta::get(buf).has_value());
            }
            break;
        default:
            break;
        }
    }

    // Final consistency check
    CHECK(ImuMeta::count(buf) == expected_imu);
    CHECK(CropMeta::count(buf) == expected_crop);
    CHECK(FovMeta::count(buf) == expected_fov);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Rapid buffer create/populate/copy/destroy cycles
// ---------------------------------------------------------------------------

TEST_CASE("rapid buffer create/populate/copy/destroy cycles")
{
    for (int i = 0; i < 500; ++i) {
        GstBuffer* buf = make_buffer();

        ImuMeta::add(buf, ImuData{.timestamp_ns = static_cast<uint64_t>(i)});
        CropMeta::add(buf, CropData{.top = i});
        FovMeta::add(buf, FovData{.horizontal_deg = static_cast<float>(i)});

        GstBuffer* copy = gst_buffer_copy(buf);

        // Verify copy
        auto imu = ImuMeta::get(copy);
        REQUIRE(imu.has_value());
        CHECK(imu->timestamp_ns == static_cast<uint64_t>(i));

        auto crop = CropMeta::get(copy);
        REQUIRE(crop.has_value());
        CHECK(crop->top == i);

        gst_buffer_unref(copy);
        gst_buffer_unref(buf);
    }
}

// ---------------------------------------------------------------------------
// Many buffers created and destroyed (leak check via count)
// ---------------------------------------------------------------------------

TEST_CASE("many buffers with metadata created and destroyed")
{
    std::vector<GstBuffer*> buffers;
    buffers.reserve(200);

    // Create 200 buffers, each with metadata
    for (int i = 0; i < 200; ++i) {
        GstBuffer* buf = make_buffer();
        ImuMeta::add(buf, ImuData{.timestamp_ns = static_cast<uint64_t>(i)});
        CropMeta::add(buf, CropData{.top = i});
        buffers.push_back(buf);
    }

    // Verify all are populated
    for (int i = 0; i < 200; ++i) {
        CHECK(ImuMeta::count(buffers[static_cast<std::size_t>(i)]) == 1);
        CHECK(CropMeta::count(buffers[static_cast<std::size_t>(i)]) == 1);
    }

    // Destroy all
    for (auto* buf : buffers) {
        gst_buffer_unref(buf);
    }

    // Create fresh buffer and verify it is clean
    GstBuffer* fresh = make_buffer();
    CHECK(ImuMeta::count(fresh) == 0);
    CHECK(CropMeta::count(fresh) == 0);
    gst_buffer_unref(fresh);
}

// ---------------------------------------------------------------------------
// Add many metadata, copy buffer, modify copy, verify original unchanged
// ---------------------------------------------------------------------------

TEST_CASE("copy buffer, modify copy, original unchanged")
{
    GstBuffer* original = make_buffer();

    for (int i = 0; i < 20; ++i) {
        ImuMeta::add(original, ImuData{.timestamp_ns = static_cast<uint64_t>(i),
                                        .temperature = static_cast<float>(i)});
    }
    CropMeta::add(original, CropData{.top = 777});

    GstBuffer* copy = gst_buffer_copy(original);

    // Modify the copy: mutate first IMU, remove Crop, add FOV
    auto* imu_mut = ImuMeta::get_mut(copy);
    REQUIRE(imu_mut != nullptr);
    imu_mut->data.temperature = -999.0F;

    CropMeta::remove(copy);
    FovMeta::add(copy, FovData{.diagonal_deg = 42.0F});

    // Verify original is unchanged
    CHECK(ImuMeta::count(original) == 20);
    auto orig_imu = ImuMeta::get(original);
    REQUIRE(orig_imu.has_value());
    CHECK(orig_imu->temperature == doctest::Approx(0.0F)); // first one had i=0

    auto orig_crop = CropMeta::get(original);
    REQUIRE(orig_crop.has_value());
    CHECK(orig_crop->top == 777);

    CHECK_FALSE(FovMeta::get(original).has_value()); // no FOV on original

    // Verify copy has modifications
    auto copy_imu = ImuMeta::get(copy);
    REQUIRE(copy_imu.has_value());
    CHECK(copy_imu->temperature == doctest::Approx(-999.0F));

    CHECK_FALSE(CropMeta::get(copy).has_value());
    CHECK(FovMeta::get(copy).has_value());

    gst_buffer_unref(copy);
    gst_buffer_unref(original);
}

// ---------------------------------------------------------------------------
// Burst: add 100 of each type, remove all of one, verify others
// ---------------------------------------------------------------------------

TEST_CASE("burst add 100 of each type, remove all of one type")
{
    GstBuffer* buf = make_buffer();

    for (int i = 0; i < 100; ++i) {
        ImuMeta::add(buf, ImuData{.timestamp_ns = static_cast<uint64_t>(i)});
        CropMeta::add(buf, CropData{.top = i});
        FovMeta::add(buf, FovData{.horizontal_deg = static_cast<float>(i)});
    }

    CHECK(ImuMeta::count(buf) == 100);
    CHECK(CropMeta::count(buf) == 100);
    CHECK(FovMeta::count(buf) == 100);

    // Remove all Crop
    while (CropMeta::remove(buf)) {}

    CHECK(CropMeta::count(buf) == 0);
    CHECK(ImuMeta::count(buf) == 100);
    CHECK(FovMeta::count(buf) == 100);

    // Verify IMU data integrity
    auto all_imu = ImuMeta::get_all(buf);
    REQUIRE(all_imu.size() == 100);
    for (int i = 0; i < 100; ++i) {
        CHECK(all_imu[static_cast<std::size_t>(i)].timestamp_ns ==
              static_cast<uint64_t>(i));
    }

    // Verify FOV data integrity
    auto all_fov = FovMeta::get_all(buf);
    REQUIRE(all_fov.size() == 100);
    for (int i = 0; i < 100; ++i) {
        CHECK(all_fov[static_cast<std::size_t>(i)].horizontal_deg ==
              doctest::Approx(static_cast<float>(i)));
    }

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Random data values including edge floats
// ---------------------------------------------------------------------------

TEST_CASE("edge float values: zero, negative zero, large, small, NaN, infinity")
{
    GstBuffer* buf = make_buffer();

    const float edge_values[] = {
        0.0F,
        -0.0F,
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::min(),            // smallest positive normal
        std::numeric_limits<float>::denorm_min(),     // smallest positive subnormal
        std::numeric_limits<float>::lowest(),         // most negative
        std::numeric_limits<float>::epsilon(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),
    };

    for (float val : edge_values) {
        FovData fov{.horizontal_deg = val, .vertical_deg = val, .diagonal_deg = val};
        auto* s = FovMeta::add(buf, fov);
        REQUIRE(s != nullptr);
    }

    auto all = FovMeta::get_all(buf);
    REQUIRE(all.size() == 10);

    // Check specific edge values round-trip correctly
    // 0.0
    CHECK(all[0].horizontal_deg == 0.0F);
    // -0.0 (compare bitwise: -0.0 == 0.0 in floating point)
    CHECK(all[1].horizontal_deg == 0.0F);
    CHECK(std::signbit(all[1].horizontal_deg)); // should still be negative zero
    // max
    CHECK(all[2].horizontal_deg == std::numeric_limits<float>::max());
    // min
    CHECK(all[3].horizontal_deg == std::numeric_limits<float>::min());
    // denorm_min
    CHECK(all[4].horizontal_deg == std::numeric_limits<float>::denorm_min());
    // lowest
    CHECK(all[5].horizontal_deg == std::numeric_limits<float>::lowest());
    // epsilon
    CHECK(all[6].horizontal_deg == std::numeric_limits<float>::epsilon());
    // infinity
    CHECK(std::isinf(all[7].horizontal_deg));
    CHECK(all[7].horizontal_deg > 0.0F);
    // negative infinity
    CHECK(std::isinf(all[8].horizontal_deg));
    CHECK(all[8].horizontal_deg < 0.0F);
    // NaN
    CHECK(std::isnan(all[9].horizontal_deg));

    gst_buffer_unref(buf);
}

TEST_CASE("edge integer values for CropData")
{
    GstBuffer* buf = make_buffer();

    CropData extremes[] = {
        {.top = std::numeric_limits<std::int32_t>::max(),
         .bottom = std::numeric_limits<std::int32_t>::min(),
         .left = 0,
         .right = -1},
        {.top = 1, .bottom = -1, .left = std::numeric_limits<std::int32_t>::max(),
         .right = std::numeric_limits<std::int32_t>::min()},
    };

    for (const auto& c : extremes) {
        CropMeta::add(buf, c);
    }

    auto all = CropMeta::get_all(buf);
    REQUIRE(all.size() == 2);
    CHECK(all[0].top == std::numeric_limits<std::int32_t>::max());
    CHECK(all[0].bottom == std::numeric_limits<std::int32_t>::min());
    CHECK(all[1].left == std::numeric_limits<std::int32_t>::max());
    CHECK(all[1].right == std::numeric_limits<std::int32_t>::min());

    gst_buffer_unref(buf);
}

TEST_CASE("edge uint64 values for ImuData timestamp")
{
    GstBuffer* buf = make_buffer();

    ImuMeta::add(buf, ImuData{.timestamp_ns = 0});
    ImuMeta::add(buf, ImuData{.timestamp_ns = 1});
    ImuMeta::add(buf, ImuData{.timestamp_ns = std::numeric_limits<uint64_t>::max()});
    ImuMeta::add(buf, ImuData{.timestamp_ns = std::numeric_limits<uint64_t>::max() - 1});

    auto all = ImuMeta::get_all(buf);
    REQUIRE(all.size() == 4);
    CHECK(all[0].timestamp_ns == 0);
    CHECK(all[1].timestamp_ns == 1);
    CHECK(all[2].timestamp_ns == std::numeric_limits<uint64_t>::max());
    CHECK(all[3].timestamp_ns == std::numeric_limits<uint64_t>::max() - 1);

    gst_buffer_unref(buf);
}

// ---------------------------------------------------------------------------
// Buffer copy with many metadata instances
// ---------------------------------------------------------------------------

TEST_CASE("buffer copy with many metadata instances verifies all copied")
{
    GstBuffer* src = make_buffer();

    constexpr int N = 50;
    for (int i = 0; i < N; ++i) {
        ImuMeta::add(src, ImuData{.timestamp_ns = static_cast<uint64_t>(i),
                                   .accel_x = static_cast<float>(i) * 0.1F});
        CropMeta::add(src, CropData{.top = i * 10, .bottom = i * 20});
    }

    GstBuffer* dst = gst_buffer_copy(src);

    // Verify counts
    CHECK(ImuMeta::count(dst) == N);
    CHECK(CropMeta::count(dst) == N);

    // Verify all values
    auto all_imu = ImuMeta::get_all(dst);
    REQUIRE(all_imu.size() == N);
    for (int i = 0; i < N; ++i) {
        CHECK(all_imu[static_cast<std::size_t>(i)].timestamp_ns ==
              static_cast<uint64_t>(i));
        CHECK(all_imu[static_cast<std::size_t>(i)].accel_x ==
              doctest::Approx(static_cast<float>(i) * 0.1F));
    }

    auto all_crop = CropMeta::get_all(dst);
    REQUIRE(all_crop.size() == N);
    for (int i = 0; i < N; ++i) {
        CHECK(all_crop[static_cast<std::size_t>(i)].top == i * 10);
        CHECK(all_crop[static_cast<std::size_t>(i)].bottom == i * 20);
    }

    gst_buffer_unref(dst);
    gst_buffer_unref(src);
}

// ---------------------------------------------------------------------------
// Random type selection with random operations (stress GType registration)
// ---------------------------------------------------------------------------

TEST_CASE("random type selection with random operations")
{
    Rng rng(99999);

    for (int round = 0; round < 100; ++round) {
        GstBuffer* buf = make_buffer();

        int num_ops = rng.next_int(10, 50);
        std::size_t counts[3] = {0, 0, 0}; // IMU, Crop, FOV

        for (int op = 0; op < num_ops; ++op) {
            int type_idx = rng.next_int(0, 2);
            bool do_add = rng.next_int(0, 1) == 0;

            if (do_add) {
                switch (type_idx) {
                case 0:
                    ImuMeta::add(buf, ImuData{.accel_x = rng.next_float()});
                    ++counts[0];
                    break;
                case 1:
                    CropMeta::add(buf, CropData{.top = rng.next_int(-1000, 1000)});
                    ++counts[1];
                    break;
                case 2:
                    FovMeta::add(buf, FovData{.vertical_deg = rng.next_float()});
                    ++counts[2];
                    break;
                }
            } else {
                switch (type_idx) {
                case 0:
                    if (counts[0] > 0 && ImuMeta::remove(buf)) --counts[0];
                    break;
                case 1:
                    if (counts[1] > 0 && CropMeta::remove(buf)) --counts[1];
                    break;
                case 2:
                    if (counts[2] > 0 && FovMeta::remove(buf)) --counts[2];
                    break;
                }
            }
        }

        CHECK(ImuMeta::count(buf) == counts[0]);
        CHECK(CropMeta::count(buf) == counts[1]);
        CHECK(FovMeta::count(buf) == counts[2]);

        gst_buffer_unref(buf);
    }
}

// ---------------------------------------------------------------------------
// Stress: random data values through add/get round-trip
// ---------------------------------------------------------------------------

TEST_CASE("random ImuData values round-trip correctly")
{
    Rng rng(77777);
    GstBuffer* buf = make_buffer();

    constexpr int N = 200;
    std::vector<ImuData> expected;
    expected.reserve(N);

    for (int i = 0; i < N; ++i) {
        ImuData d{
            .timestamp_ns = rng.next_u64(),
            .accel_x = rng.next_float(),
            .accel_y = rng.next_float(),
            .accel_z = rng.next_float(),
            .gyro_x = rng.next_float(),
            .gyro_y = rng.next_float(),
            .gyro_z = rng.next_float(),
            .temperature = rng.next_float(),
            .pressure = rng.next_float(),
            .humidity = rng.next_float(),
            .altitude = rng.next_float(),
        };
        ImuMeta::add(buf, d);
        expected.push_back(d);
    }

    auto all = ImuMeta::get_all(buf);
    REQUIRE(all.size() == N);

    for (int i = 0; i < N; ++i) {
        auto idx = static_cast<std::size_t>(i);
        CHECK(all[idx].timestamp_ns == expected[idx].timestamp_ns);
        CHECK(all[idx].accel_x == doctest::Approx(expected[idx].accel_x));
        CHECK(all[idx].accel_y == doctest::Approx(expected[idx].accel_y));
        CHECK(all[idx].accel_z == doctest::Approx(expected[idx].accel_z));
        CHECK(all[idx].gyro_x == doctest::Approx(expected[idx].gyro_x));
        CHECK(all[idx].gyro_y == doctest::Approx(expected[idx].gyro_y));
        CHECK(all[idx].gyro_z == doctest::Approx(expected[idx].gyro_z));
        CHECK(all[idx].temperature == doctest::Approx(expected[idx].temperature));
        CHECK(all[idx].pressure == doctest::Approx(expected[idx].pressure));
        CHECK(all[idx].humidity == doctest::Approx(expected[idx].humidity));
        CHECK(all[idx].altitude == doctest::Approx(expected[idx].altitude));
    }

    gst_buffer_unref(buf);
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

// Example metadata types (NOT part of the library — used for integration testing)
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

TEST_CASE("independent metadata types on same buffer")
{
    GstBuffer* buf = make_buffer();

    ImuData imu{.timestamp_ns = 42, .accel_z = -9.81F, .temperature = 25.0F};
    CropData crop{.top = 10, .bottom = 10, .left = 20, .right = 20};
    FovData fov{.horizontal_deg = 82.0F, .vertical_deg = 52.0F};

    SUBCASE("add and retrieve each type independently")
    {
        CHECK(ImuMeta::add(buf, imu) != nullptr);
        CHECK(CropMeta::add(buf, crop) != nullptr);
        CHECK(FovMeta::add(buf, fov) != nullptr);

        auto got_imu = ImuMeta::get(buf);
        REQUIRE(got_imu.has_value());
        CHECK(got_imu->timestamp_ns == 42);
        CHECK(got_imu->accel_z == doctest::Approx(-9.81F));

        auto got_crop = CropMeta::get(buf);
        REQUIRE(got_crop.has_value());
        CHECK(got_crop->top == 10);
        CHECK(got_crop->right == 20);

        auto got_fov = FovMeta::get(buf);
        REQUIRE(got_fov.has_value());
        CHECK(got_fov->horizontal_deg == doctest::Approx(82.0F));
    }

    SUBCASE("missing metadata returns nullopt")
    {
        CHECK_FALSE(ImuMeta::get(buf).has_value());
        CHECK_FALSE(CropMeta::get(buf).has_value());
        CHECK_FALSE(FovMeta::get(buf).has_value());
    }

    gst_buffer_unref(buf);
}

TEST_CASE("multiple instances of same type")
{
    GstBuffer* buf = make_buffer();

    CropData crop1{.top = 10, .bottom = 10, .left = 0, .right = 0};
    CropData crop2{.top = 0, .bottom = 0, .left = 20, .right = 20};

    CropMeta::add(buf, crop1);
    CropMeta::add(buf, crop2);

    CHECK(CropMeta::count(buf) == 2);

    auto all = CropMeta::get_all(buf);
    REQUIRE(all.size() == 2);
    CHECK(all[0].top == 10);
    CHECK(all[1].left == 20);

    gst_buffer_unref(buf);
}

TEST_CASE("remove metadata")
{
    GstBuffer* buf = make_buffer();

    ImuData imu{.temperature = 30.0F};
    ImuMeta::add(buf, imu);
    REQUIRE(ImuMeta::get(buf).has_value());

    CHECK(ImuMeta::remove(buf));
    CHECK_FALSE(ImuMeta::get(buf).has_value());
    CHECK_FALSE(ImuMeta::remove(buf)); // already gone

    gst_buffer_unref(buf);
}

TEST_CASE("mutate metadata in place")
{
    GstBuffer* buf = make_buffer();

    ImuData imu{.temperature = 20.0F};
    ImuMeta::add(buf, imu);

    auto* s = ImuMeta::get_mut(buf);
    REQUIRE(s != nullptr);
    s->data.temperature = 99.0F;

    auto got = ImuMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->temperature == doctest::Approx(99.0F));

    gst_buffer_unref(buf);
}

TEST_CASE("version header is set correctly")
{
    GstBuffer* buf = make_buffer();

    ImuData imu{};
    auto* s = ImuMeta::add(buf, imu);
    REQUIRE(s != nullptr);
    CHECK(s->header.version == 1);
    CHECK(s->header.data_size == sizeof(ImuData));

    gst_buffer_unref(buf);
}

TEST_CASE("metadata types have distinct GTypes")
{
    GType imu_type = ImuMeta::api_type();
    GType crop_type = CropMeta::api_type();
    GType fov_type = FovMeta::api_type();

    CHECK(imu_type != crop_type);
    CHECK(imu_type != fov_type);
    CHECK(crop_type != fov_type);
}

TEST_CASE("metadata survives buffer copy")
{
    GstBuffer* src = make_buffer();

    ImuData imu{.timestamp_ns = 123, .temperature = 36.6F};
    CropData crop{.top = 5};
    ImuMeta::add(src, imu);
    CropMeta::add(src, crop);

    GstBuffer* dst = gst_buffer_copy(src);

    auto got_imu = ImuMeta::get(dst);
    REQUIRE(got_imu.has_value());
    CHECK(got_imu->timestamp_ns == 123);
    CHECK(got_imu->temperature == doctest::Approx(36.6F));

    auto got_crop = CropMeta::get(dst);
    REQUIRE(got_crop.has_value());
    CHECK(got_crop->top == 5);

    gst_buffer_unref(src);
    gst_buffer_unref(dst);
}

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <gst-metadata/gst_metadata.hpp>

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

    gstmeta::ImuData imu{.timestamp_ns = 42, .accel_z = -9.81F, .temperature = 25.0F};
    gstmeta::CropData crop{.top = 10, .bottom = 10, .left = 20, .right = 20};
    gstmeta::FovData fov{.horizontal_deg = 82.0F, .vertical_deg = 52.0F};

    SUBCASE("add and retrieve each type independently")
    {
        CHECK(gstmeta::ImuMeta::add(buf, imu) != nullptr);
        CHECK(gstmeta::CropMeta::add(buf, crop) != nullptr);
        CHECK(gstmeta::FovMeta::add(buf, fov) != nullptr);

        auto got_imu = gstmeta::ImuMeta::get(buf);
        REQUIRE(got_imu.has_value());
        CHECK(got_imu->timestamp_ns == 42);
        CHECK(got_imu->accel_z == doctest::Approx(-9.81F));

        auto got_crop = gstmeta::CropMeta::get(buf);
        REQUIRE(got_crop.has_value());
        CHECK(got_crop->top == 10);
        CHECK(got_crop->right == 20);

        auto got_fov = gstmeta::FovMeta::get(buf);
        REQUIRE(got_fov.has_value());
        CHECK(got_fov->horizontal_deg == doctest::Approx(82.0F));
    }

    SUBCASE("missing metadata returns nullopt")
    {
        CHECK_FALSE(gstmeta::ImuMeta::get(buf).has_value());
        CHECK_FALSE(gstmeta::CropMeta::get(buf).has_value());
        CHECK_FALSE(gstmeta::FovMeta::get(buf).has_value());
    }

    gst_buffer_unref(buf);
}

TEST_CASE("multiple instances of same type")
{
    GstBuffer* buf = make_buffer();

    gstmeta::CropData crop1{.top = 10, .bottom = 10, .left = 0, .right = 0};
    gstmeta::CropData crop2{.top = 0, .bottom = 0, .left = 20, .right = 20};

    gstmeta::CropMeta::add(buf, crop1);
    gstmeta::CropMeta::add(buf, crop2);

    CHECK(gstmeta::CropMeta::count(buf) == 2);

    auto all = gstmeta::CropMeta::get_all(buf);
    REQUIRE(all.size() == 2);
    CHECK(all[0].top == 10);
    CHECK(all[1].left == 20);

    gst_buffer_unref(buf);
}

TEST_CASE("remove metadata")
{
    GstBuffer* buf = make_buffer();

    gstmeta::ImuData imu{.temperature = 30.0F};
    gstmeta::ImuMeta::add(buf, imu);
    REQUIRE(gstmeta::ImuMeta::get(buf).has_value());

    CHECK(gstmeta::ImuMeta::remove(buf));
    CHECK_FALSE(gstmeta::ImuMeta::get(buf).has_value());
    CHECK_FALSE(gstmeta::ImuMeta::remove(buf)); // already gone

    gst_buffer_unref(buf);
}

TEST_CASE("mutate metadata in place")
{
    GstBuffer* buf = make_buffer();

    gstmeta::ImuData imu{.temperature = 20.0F};
    gstmeta::ImuMeta::add(buf, imu);

    auto* s = gstmeta::ImuMeta::get_mut(buf);
    REQUIRE(s != nullptr);
    s->data.temperature = 99.0F;

    auto got = gstmeta::ImuMeta::get(buf);
    REQUIRE(got.has_value());
    CHECK(got->temperature == doctest::Approx(99.0F));

    gst_buffer_unref(buf);
}

TEST_CASE("version header is set correctly")
{
    GstBuffer* buf = make_buffer();

    gstmeta::ImuData imu{};
    auto* s = gstmeta::ImuMeta::add(buf, imu);
    REQUIRE(s != nullptr);
    CHECK(s->header.version == 1);
    CHECK(s->header.data_size == sizeof(gstmeta::ImuData));

    gst_buffer_unref(buf);
}

TEST_CASE("metadata types have distinct GTypes")
{
    GType imu_type = gstmeta::ImuMeta::api_type();
    GType crop_type = gstmeta::CropMeta::api_type();
    GType fov_type = gstmeta::FovMeta::api_type();

    CHECK(imu_type != crop_type);
    CHECK(imu_type != fov_type);
    CHECK(crop_type != fov_type);
}

TEST_CASE("metadata survives buffer copy")
{
    GstBuffer* src = make_buffer();

    gstmeta::ImuData imu{.timestamp_ns = 123, .temperature = 36.6F};
    gstmeta::CropData crop{.top = 5};
    gstmeta::ImuMeta::add(src, imu);
    gstmeta::CropMeta::add(src, crop);

    GstBuffer* dst = gst_buffer_copy(src);

    auto got_imu = gstmeta::ImuMeta::get(dst);
    REQUIRE(got_imu.has_value());
    CHECK(got_imu->timestamp_ns == 123);
    CHECK(got_imu->temperature == doctest::Approx(36.6F));

    auto got_crop = gstmeta::CropMeta::get(dst);
    REQUIRE(got_crop.has_value());
    CHECK(got_crop->top == 5);

    gst_buffer_unref(src);
    gst_buffer_unref(dst);
}

// Minimal self-contained tests (no framework dependency).
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <utility>

#include "autoaim/ballistics.hpp"
#include "autoaim/aimer.hpp"
#include "autoaim/config.hpp"
#include "autoaim/evaluation.hpp"
#include "autoaim/pose.hpp"
#include "autoaim/tracker.hpp"

using namespace autoaim;

static int failures = 0;
#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            ++failures;                                   \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__); \
            std::printf(__VA_ARGS__);                     \
            std::printf("\n");                            \
        }                                                 \
    } while (0)

static void testBallistics() {
    BallisticConfig c;
    c.use_drag = false;
    Ballistics ideal(c);
    double h, t;
    CHECK(ideal.simulate(0.0, 5.0, h, t), "ideal simulate");
    const double t_exp = 5.0 / c.bullet_speed;
    CHECK(std::abs(t - t_exp) < 1e-9 && std::abs(h + 0.5 * c.gravity * t_exp * t_exp) < 1e-9,
          "ideal closed form h=%f t=%f", h, t);

    c.use_drag = true;
    Ballistics drag(c);
    double hd, td;
    CHECK(drag.simulate(0.0, 5.0, hd, td), "drag simulate");
    CHECK(td > t && hd < h, "drag must slow the bullet: t %f vs %f, h %f vs %f", td, t, hd, h);
    // RK4 with k = 0 must equal the closed form
    c.drag_k = 0.0;
    double h0, t0;
    Ballistics(c).simulate(0.1, 6.0, h0, t0);
    BallisticConfig ci = c;
    ci.use_drag = false;
    double h1, t1;
    Ballistics(ci).simulate(0.1, 6.0, h1, t1);
    CHECK(std::abs(h0 - h1) < 1e-6 && std::abs(t0 - t1) < 1e-6, "rk4 vs closed form %g %g", h0 - h1, t0 - t1);

    for (double s : {2.0, 5.0, 8.0})
        for (double z : {-0.3, 0.0, 0.4}) {
            const auto sol = drag.solve(s, z);
            CHECK(sol.ok, "solve %f %f", s, z);
            double hh, tt;
            drag.simulate(sol.pitch, s, hh, tt);
            CHECK(std::abs(hh - z) < 1e-3, "solve accuracy s=%f z=%f got %f", s, z, hh);
        }
}

static void testPose() {
    CameraConfig cc;
    cc.pitch_deg = 3.0;
    CameraModel cam(cc);
    PoseConfig pc;
    PoseSolver solver(cam, pc);
    std::mt19937 rng(1);
    std::normal_distribution<double> n(0, 0.3);
    double max_pos = 0, max_yaw = 0;
    for (double dist : {2.0, 4.0, 6.0})
        for (double yaw_off : {-40.0, -15.0, 10.0, 35.0}) {
            const cv::Vec3d p(dist, 0.3, -0.15);
            const double yaw = std::atan2(p[1], p[0]) + yaw_off * kPi / 180;
            const auto c3 = armorCorners(p, yaw, 15 * kPi / 180, ArmorType::SMALL);
            ArmorDetection d;
            d.type = ArmorType::SMALL;
            for (int i = 0; i < 4; ++i) {
                const cv::Point2d q = cam.project(c3[i]);
                d.corners[i] = cv::Point2f(float(q.x + n(rng)), float(q.y + n(rng)));
            }
            d.center = (d.corners[0] + d.corners[1] + d.corners[2] + d.corners[3]) * 0.25f;
            ArmorObservation o;
            CHECK(solver.solve(d, o), "pose solve");
            max_pos = std::max(max_pos, cv::norm(o.position - p) / dist);
            max_yaw = std::max(max_yaw, std::abs(wrapAngle(o.yaw - yaw)) * 180 / kPi);
        }
    CHECK(max_pos < 0.03, "relative position error %.3f", max_pos);
    CHECK(max_yaw < 8.0, "yaw error %.1f deg", max_yaw);
    std::printf("pose: max rel. position error %.2f%%, max yaw error %.2f deg (0.3 px noise)\n", 100 * max_pos,
                max_yaw);
}

static void testTracker() {
    CameraConfig cc;
    CameraModel cam(cc);
    TrackerConfig tc;
    RobotTracker tr(tc, cam);
    const double w = 3.0, r = 0.24, fps = 50;
    const cv::Vec3d c(5, 0.2, -0.1);
    for (int n = 0; n < 200; ++n) {
        const double t = n / fps;
        FrameObservations f;
        f.frame = n, f.stamp = t;
        const double los = std::atan2(c[1], c[0]);
        for (int k = 0; k < 4; ++k) {
            const double yaw = 0.2 + w * t + kPi / 2 * k;
            if (std::abs(wrapAngle(yaw - los)) > 1.0) continue;
            ArmorObservation o;
            o.position = {c[0] - r * std::cos(yaw), c[1] - r * std::sin(yaw), c[2]};
            o.distance = cv::norm(o.position);
            o.yaw = wrapAngle(yaw);
            o.det.number = "3";
            o.det.left.length = o.det.right.length = 20;
            o.image_center_dist = 100;
            f.armors.push_back(o);
        }
        tr.update(f);
    }
    const auto& x = tr.x();
    CHECK(tr.state() == TrackState::TRACKING, "tracking state %s", toString(tr.state()));
    CHECK(std::abs(x[RobotTracker::W] - w) < 0.2, "omega %f", x[RobotTracker::W]);
    CHECK(std::hypot(x[RobotTracker::XC] - c[0], x[RobotTracker::YC] - c[1]) < 0.05, "centre %f %f",
          x[RobotTracker::XC], x[RobotTracker::YC]);
    CHECK(std::abs(x[RobotTracker::R1] - r) < 0.03, "radius %f", x[RobotTracker::R1]);
    std::printf("tracker: w %.3f (true %.1f), r1 %.3f r2 %.3f (true %.2f)\n", x[RobotTracker::W], w,
                x[RobotTracker::R1], x[RobotTracker::R2], r);
}

static void testDetectionCacheParsing() {
    const auto path = std::filesystem::temp_directory_path() / "rm_autoaim_test_detections.txt";
    auto write = [&](const char* data) {
        std::ofstream file(path);
        file << data;
    };
    std::vector<FrameDetections> frames;
    write("0 0.000 0\n1 0.040 0\n");
    CHECK(loadDetections(path.string(), frames) && frames.size() == 2,
          "valid detection cache should load");
    FrameDetections precise;
    ArmorDetection armor;
    armor.number = "3";
    armor.confidence = 0.7123456f;
    armor.left.top = {123.456789f, 234.56789f};
    armor.left.bottom = {123.654321f, 246.12345f};
    armor.right.top = {173.456789f, 234.98765f};
    armor.right.bottom = {173.654321f, 246.54321f};
    armor.left.width = armor.right.width = 3.1234567f;
    precise.armors.push_back(armor);
    CHECK(saveDetections(path.string(), {precise}), "precise detection cache should save");
    CHECK(loadDetections(path.string(), frames) && frames.size() == 1 &&
          frames[0].armors.size() == 1 &&
          frames[0].armors[0].left.top.x == armor.left.top.x &&
          frames[0].armors[0].right.bottom.y == armor.right.bottom.y,
          "cache should round-trip light coordinates exactly");
    write("0 0.000 0\n1 0.040 1\n");
    CHECK(!loadDetections(path.string(), frames), "truncated armor must invalidate cache");
    write("0 0.000 0\n2 0.040 0\n");
    CHECK(!loadDetections(path.string(), frames), "non-sequential frame must invalidate cache");
    std::filesystem::remove(path);
}

static void testRateLimitedShots() {
    StrategyResult source;
    source.name = "test";
    source.frames = 25;
    for (const auto& [frame, result] : {std::pair{0, ShotResult::HIT},
                                      std::pair{1, ShotResult::MISS},
                                      std::pair{2, ShotResult::HIT},
                                      std::pair{3, ShotResult::UNKNOWN},
                                      std::pair{4, ShotResult::MISS},
                                      std::pair{6, ShotResult::HIT}}) {
        ShotRecord shot;
        shot.frame = frame;
        shot.result = result;
        source.shots_log.push_back(shot);
    }
    const StrategyResult ten = rateLimitedShots(source, 25, 10);
    CHECK(ten.shots == 3 && ten.verifiable == 2 && ten.hits == 2 &&
          ten.shots_log[0].frame == 0 && ten.shots_log[1].frame == 3 &&
          ten.shots_log[2].frame == 6,
          "10 Hz cap should accept frames 0, 3, 6 and preserve unknown shots");
    const StrategyResult five = rateLimitedShots(source, 25, 5);
    CHECK(five.shots == 2 && five.verifiable == 2 && five.hits == 2,
          "5 Hz cap should accept frames 0 and 6");
    CHECK(rateLimitedShots(source, 25, 25).shots == 6,
          "25 Hz cap should retain every frame's fire decision");
}

static void testAimerResetsSelectionAfterLoss() {
    CameraConfig cc;
    CameraModel cam(cc);
    TrackerConfig tc;
    tc.tracking_thres = 1;
    RobotTracker tr(tc, cam);
    AimerConfig ac;
    ac.side_angle_deg = 0;
    ac.switch_hysteresis_deg = 180;  // expose stale plate indices after reinitialization
    Ballistics ball(BallisticConfig{});
    RobotAimer aimer(ac, ball), fresh(ac, ball);
    auto feed = [&](int first_frame, double yaw) {
        for (int i = 0; i < 5; ++i) {
            FrameObservations frame;
            frame.frame = first_frame + i;
            frame.stamp = frame.frame * 0.04;
            ArmorObservation o;
            o.yaw = yaw;
            o.position = {5.0 - 0.25 * std::cos(yaw), -0.25 * std::sin(yaw), 0.0};
            o.distance = cv::norm(o.position);
            o.det.number = "3";
            o.det.left.length = o.det.right.length = 20;
            frame.armors.push_back(o);
            tr.update(frame);
        }
    };
    feed(0, 0);
    const AimCommand before = aimer.aim(tr, tr.stamp());
    CHECK(before.valid, "first target should produce an aim command");
    tr.reset();
    CHECK(!aimer.aim(tr, tr.stamp()).valid, "lost target should not produce an aim command");
    feed(5, kPi / 2);
    const AimCommand after = aimer.aim(tr, tr.stamp());
    const AimCommand expected = fresh.aim(tr, tr.stamp());
    CHECK(after.valid && expected.valid && after.armor_k == expected.armor_k &&
          after.armor_k != before.armor_k,
          "reacquired target must not reuse an old plate index: %d -> %d, fresh %d",
          before.armor_k, after.armor_k, expected.armor_k);
}

int main() {
    testBallistics();
    testPose();
    testTracker();
    testDetectionCacheParsing();
    testRateLimitedShots();
    testAimerResetsSelectionAfterLoss();
    if (failures == 0) std::printf("ALL TESTS PASSED\n");
    return failures == 0 ? 0 : 1;
}

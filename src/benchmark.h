#pragma once

#include "commonUtils.h"

static const int MAX_BENCH_FRAMES = 8192;
static const int MAX_BENCH_RUNS = 64;
static const int BENCH_SECTION_COUNT = 8;

struct BenchmarkKeyframe {
	Vector3f position;
	Vector3f rotation;  // pitch, yaw, roll in radians
	float durationMs;   // wall-clock time to reach this keyframe from the previous one
};

struct BenchmarkRunSummary {
	int totalFrames;
	float totalTimeMs;
	float avgFrameTimeMs;
	float minFrameTimeMs, maxFrameTimeMs;
	float pct1FrameTimeMs, pct01FrameTimeMs;  // 1% and 0.1% lows (frame time)
};

struct BenchmarkState {
	bool active;
	int currentKeyframe;
	float elapsedMs;    // time accumulated within the current segment
	int totalFrames;
	float minFrameTime;
	float maxFrameTime;
	float totalFrameTime;

	float frameTimes[MAX_BENCH_FRAMES];    // per-frame times (32KB)
	int segmentTransitions[32];             // frame index where each keyframe segment starts
	int segmentCount;                       // number of transitions recorded
};

void benchmarkInit(BenchmarkState& state);

// Advance the benchmark by one frame. Writes the interpolated camera position/rotation
// into outPosition/outRotation. Returns true if the benchmark is still running,
// false if it just finished (results printed internally).
bool benchmarkUpdate(BenchmarkState& state, float frameTimeMs,
	Vector3f& outPosition, Vector3f& outRotation);

// Returns the built-in keyframe count.
int benchmarkGetKeyframeCount();

// Returns the built-in keyframes array.
const BenchmarkKeyframe* benchmarkGetKeyframes();

// Write benchmark results to CSV log file.
void benchmarkWriteLog(const BenchmarkState& state);

// Returns the section name for a given section index (0 to BENCH_SECTION_COUNT-1).
const char* benchmarkGetSectionName(int sectionIndex);

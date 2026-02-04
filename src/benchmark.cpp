#include "benchmark.h"
#include <psp2/kernel/clib.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <cstring>
#include <algorithm>

// Camera starts at (0, 1, 0), rot (0, 0, 0) looking along -Z.
// Cubes fill a sphere of radius ~25 at origin; lights orbit near (0, 2, -7).
//
// Rotation axes (all in RADIANS, fed directly to sinf/cosf):
//   rotation.x = pitch:  positive = look UP,    negative = look DOWN
//   rotation.y = yaw:    positive = turn LEFT,   negative = turn RIGHT
//   rotation.z = roll:   positive = bank RIGHT,  negative = bank LEFT
//
// Cube volume: XY within radius-25 circle, Z in [-25, -8].
// Lights orbit near (0, 2, -7) with radius 6.
// Camera starts at (0, 1, 0) looking along -Z toward the cubes.
//
// The path stays within or just outside the cube sphere so
// cubes remain visible throughout. Two forward passes through
// the cube field, a pitch-up + barrel roll, and overhead passes
// looking down. Post-roll keyframes offset roll by 2*PI (6.28)
// for smooth interpolation; sin/cos are periodic so it's identical.
static BenchmarkKeyframe s_keyframes[] = {
	// ===== START =====
	{ {0.0f,   1.0f,   0.0f},  {0.0f,    0.0f,   0.0f},      0.0f },

	// ===== PASS 1: Fly forward into the cube field =====
	{ {0.0f,   2.0f,  -6.0f},  {0.0f,    0.0f,   0.0f},   2500.0f },
	{ {0.0f,   3.0f, -14.0f},  {0.0f,    0.0f,   0.0f},   2500.0f },
	{ {0.0f,   3.0f, -20.0f},  {0.0f,    0.0f,   0.0f},   2000.0f },

	// ===== RIGHT TURN + PITCH UP (cubes above from Y=6..25) =====
	{ {5.0f,   4.0f, -20.0f},  {0.15f,  -0.15f,  0.06f},  2000.0f },
	{ {10.0f,  5.0f, -18.0f},  {0.4f,   -0.3f,   0.12f},  2500.0f },
	{ {14.0f,  6.0f, -16.0f},  {0.8f,   -0.35f,  0.15f},  3000.0f },

	// ===== BARREL ROLL (corkscrew) among the cubes =====
	{ {16.0f, 10.0f, -15.0f},  {0.5f,   -0.3f,   1.57f},  2500.0f },
	{ {14.0f, 15.0f, -14.0f},  {0.0f,   -0.2f,   3.14f},  2500.0f },
	{ {10.0f, 19.0f, -14.0f},  {-0.5f,  -0.1f,   4.71f},  2500.0f },
	{ {4.0f,  22.0f, -14.0f},  {-1.0f,   0.0f,   6.28f},  2500.0f },

	// ===== OVERHEAD PASS: looking down at the cube field =====
	{ {0.0f,  22.0f, -14.0f},  {-1.2f,   0.0f,   6.28f},  2500.0f },
	{ {-4.0f, 20.0f, -14.0f},  {-1.0f,   0.0f,   6.28f},  2500.0f },

	// ===== DESCEND ahead of cubes (Z > -8 region) =====
	{ {-8.0f, 12.0f,  -8.0f},  {-0.5f,   0.0f,   6.28f},  2500.0f },
	{ {-8.0f,  5.0f,  -4.0f},  {-0.1f,   0.0f,   6.28f},  2500.0f },

	// ===== PASS 2: Fly through cube field again =====
	{ {-5.0f,  4.0f, -10.0f},  {-0.05f,  0.0f,   6.28f},  2000.0f },
	{ {0.0f,   3.0f, -16.0f},  {0.0f,    0.0f,   6.28f},  2000.0f },
	{ {5.0f,   3.0f, -20.0f},  {-0.05f, -0.1f,   6.28f},  2000.0f },

	// ===== CLIMB above cubes, second overhead look-down =====
	{ {5.0f,  10.0f, -22.0f},  {-0.3f,  -0.1f,   6.28f},  2500.0f },
	{ {3.0f,  18.0f, -20.0f},  {-0.8f,   0.0f,   6.28f},  2500.0f },
	{ {0.0f,  18.0f, -14.0f},  {-1.1f,   0.0f,   6.28f},  2500.0f },

	// ===== DESCEND TO START =====
	{ {0.0f,   8.0f,  -6.0f},  {-0.5f,   0.0f,   6.28f},  2500.0f },
	{ {0.0f,   1.0f,   0.0f},  {0.0f,    0.0f,   6.28f},  3000.0f },
};

static const int KEYFRAME_COUNT = sizeof(s_keyframes) / sizeof(s_keyframes[0]);

static const char* s_sectionNames[BENCH_SECTION_COUNT] = {
	"Forward Approach", "Right Turn + Pitch Up", "Barrel Roll",
	"Overhead Pass", "Descend", "Pass 2",
	"Climb + Overhead 2", "Descend to Start"
};

static const int s_sectionForSegment[] = {
	0,0,0, 1,1,1, 2,2,2,2, 3,3, 4,4, 5,5,5, 6,6,6, 7,7
};

static const int SEGMENT_COUNT = sizeof(s_sectionForSegment) / sizeof(s_sectionForSegment[0]);

static Vector3f lerp(const Vector3f& a, const Vector3f& b, float t)
{
	return {
		a.x + (b.x - a.x) * t,
		a.y + (b.y - a.y) * t,
		a.z + (b.z - a.z) * t
	};
}

const char* benchmarkGetSectionName(int sectionIndex)
{
	if (sectionIndex < 0 || sectionIndex >= BENCH_SECTION_COUNT)
		return "Unknown";
	return s_sectionNames[sectionIndex];
}

void benchmarkInit(BenchmarkState& state)
{
	state.active = true;
	state.currentKeyframe = 0;
	state.elapsedMs = 0.0f;
	state.totalFrames = 0;
	state.minFrameTime = 999999.0f;
	state.maxFrameTime = 0.0f;
	state.totalFrameTime = 0.0f;

	memset(state.frameTimes, 0, sizeof(state.frameTimes));
	memset(state.segmentTransitions, 0, sizeof(state.segmentTransitions));
	state.segmentTransitions[0] = 0;
	state.segmentCount = 1;
}

bool benchmarkUpdate(BenchmarkState& state, float frameTimeMs,
	Vector3f& outPosition, Vector3f& outRotation)
{
	// Record per-frame time
	if (state.totalFrames < MAX_BENCH_FRAMES)
		state.frameTimes[state.totalFrames] = frameTimeMs;

	// Record frame time stats
	state.totalFrameTime += frameTimeMs;
	if (frameTimeMs < state.minFrameTime) state.minFrameTime = frameTimeMs;
	if (frameTimeMs > state.maxFrameTime) state.maxFrameTime = frameTimeMs;
	state.totalFrames++;

	state.elapsedMs += frameTimeMs;

	int prevKeyframe = state.currentKeyframe;
	int kf = state.currentKeyframe;

	if (kf < KEYFRAME_COUNT - 1)
	{
		const BenchmarkKeyframe& from = s_keyframes[kf];
		const BenchmarkKeyframe& to = s_keyframes[kf + 1];
		float t = (to.durationMs > 0.0f)
			? state.elapsedMs / to.durationMs
			: 1.0f;

		// Advance to next segment(s) if time exceeded
		while (t >= 1.0f && kf < KEYFRAME_COUNT - 1)
		{
			float overshoot = state.elapsedMs - to.durationMs;
			state.currentKeyframe++;
			state.elapsedMs = overshoot;
			kf = state.currentKeyframe;

			if (kf >= KEYFRAME_COUNT - 1)
				break;

			const BenchmarkKeyframe& nextTo = s_keyframes[kf + 1];
			t = (nextTo.durationMs > 0.0f)
				? state.elapsedMs / nextTo.durationMs
				: 1.0f;
		}

		// Record segment transitions
		if (state.currentKeyframe != prevKeyframe)
		{
			for (int seg = prevKeyframe + 1; seg <= state.currentKeyframe; seg++)
			{
				if (state.segmentCount < 32)
				{
					state.segmentTransitions[state.segmentCount] = state.totalFrames;
					state.segmentCount++;
				}
			}
		}

		if (kf < KEYFRAME_COUNT - 1)
		{
			if (t > 1.0f) t = 1.0f;
			outPosition = lerp(s_keyframes[kf].position, s_keyframes[kf + 1].position, t);
			outRotation = lerp(s_keyframes[kf].rotation, s_keyframes[kf + 1].rotation, t);
			return true;
		}
	}

	// Benchmark finished — snap camera to final keyframe
	outPosition = s_keyframes[KEYFRAME_COUNT - 1].position;
	outRotation = s_keyframes[KEYFRAME_COUNT - 1].rotation;
	state.active = false;

	float avgFrameTime = state.totalFrameTime / (float)state.totalFrames;
	float avgFps = 1000.0f / avgFrameTime;
	float minFps = 1000.0f / state.maxFrameTime;
	float maxFps = 1000.0f / state.minFrameTime;

	sceClibPrintf("=== BENCHMARK RESULTS ===\n");
	sceClibPrintf("Total frames: %d\n", state.totalFrames);
	sceClibPrintf("Total time: %.1f ms\n", state.totalFrameTime);
	sceClibPrintf("Avg frame time: %.2f ms (%.1f FPS)\n", avgFrameTime, avgFps);
	sceClibPrintf("Min frame time: %.2f ms (%.1f FPS)\n", state.minFrameTime, maxFps);
	sceClibPrintf("Max frame time: %.2f ms (%.1f FPS)\n", state.maxFrameTime, minFps);
	sceClibPrintf("=========================\n");

	return false;
}

int benchmarkGetKeyframeCount()
{
	return KEYFRAME_COUNT;
}

const BenchmarkKeyframe* benchmarkGetKeyframes()
{
	return s_keyframes;
}

// --- CSV Logging ---

static float computePercentile(float* sorted, int count, float percentile)
{
	if (count <= 0) return 0.0f;
	int idx = (int)(count * percentile);
	if (idx >= count) idx = count - 1;
	return sorted[idx];
}

static void writeStr(SceUID fd, const char* str)
{
	sceIoWrite(fd, str, strlen(str));
}

static int benchmarkLoadPreviousRuns(BenchmarkRunSummary* out, int max, SceOff* cumOffset)
{
	*cumOffset = -1;
	SceUID fd = sceIoOpen("ux0:/data/nativeRenderBench.csv", SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;

	int runCount = 0;
	static char readBuf[4096];
	static char lineBuf[512];
	int lineLen = 0;
	SceOff filePos = 0;

	int bytesRead;
	while ((bytesRead = sceIoRead(fd, readBuf, sizeof(readBuf))) > 0)
	{
		for (int i = 0; i < bytesRead; i++)
		{
			if (readBuf[i] == '\n' || readBuf[i] == '\r')
			{
				if (lineLen > 0)
				{
					lineBuf[lineLen] = '\0';

					// Check for RUNSUMMARY line
					if (lineLen > 14 && memcmp(lineBuf, "# RUNSUMMARY,", 13) == 0 && runCount < max)
					{
						BenchmarkRunSummary& s = out[runCount];
						// Parse: # RUNSUMMARY,frames,totalMs,avgMs,minMs,maxMs,pct1Ms,pct01Ms
						char* p = lineBuf + 13;
						s.totalFrames = 0;
						s.totalTimeMs = 0; s.avgFrameTimeMs = 0;
						s.minFrameTimeMs = 0; s.maxFrameTimeMs = 0;
						s.pct1FrameTimeMs = 0; s.pct01FrameTimeMs = 0;

						// Simple CSV parse using sceClibStrtof-like approach
						// Use manual parsing since we don't have full sscanf
						int fieldIdx = 0;
						while (*p && fieldIdx < 7)
						{
							// Skip whitespace
							while (*p == ' ') p++;
							char* start = p;
							while (*p && *p != ',') p++;
							char saved = *p;
							*p = '\0';

							float val = 0.0f;
							// Manual float parse
							float sign = 1.0f;
							char* fp = start;
							if (*fp == '-') { sign = -1.0f; fp++; }
							float intPart = 0.0f;
							while (*fp >= '0' && *fp <= '9')
							{
								intPart = intPart * 10.0f + (*fp - '0');
								fp++;
							}
							float fracPart = 0.0f;
							if (*fp == '.')
							{
								fp++;
								float divisor = 10.0f;
								while (*fp >= '0' && *fp <= '9')
								{
									fracPart += (*fp - '0') / divisor;
									divisor *= 10.0f;
									fp++;
								}
							}
							val = sign * (intPart + fracPart);

							switch (fieldIdx)
							{
							case 0: s.totalFrames = (int)val; break;
							case 1: s.totalTimeMs = val; break;
							case 2: s.avgFrameTimeMs = val; break;
							case 3: s.minFrameTimeMs = val; break;
							case 4: s.maxFrameTimeMs = val; break;
							case 5: s.pct1FrameTimeMs = val; break;
							case 6: s.pct01FrameTimeMs = val; break;
							}

							*p = saved;
							if (*p == ',') p++;
							fieldIdx++;
						}
						runCount++;
					}

					// Check for CUMULATIVE line
					if (lineLen >= 15 && memcmp(lineBuf, "# === CUMULATIVE", 16) == 0)
					{
						*cumOffset = filePos + i - lineLen;
						// Adjust for possible \r before \n
						if (*cumOffset > 0 && readBuf[i] == '\n' && i > 0 && readBuf[i-1] == '\r')
						{
							// lineLen doesn't include \r, position is correct
						}
					}

					lineLen = 0;
				}
			}
			else
			{
				if (lineLen < (int)sizeof(lineBuf) - 1)
					lineBuf[lineLen++] = readBuf[i];
			}
		}
		filePos += bytesRead;
	}

	// Handle last line without newline
	if (lineLen > 0)
	{
		lineBuf[lineLen] = '\0';
		if (lineLen >= 15 && memcmp(lineBuf, "# === CUMULATIVE", 16) == 0)
		{
			*cumOffset = filePos - lineLen;
		}
	}

	sceIoClose(fd);
	return runCount;
}

static int sceClibSnprintfInt(char* buf, int bufSize, int value)
{
	if (bufSize <= 0) return 0;
	char tmp[16];
	int len = 0;
	int v = value;
	if (v < 0)
	{
		buf[0] = '-';
		v = -v;
		len = 1;
	}
	if (v == 0)
	{
		tmp[0] = '0';
		int tl = 1;
		for (int i = 0; i < tl && len < bufSize - 1; i++)
			buf[len++] = tmp[i];
		buf[len] = '\0';
		return len;
	}
	int tl = 0;
	while (v > 0 && tl < 15)
	{
		tmp[tl++] = '0' + (v % 10);
		v /= 10;
	}
	// Reverse
	for (int i = tl - 1; i >= 0 && len < bufSize - 1; i--)
		buf[len++] = tmp[i];
	buf[len] = '\0';
	return len;
}

static int formatFloat(char* buf, int bufSize, float value, int decimals)
{
	if (bufSize <= 0) return 0;
	int len = 0;

	if (value < 0.0f)
	{
		if (len < bufSize - 1) buf[len++] = '-';
		value = -value;
	}

	// Compute multiplier for decimal places
	float mult = 1.0f;
	for (int i = 0; i < decimals; i++) mult *= 10.0f;

	unsigned long scaled = (unsigned long)(value * mult + 0.5f);
	unsigned long intPart = scaled;
	for (int i = 0; i < decimals; i++) intPart /= 10;
	unsigned long fracPart = scaled;
	{
		unsigned long ip = intPart;
		for (int i = 0; i < decimals; i++) ip *= 10;
		fracPart = scaled - ip;
	}

	// Write integer part
	char tmp[16];
	int tl = 0;
	if (intPart == 0)
	{
		tmp[tl++] = '0';
	}
	else
	{
		unsigned long v = intPart;
		while (v > 0 && tl < 15)
		{
			tmp[tl++] = '0' + (int)(v % 10);
			v /= 10;
		}
	}
	for (int i = tl - 1; i >= 0 && len < bufSize - 1; i--)
		buf[len++] = tmp[i];

	if (decimals > 0 && len < bufSize - 1)
	{
		buf[len++] = '.';
		// Write frac part with leading zeros
		tl = 0;
		unsigned long fv = fracPart;
		for (int i = 0; i < decimals && tl < 15; i++)
		{
			tmp[tl++] = '0' + (int)(fv % 10);
			fv /= 10;
		}
		for (int i = tl - 1; i >= 0 && len < bufSize - 1; i--)
			buf[len++] = tmp[i];
	}

	buf[len] = '\0';
	return len;
}

void benchmarkWriteLog(const BenchmarkState& state)
{
	static BenchmarkRunSummary prevRuns[MAX_BENCH_RUNS];
	SceOff cumOffset = -1;
	int prevRunCount = benchmarkLoadPreviousRuns(prevRuns, MAX_BENCH_RUNS, &cumOffset);

	int frameCount = state.totalFrames;
	if (frameCount > MAX_BENCH_FRAMES) frameCount = MAX_BENCH_FRAMES;

	// Sort a copy for percentile computation
	static float sortedFrameTimes[MAX_BENCH_FRAMES];
	memcpy(sortedFrameTimes, state.frameTimes, frameCount * sizeof(float));
	std::sort(sortedFrameTimes, sortedFrameTimes + frameCount);

	// Compute overall stats
	BenchmarkRunSummary currentRun;
	currentRun.totalFrames = frameCount;
	currentRun.totalTimeMs = state.totalFrameTime;
	currentRun.avgFrameTimeMs = (frameCount > 0) ? state.totalFrameTime / (float)frameCount : 0.0f;
	currentRun.minFrameTimeMs = state.minFrameTime;
	currentRun.maxFrameTimeMs = state.maxFrameTime;
	currentRun.pct1FrameTimeMs = computePercentile(sortedFrameTimes, frameCount, 0.99f);
	currentRun.pct01FrameTimeMs = computePercentile(sortedFrameTimes, frameCount, 0.999f);

	// Open file
	int runNumber = prevRunCount + 1;
	SceMode perms = 0666;
	SceUID fd;

	if (prevRunCount == 0 || cumOffset < 0)
	{
		if (prevRunCount == 0)
		{
			fd = sceIoOpen("ux0:/data/nativeRenderBench.csv",
				SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, perms);
			if (fd < 0)
			{
				sceClibPrintf("benchmarkWriteLog: failed to create file (%d)\n", fd);
				return;
			}
			writeStr(fd, "# nativeRenderBench Results\n");
		}
		else
		{
			// Previous runs exist but no cumulative marker found, append
			fd = sceIoOpen("ux0:/data/nativeRenderBench.csv",
				SCE_O_WRONLY | SCE_O_APPEND, perms);
			if (fd < 0)
			{
				sceClibPrintf("benchmarkWriteLog: failed to open file for append (%d)\n", fd);
				return;
			}
		}
	}
	else
	{
		// Overwrite from cumulative offset
		fd = sceIoOpen("ux0:/data/nativeRenderBench.csv", SCE_O_WRONLY, perms);
		if (fd < 0)
		{
			sceClibPrintf("benchmarkWriteLog: failed to open file (%d)\n", fd);
			return;
		}
		sceIoLseek(fd, cumOffset, SCE_SEEK_SET);
	}

	char buf[512];
	int len;

	// Write run header
	len = 0;
	memcpy(buf, "# Run ", 6); len = 6;
	len += sceClibSnprintfInt(buf + len, sizeof(buf) - len, runNumber);
	buf[len++] = '\n';
	buf[len] = '\0';
	writeStr(fd, buf);

	writeStr(fd, "Timestamp(ms),FrameTime(ms),FPS,Section\n");

	// Per-frame data
	float timestamp = 0.0f;
	int segIdx = 0; // index into segmentTransitions to determine current keyframe segment
	for (int i = 0; i < frameCount; i++)
	{
		// Determine which keyframe segment this frame belongs to
		while (segIdx < state.segmentCount - 1 && i >= state.segmentTransitions[segIdx + 1])
			segIdx++;

		int sectionIdx = 0;
		if (segIdx < SEGMENT_COUNT)
			sectionIdx = s_sectionForSegment[segIdx];

		float ft = state.frameTimes[i];
		float fps = (ft > 0.0f) ? 1000.0f / ft : 0.0f;

		len = 0;
		len += formatFloat(buf + len, sizeof(buf) - len, timestamp, 2);
		buf[len++] = ',';
		len += formatFloat(buf + len, sizeof(buf) - len, ft, 2);
		buf[len++] = ',';
		len += formatFloat(buf + len, sizeof(buf) - len, fps, 1);
		buf[len++] = ',';
		const char* secName = s_sectionNames[sectionIdx];
		int nameLen = strlen(secName);
		if (len + nameLen < (int)sizeof(buf) - 2)
		{
			memcpy(buf + len, secName, nameLen);
			len += nameLen;
		}
		buf[len++] = '\n';
		buf[len] = '\0';
		writeStr(fd, buf);

		timestamp += ft;
	}

	// Section summary
	writeStr(fd, "# --- Run ");
	len = sceClibSnprintfInt(buf, sizeof(buf), runNumber);
	buf[len] = '\0';
	writeStr(fd, buf);
	writeStr(fd, " Section Summary ---\n");
	writeStr(fd, "# Section, Frames, AvgMS, AvgFPS, MinMS, MaxMS, 1%LowFPS, 0.1%LowFPS\n");

	for (int sec = 0; sec < BENCH_SECTION_COUNT; sec++)
	{
		// Find frame range for this section
		int secStart = -1, secEnd = -1;
		int segStart = -1;
		for (int s = 0; s < SEGMENT_COUNT; s++)
		{
			if (s_sectionForSegment[s] == sec)
			{
				if (segStart < 0) segStart = s;
			}
		}
		if (segStart < 0) continue;

		int segEndSeg = segStart;
		for (int s = segStart; s < SEGMENT_COUNT; s++)
		{
			if (s_sectionForSegment[s] == sec) segEndSeg = s;
			else break;
		}

		// Map segment indices to frame ranges via segmentTransitions
		// segmentTransitions[0] = frame of segment 0 start
		// segmentTransitions[1] = frame of segment 1 start, etc.
		if (segStart < state.segmentCount)
			secStart = state.segmentTransitions[segStart];
		else
			continue;

		if (segEndSeg + 1 < state.segmentCount)
			secEnd = state.segmentTransitions[segEndSeg + 1];
		else
			secEnd = frameCount;

		int secFrames = secEnd - secStart;
		if (secFrames <= 0) continue;

		// Compute section stats
		float secTotal = 0.0f, secMin = 999999.0f, secMax = 0.0f;
		for (int i = secStart; i < secEnd && i < frameCount; i++)
		{
			float ft = state.frameTimes[i];
			secTotal += ft;
			if (ft < secMin) secMin = ft;
			if (ft > secMax) secMax = ft;
		}
		float secAvg = secTotal / (float)secFrames;
		float secAvgFps = 1000.0f / secAvg;

		// Sort section frames for percentiles
		int sortCount = 0;
		for (int i = secStart; i < secEnd && i < frameCount; i++)
			sortedFrameTimes[sortCount++] = state.frameTimes[i];
		std::sort(sortedFrameTimes, sortedFrameTimes + sortCount);

		float sec1pct = computePercentile(sortedFrameTimes, sortCount, 0.99f);
		float sec01pct = computePercentile(sortedFrameTimes, sortCount, 0.999f);
		float sec1pctFps = (sec1pct > 0.0f) ? 1000.0f / sec1pct : 0.0f;
		float sec01pctFps = (sec01pct > 0.0f) ? 1000.0f / sec01pct : 0.0f;

		len = 0;
		memcpy(buf + len, "# ", 2); len += 2;
		int nl = strlen(s_sectionNames[sec]);
		memcpy(buf + len, s_sectionNames[sec], nl); len += nl;
		memcpy(buf + len, ", ", 2); len += 2;
		len += sceClibSnprintfInt(buf + len, sizeof(buf) - len, secFrames);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, secAvg, 1);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, secAvgFps, 1);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, secMin, 1);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, secMax, 1);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, sec1pctFps, 1);
		memcpy(buf + len, ", ", 2); len += 2;
		len += formatFloat(buf + len, sizeof(buf) - len, sec01pctFps, 1);
		buf[len++] = '\n';
		buf[len] = '\0';
		writeStr(fd, buf);
	}

	// Machine-parseable run summary
	len = 0;
	memcpy(buf + len, "# RUNSUMMARY,", 13); len += 13;
	len += sceClibSnprintfInt(buf + len, sizeof(buf) - len, currentRun.totalFrames);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.totalTimeMs, 1);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.avgFrameTimeMs, 2);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.minFrameTimeMs, 2);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.maxFrameTimeMs, 2);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.pct1FrameTimeMs, 2);
	buf[len++] = ',';
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.pct01FrameTimeMs, 2);
	buf[len++] = '\n';
	buf[len] = '\0';
	writeStr(fd, buf);

	// Overall summary
	writeStr(fd, "# --- Run ");
	len = sceClibSnprintfInt(buf, sizeof(buf), runNumber);
	buf[len] = '\0';
	writeStr(fd, buf);
	writeStr(fd, " Overall ---\n");

	len = 0;
	memcpy(buf + len, "# Frames: ", 10); len += 10;
	len += sceClibSnprintfInt(buf + len, sizeof(buf) - len, currentRun.totalFrames);
	memcpy(buf + len, " | Duration: ", 13); len += 13;
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.totalTimeMs, 1);
	memcpy(buf + len, "ms\n", 3); len += 3;
	buf[len] = '\0';
	writeStr(fd, buf);

	len = 0;
	memcpy(buf + len, "# Avg: ", 7); len += 7;
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.avgFrameTimeMs, 2);
	memcpy(buf + len, "ms (", 4); len += 4;
	float avgFps = (currentRun.avgFrameTimeMs > 0.0f) ? 1000.0f / currentRun.avgFrameTimeMs : 0.0f;
	len += formatFloat(buf + len, sizeof(buf) - len, avgFps, 1);
	memcpy(buf + len, " FPS)\n", 6); len += 6;
	buf[len] = '\0';
	writeStr(fd, buf);

	float minFps = (currentRun.maxFrameTimeMs > 0.0f) ? 1000.0f / currentRun.maxFrameTimeMs : 0.0f;
	float maxFps = (currentRun.minFrameTimeMs > 0.0f) ? 1000.0f / currentRun.minFrameTimeMs : 0.0f;
	len = 0;
	memcpy(buf + len, "# Min: ", 7); len += 7;
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.minFrameTimeMs, 1);
	memcpy(buf + len, "ms (", 4); len += 4;
	len += formatFloat(buf + len, sizeof(buf) - len, maxFps, 1);
	memcpy(buf + len, " FPS) | Max: ", 13); len += 13;
	len += formatFloat(buf + len, sizeof(buf) - len, currentRun.maxFrameTimeMs, 1);
	memcpy(buf + len, "ms (", 4); len += 4;
	len += formatFloat(buf + len, sizeof(buf) - len, minFps, 1);
	memcpy(buf + len, " FPS)\n", 6); len += 6;
	buf[len] = '\0';
	writeStr(fd, buf);

	float pct1Fps = (currentRun.pct1FrameTimeMs > 0.0f) ? 1000.0f / currentRun.pct1FrameTimeMs : 0.0f;
	float pct01Fps = (currentRun.pct01FrameTimeMs > 0.0f) ? 1000.0f / currentRun.pct01FrameTimeMs : 0.0f;
	len = 0;
	memcpy(buf + len, "# 1% Low: ", 10); len += 10;
	len += formatFloat(buf + len, sizeof(buf) - len, pct1Fps, 1);
	memcpy(buf + len, " FPS | 0.1% Low: ", 17); len += 17;
	len += formatFloat(buf + len, sizeof(buf) - len, pct01Fps, 1);
	memcpy(buf + len, " FPS\n", 5); len += 5;
	buf[len] = '\0';
	writeStr(fd, buf);

	writeStr(fd, "# === END RUN ");
	len = sceClibSnprintfInt(buf, sizeof(buf), runNumber);
	buf[len] = '\0';
	writeStr(fd, buf);
	writeStr(fd, " ===\n");

	// Cumulative average section
	float cumAvgFrameTime = 0.0f;
	float cumMinFrameTime = currentRun.minFrameTimeMs;
	float cumMaxFrameTime = currentRun.maxFrameTimeMs;
	float cumPct1FrameTime = 0.0f;
	float cumPct01FrameTime = 0.0f;
	int cumTotalFrames = currentRun.totalFrames;

	float weightedAvgSum = currentRun.avgFrameTimeMs * (float)currentRun.totalFrames;
	float weightedPct1Sum = currentRun.pct1FrameTimeMs * (float)currentRun.totalFrames;
	float weightedPct01Sum = currentRun.pct01FrameTimeMs * (float)currentRun.totalFrames;

	for (int r = 0; r < prevRunCount; r++)
	{
		cumTotalFrames += prevRuns[r].totalFrames;
		weightedAvgSum += prevRuns[r].avgFrameTimeMs * (float)prevRuns[r].totalFrames;
		weightedPct1Sum += prevRuns[r].pct1FrameTimeMs * (float)prevRuns[r].totalFrames;
		weightedPct01Sum += prevRuns[r].pct01FrameTimeMs * (float)prevRuns[r].totalFrames;
		if (prevRuns[r].minFrameTimeMs < cumMinFrameTime) cumMinFrameTime = prevRuns[r].minFrameTimeMs;
		if (prevRuns[r].maxFrameTimeMs > cumMaxFrameTime) cumMaxFrameTime = prevRuns[r].maxFrameTimeMs;
	}

	if (cumTotalFrames > 0)
	{
		cumAvgFrameTime = weightedAvgSum / (float)cumTotalFrames;
		cumPct1FrameTime = weightedPct1Sum / (float)cumTotalFrames;
		cumPct01FrameTime = weightedPct01Sum / (float)cumTotalFrames;
	}

	writeStr(fd, "# === CUMULATIVE AVERAGE (");
	len = sceClibSnprintfInt(buf, sizeof(buf), runNumber);
	buf[len] = '\0';
	writeStr(fd, buf);
	if (runNumber == 1)
		writeStr(fd, " run) ===\n");
	else
		writeStr(fd, " runs) ===\n");

	len = 0;
	memcpy(buf + len, "# Avg: ", 7); len += 7;
	len += formatFloat(buf + len, sizeof(buf) - len, cumAvgFrameTime, 2);
	memcpy(buf + len, "ms (", 4); len += 4;
	float cumAvgFps = (cumAvgFrameTime > 0.0f) ? 1000.0f / cumAvgFrameTime : 0.0f;
	len += formatFloat(buf + len, sizeof(buf) - len, cumAvgFps, 1);
	memcpy(buf + len, " FPS)\n", 6); len += 6;
	buf[len] = '\0';
	writeStr(fd, buf);

	len = 0;
	memcpy(buf + len, "# Min: ", 7); len += 7;
	len += formatFloat(buf + len, sizeof(buf) - len, cumMinFrameTime, 1);
	memcpy(buf + len, "ms | Max: ", 10); len += 10;
	len += formatFloat(buf + len, sizeof(buf) - len, cumMaxFrameTime, 1);
	memcpy(buf + len, "ms\n", 3); len += 3;
	buf[len] = '\0';
	writeStr(fd, buf);

	float cumPct1Fps = (cumPct1FrameTime > 0.0f) ? 1000.0f / cumPct1FrameTime : 0.0f;
	float cumPct01Fps = (cumPct01FrameTime > 0.0f) ? 1000.0f / cumPct01FrameTime : 0.0f;
	len = 0;
	memcpy(buf + len, "# 1% Low: ", 10); len += 10;
	len += formatFloat(buf + len, sizeof(buf) - len, cumPct1Fps, 1);
	memcpy(buf + len, " FPS | 0.1% Low: ", 17); len += 17;
	len += formatFloat(buf + len, sizeof(buf) - len, cumPct01Fps, 1);
	memcpy(buf + len, " FPS\n", 5); len += 5;
	buf[len] = '\0';
	writeStr(fd, buf);

	writeStr(fd, "# === END CUMULATIVE ===\n");

	sceIoClose(fd);

	sceClibPrintf("Benchmark log written to ux0:/data/nativeRenderBench.csv (run %d)\n", runNumber);
}

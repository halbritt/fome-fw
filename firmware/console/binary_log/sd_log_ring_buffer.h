#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sd_log_ring_buffer {

// Minimum free space target on the SD/SDNAND volume (64 MiB), capped at 25% of total volume capacity
static constexpr uint64_t SD_MIN_FREE_BYTES = 64ULL * 1024ULL * 1024ULL;
static constexpr int SD_MAX_PRUNE_FILES_PER_PASS = 32;

inline bool isAsciiDigit(char c) {
	return c >= '0' && c <= '9';
}

inline const char* stripLeadingSlashes(const char* path) {
	if (!path) {
		return "";
	}
	while (*path == '/') {
		path++;
	}
	return path;
}

inline bool isSameLogPath(const char* candidatePath, const char* activeLogPath) {
	if (!candidatePath || !activeLogPath || activeLogPath[0] == '\0') {
		return false;
	}
	return std::strcmp(stripLeadingSlashes(candidatePath), stripLeadingSlashes(activeLogPath)) == 0;
}

inline bool isDateFolderName(const char* name, size_t expectedDigits) {
	if (!name || std::strlen(name) != expectedDigits) {
		return false;
	}
	for (size_t i = 0; i < expectedDigits; i++) {
		if (!isAsciiDigit(name[i])) {
			return false;
		}
	}
	if (expectedDigits == 4 && (name[0] != '2' || name[1] != '0')) {
		return false;
	}
	return true;
}

/**
 * Parses a FOME log filename ("fome_<index>.mlg"|".teeth" or "fome_YYMMDD_HHMMSS.mlg"|".teeth")
 * and computes a monotonic 64-bit sort key so the oldest log file always has the smallest key.
 * Returns false for non-log files (such as "index.txt").
 */
inline bool parseFomeLogFileSortKey(const char* pathOrFileName, uint64_t* outSortKey) {
	if (!pathOrFileName || !outSortKey) {
		return false;
	}

	const char* base = std::strrchr(pathOrFileName, '/');
	base = base ? (base + 1) : pathOrFileName;

	static constexpr const char* kPrefix = "fome_";
	static constexpr size_t kPrefixLen = 5;
	if (std::strncmp(base, kPrefix, kPrefixLen) != 0) {
		return false;
	}

	const char* ext = std::strrchr(base, '.');
	if (!ext) {
		return false;
	}
	if (std::strcmp(ext, ".mlg") != 0 && std::strcmp(ext, ".teeth") != 0) {
		return false;
	}

	const char* stem = base + kPrefixLen;
	size_t stemLen = static_cast<size_t>(ext - stem);
	if (stemLen == 0 || stemLen > 16) {
		return false;
	}

	// Case 1: RTC timestamp format "YYMMDD_HHMMSS" (13 chars with '_' at index 6)
	if (stemLen == 13 && stem[6] == '_') {
		uint64_t datePart = 0;
		uint64_t timePart = 0;
		for (size_t i = 0; i < 6; i++) {
			if (!isAsciiDigit(stem[i]) || !isAsciiDigit(stem[7 + i])) {
				return false;
			}
			datePart = datePart * 10 + static_cast<uint64_t>(stem[i] - '0');
			timePart = timePart * 10 + static_cast<uint64_t>(stem[7 + i] - '0');
		}
		*outSortKey = (1ULL << 62) | (datePart * 1000000ULL + timePart);
		return true;
	}

	// Case 2: Sequential numeric index format "<N>" (e.g. "fome_10.mlg" .. "fome_99999.mlg")
	uint64_t seqIndex = 0;
	for (size_t i = 0; i < stemLen; i++) {
		if (!isAsciiDigit(stem[i])) {
			return false;
		}
		seqIndex = seqIndex * 10 + static_cast<uint64_t>(stem[i] - '0');
	}
	*outSortKey = seqIndex;
	return true;
}

inline uint64_t computeTargetFreeBytes(uint64_t totalVolumeBytes) {
	if (totalVolumeBytes == 0) {
		return SD_MIN_FREE_BYTES;
	}
	uint64_t quarterVolume = totalVolumeBytes / 4;
	return (SD_MIN_FREE_BYTES < quarterVolume) ? SD_MIN_FREE_BYTES : quarterVolume;
}

} // namespace sd_log_ring_buffer

#include "pch.h"

#include "sd_log_ring_buffer.h"

using namespace sd_log_ring_buffer;

TEST(SdLogRingBuffer, ParseSequentialLogSortKeys) {
	uint64_t k10 = 0;
	uint64_t k20 = 0;
	uint64_t k100 = 0;
	uint64_t kTeeth = 0;

	EXPECT_TRUE(parseFomeLogFileSortKey("fome_10.mlg", &k10));
	EXPECT_TRUE(parseFomeLogFileSortKey("fome_20.mlg", &k20));
	EXPECT_TRUE(parseFomeLogFileSortKey("fome_100.mlg", &k100));
	EXPECT_TRUE(parseFomeLogFileSortKey("/fome_15.teeth", &kTeeth));

	// Numeric order must hold (10 < 15 < 20 < 100), not ASCII lexicographical ("100" < "20")
	EXPECT_EQ(10ULL, k10);
	EXPECT_EQ(15ULL, kTeeth);
	EXPECT_EQ(20ULL, k20);
	EXPECT_EQ(100ULL, k100);
	EXPECT_LT(k20, k100);
}

TEST(SdLogRingBuffer, ParseTimestampedLogSortKeys) {
	uint64_t kEarlier = 0;
	uint64_t kLaterSameDay = 0;
	uint64_t kNextDay = 0;

	EXPECT_TRUE(parseFomeLogFileSortKey("2026/10/16/fome_261016_091500.mlg", &kEarlier));
	EXPECT_TRUE(parseFomeLogFileSortKey("fome_261016_183045.mlg", &kLaterSameDay));
	EXPECT_TRUE(parseFomeLogFileSortKey("fome_261017_000102.teeth", &kNextDay));

	EXPECT_LT(kEarlier, kLaterSameDay);
	EXPECT_LT(kLaterSameDay, kNextDay);
}

TEST(SdLogRingBuffer, RejectNonLogFiles) {
	uint64_t key = 0;
	EXPECT_FALSE(parseFomeLogFileSortKey("index.txt", &key));
	EXPECT_FALSE(parseFomeLogFileSortKey("System Volume Information", &key));
	EXPECT_FALSE(parseFomeLogFileSortKey("fome_.mlg", &key));
	EXPECT_FALSE(parseFomeLogFileSortKey("fome_abc.mlg", &key));
	EXPECT_FALSE(parseFomeLogFileSortKey("fome_10.txt", &key));
	EXPECT_FALSE(parseFomeLogFileSortKey("other_10.mlg", &key));
}

TEST(SdLogRingBuffer, ActiveLogProtectionAndDateFolders) {
	EXPECT_TRUE(isSameLogPath("/fome_10.mlg", "fome_10.mlg"));
	EXPECT_TRUE(isSameLogPath("2026/10/16/fome_261016_120000.mlg", "/2026/10/16/fome_261016_120000.mlg"));
	EXPECT_FALSE(isSameLogPath("fome_10.mlg", "fome_11.mlg"));
	EXPECT_FALSE(isSameLogPath("fome_10.mlg", nullptr));

	EXPECT_TRUE(isDateFolderName("2026", 4));
	EXPECT_FALSE(isDateFolderName("1999", 4));
	EXPECT_TRUE(isDateFolderName("10", 2));
	EXPECT_FALSE(isDateFolderName("10a", 2));

	// 4 GB volume -> 64 MiB free space target; 128 MB volume -> 32 MiB (25%) free space target
	EXPECT_EQ(SD_MIN_FREE_BYTES, computeTargetFreeBytes(4ULL * 1024 * 1024 * 1024));
	EXPECT_EQ(32ULL * 1024 * 1024, computeTargetFreeBytes(128ULL * 1024 * 1024));
}

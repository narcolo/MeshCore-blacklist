#include <gtest/gtest.h>
#include "helpers/Blacklist.h"

static uint8_t prefix1[] = { 0x12 };
static uint8_t prefix2[] = { 0x12, 0x34 };
static uint8_t prefix3[] = { 0x12, 0x34, 0x56 };

// ── add / remove / matches / clear ──────────────────────────────────────────

TEST(Blacklist, AddThenMatchesSamePrefix) {
  Blacklist bl;
  EXPECT_TRUE(bl.add(prefix2, 2));
  EXPECT_TRUE(bl.matches(prefix2, 2));
}

TEST(Blacklist, MatchesReturnsFalseForUnknownPrefix) {
  Blacklist bl;
  bl.add(prefix2, 2);
  uint8_t other[] = { 0xAB, 0xCD };
  EXPECT_FALSE(bl.matches(other, 2));
}

TEST(Blacklist, MatchesComparesOnlyEntryLengthPrefix) {
  Blacklist bl;
  bl.add(prefix1, 1);  // 1-byte entry: 0x12
  uint8_t hash[] = { 0x12, 0x99, 0x99 };  // last hop hash is longer, but starts with 0x12
  EXPECT_TRUE(bl.matches(hash, 3));
}

TEST(Blacklist, MatchesSkipsEntryLongerThanSuppliedHash) {
  Blacklist bl;
  bl.add(prefix3, 3);  // 3-byte entry
  uint8_t hash[] = { 0x12 };  // only 1 byte available, too short to resolve the entry
  EXPECT_FALSE(bl.matches(hash, 1));
}

TEST(Blacklist, AddIsIdempotentForSamePrefix) {
  Blacklist bl;
  EXPECT_TRUE(bl.add(prefix2, 2));
  EXPECT_TRUE(bl.add(prefix2, 2));
  EXPECT_EQ(1, bl.getCount());
}

TEST(Blacklist, AddFailsWhenFull) {
  Blacklist bl;
  for (int i = 0; i < MAX_BLACKLIST; i++) {
    uint8_t p[1] = { (uint8_t)i };
    EXPECT_TRUE(bl.add(p, 1));
  }
  EXPECT_EQ(MAX_BLACKLIST, bl.getCount());

  uint8_t one_more[1] = { 0xFF };
  EXPECT_FALSE(bl.add(one_more, 1));
  EXPECT_EQ(MAX_BLACKLIST, bl.getCount());
}

TEST(Blacklist, RemoveFindsAndRemovesEntry) {
  Blacklist bl;
  bl.add(prefix2, 2);
  EXPECT_TRUE(bl.remove(prefix2, 2));
  EXPECT_EQ(0, bl.getCount());
  EXPECT_FALSE(bl.matches(prefix2, 2));
}

TEST(Blacklist, RemoveReturnsFalseWhenNotFound) {
  Blacklist bl;
  EXPECT_FALSE(bl.remove(prefix2, 2));
}

TEST(Blacklist, RemoveShiftsRemainingEntriesDown) {
  Blacklist bl;
  uint8_t a[] = { 0x01 }, b[] = { 0x02 }, c[] = { 0x03 };
  bl.add(a, 1);
  bl.add(b, 1);
  bl.add(c, 1);

  EXPECT_TRUE(bl.remove(a, 1));
  EXPECT_EQ(2, bl.getCount());
  EXPECT_TRUE(bl.matches(b, 1));
  EXPECT_TRUE(bl.matches(c, 1));
  EXPECT_FALSE(bl.matches(a, 1));
}

TEST(Blacklist, ClearRemovesAllEntries) {
  Blacklist bl;
  bl.add(prefix1, 1);
  bl.add(prefix2, 2);
  bl.clear();
  EXPECT_EQ(0, bl.getCount());
  EXPECT_FALSE(bl.matches(prefix1, 1));
}

// ── mode ─────────────────────────────────────────────────────────────────────

TEST(Blacklist, DefaultModeIsOff) {
  Blacklist bl;
  EXPECT_EQ(BLACKLIST_MODE_OFF, bl.getMode());
}

TEST(Blacklist, SetModeAcceptsValidValues) {
  Blacklist bl;
  EXPECT_TRUE(bl.setMode(BLACKLIST_MODE_DIRECT));
  EXPECT_EQ(BLACKLIST_MODE_DIRECT, bl.getMode());

  EXPECT_TRUE(bl.setMode(BLACKLIST_MODE_INDIRECT));
  EXPECT_EQ(BLACKLIST_MODE_INDIRECT, bl.getMode());
}

TEST(Blacklist, SetModeRejectsOutOfRangeValue) {
  Blacklist bl;
  EXPECT_FALSE(bl.setMode(BLACKLIST_MODE_INDIRECT + 1));
  EXPECT_EQ(BLACKLIST_MODE_OFF, bl.getMode());  // unchanged
}

// ── handleCommand ────────────────────────────────────────────────────────────

TEST(BlacklistHandleCommand, AddValidHexPrefix) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add 1234", reply);
  EXPECT_STREQ("OK", reply);
  EXPECT_EQ(1, bl.getCount());
}

TEST(BlacklistHandleCommand, AddRejectsBadHexLength) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add 123", reply);  // odd length, not 2/4/6
  EXPECT_STREQ("Err - prefix must be 2/4/6 hex chars (1-3 bytes)", reply);
  EXPECT_EQ(0, bl.getCount());
}

TEST(BlacklistHandleCommand, AddRejectsNonHexChars) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add zz", reply);
  EXPECT_STREQ("Err - prefix must be 2/4/6 hex chars (1-3 bytes)", reply);
}

TEST(BlacklistHandleCommand, AddReportsFullBlacklist) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  char cmd[16];
  for (int i = 0; i < MAX_BLACKLIST; i++) {
    snprintf(cmd, sizeof(cmd), "add %02x", i);
    bl.handleCommand(cmd, reply);
    EXPECT_STREQ("OK", reply);
  }
  bl.handleCommand("add ff", reply);
  EXPECT_STREQ("Err - blacklist full", reply);
}

TEST(BlacklistHandleCommand, RemoveExistingPrefix) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add 1234", reply);
  bl.handleCommand("remove 1234", reply);
  EXPECT_STREQ("OK", reply);
  EXPECT_EQ(0, bl.getCount());
}

TEST(BlacklistHandleCommand, RemoveReportsNotFound) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("remove 1234", reply);
  EXPECT_STREQ("Err - not found", reply);
}

TEST(BlacklistHandleCommand, ListReportsNoneWhenEmpty) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("list", reply);
  EXPECT_STREQ("-none-", reply);
}

TEST(BlacklistHandleCommand, ListReportsEntries) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add 1234", reply);
  bl.handleCommand("add ab", reply);
  bl.handleCommand("list", reply);
  EXPECT_STREQ("2:1234 1:AB", reply);
}

TEST(BlacklistHandleCommand, ClearEmptiesTheList) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("add 1234", reply);
  bl.handleCommand("clear", reply);
  EXPECT_STREQ("OK", reply);
  EXPECT_EQ(0, bl.getCount());
}

TEST(BlacklistHandleCommand, ModeGetDefaultsToOff) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("mode", reply);
  EXPECT_STREQ("off", reply);
}

TEST(BlacklistHandleCommand, ModeSetAndGetRoundTrip) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("mode direct", reply);
  EXPECT_STREQ("OK", reply);

  bl.handleCommand("mode", reply);
  EXPECT_STREQ("direct", reply);

  bl.handleCommand("mode indirect", reply);
  EXPECT_STREQ("OK", reply);
  bl.handleCommand("mode", reply);
  EXPECT_STREQ("indirect", reply);
}

TEST(BlacklistHandleCommand, ModeSetRejectsBadValue) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("mode bogus", reply);
  EXPECT_STREQ("Err - must be: off, direct, or indirect", reply);
}

TEST(BlacklistHandleCommand, UnknownSubcommandIsRejected) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);

  char reply[160];
  bl.handleCommand("bogus", reply);
  EXPECT_STREQ("Err - bad params", reply);
}

// ── persistence (load / save) ───────────────────────────────────────────────

TEST(BlacklistPersistence, LoadOnEmptyFilesystemIsOffAndEmpty) {
  FILESYSTEM fs;
  Blacklist bl;
  bl.load(&fs);
  EXPECT_EQ(BLACKLIST_MODE_OFF, bl.getMode());
  EXPECT_EQ(0, bl.getCount());
}

TEST(BlacklistPersistence, SavedEntriesAndModeSurviveReload) {
  FILESYSTEM fs;
  {
    Blacklist bl;
    bl.load(&fs);
    bl.setMode(BLACKLIST_MODE_INDIRECT);
    bl.add(prefix1, 1);
    bl.add(prefix2, 2);
    bl.add(prefix3, 3);
    bl.save(&fs);
  }

  Blacklist reloaded;
  reloaded.load(&fs);
  EXPECT_EQ(BLACKLIST_MODE_INDIRECT, reloaded.getMode());
  EXPECT_EQ(3, reloaded.getCount());
  EXPECT_TRUE(reloaded.matches(prefix1, 1));
  EXPECT_TRUE(reloaded.matches(prefix2, 2));
  EXPECT_TRUE(reloaded.matches(prefix3, 3));
}

TEST(BlacklistPersistence, ClearDeletesTheFileSoReloadIsEmpty) {
  FILESYSTEM fs;
  {
    Blacklist bl;
    bl.load(&fs);
    bl.add(prefix2, 2);
    bl.save(&fs);
    bl.clear();
  }

  Blacklist reloaded;
  reloaded.load(&fs);
  EXPECT_EQ(0, reloaded.getCount());
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

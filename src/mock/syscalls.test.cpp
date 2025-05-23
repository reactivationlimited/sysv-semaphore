#include "syscalls.h"
#include <cerrno>
#include <gtest/gtest.h>
#include <sys/sem.h>

class MockSyscallsTest : public ::testing::Test {
protected:
  void SetUp() override {
    mock_reset();
    errno = 0; // Reset errno before each test
  }

  void TearDown() override { mock_reset(); }
};

TEST_F(MockSyscallsTest, EmptyQueueSetsENOSYS) {
  EXPECT_EQ(ftok("test", 42), -1);
  EXPECT_EQ(errno, ENOSYS);
}

TEST_F(MockSyscallsTest, FtokArgumentMismatchTests) {
  // Test wrong pathname
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  EXPECT_EQ(ftok("wrong", 42), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong proj_id
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  EXPECT_EQ(ftok("test", 43), -1);
  EXPECT_EQ(errno, ENODATA);
}

TEST_F(MockSyscallsTest, FtokMockWorks) {
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  EXPECT_EQ(ftok("test", 42), 1234);
  EXPECT_EQ(errno, 0);
}

TEST_F(MockSyscallsTest, SemgetMockWorks) {
  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  EXPECT_EQ(semget(1234, 2, 0600), 5678);
  EXPECT_EQ(errno, 0);
}

TEST_F(MockSyscallsTest, SemopMockWorks) {
  struct sembuf ops[1] = {{0, 1, SEM_UNDO}};

  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 1234, .sops = ops, .nsops = 1}}});

  EXPECT_EQ(semop(1234, ops, 1), 0);
  EXPECT_EQ(errno, 0);
}

TEST_F(MockSyscallsTest, SemctlMockWorks) {
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(semctl(1234, 0, GETVAL), 5);
  EXPECT_EQ(errno, 0);
}

TEST_F(MockSyscallsTest, MockCallsRunInOrder) {
  // Push multiple calls in a specific order
  mock_push_expected_call({.syscall = MOCK_FTOK,
                           .return_value = 1234,
                           .errno_value = 0,
                           .args = {.ftok_args = {.pathname = "test", .proj_id = 42}}});

  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 5678, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  // Execute calls in the same order
  EXPECT_EQ(ftok("test", 42), 1234);
  EXPECT_EQ(errno, 0);

  EXPECT_EQ(semget(1234, 2, 0600), 5678);
  EXPECT_EQ(errno, 0);

  EXPECT_EQ(semctl(5678, 0, SETVAL, 1), 0);
  EXPECT_EQ(errno, 0);

  // Queue should be empty now
  EXPECT_EQ(ftok("test", 42), -1);
  EXPECT_EQ(errno, ENOSYS);
}

TEST_F(MockSyscallsTest, SemgetArgumentMismatchTests) {
  // Test wrong key
  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  EXPECT_EQ(semget(4321, 2, 0600), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong nsems
  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  EXPECT_EQ(semget(1234, 3, 0600), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong semflg
  mock_push_expected_call({.syscall = MOCK_SEMGET,
                           .return_value = 5678,
                           .errno_value = 0,
                           .args = {.semget = {.key = 1234, .nsems = 2, .semflg = 0600}}});

  EXPECT_EQ(semget(1234, 2, 0644), -1);
  EXPECT_EQ(errno, ENODATA);
}

TEST_F(MockSyscallsTest, SemopArgumentMismatchTests) {
  struct sembuf ops[1] = {{0, 1, SEM_UNDO}};
  struct sembuf different_ops[1] = {{1, -1, 0}}; // Different values for testing

  // Test wrong semid
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 1234, .sops = ops, .nsops = 1}}});

  EXPECT_EQ(semop(5678, ops, 1), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong nsops
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 1234, .sops = ops, .nsops = 1}}});

  EXPECT_EQ(semop(1234, ops, 2), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong sembuf values
  mock_push_expected_call({.syscall = MOCK_SEMOP,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semop = {.semid = 1234, .sops = ops, .nsops = 1}}});

  EXPECT_EQ(semop(1234, different_ops, 1), -1);
  EXPECT_EQ(errno, ENODATA);
}

TEST_F(MockSyscallsTest, SemctlArgumentMismatchTests) {
  // Test wrong semid
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(semctl(5678, 0, GETVAL), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong semnum
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(semctl(1234, 1, GETVAL), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong cmd
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 5,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = GETVAL}}});

  EXPECT_EQ(semctl(1234, 0, SETVAL), -1);
  EXPECT_EQ(errno, ENODATA);

  // Test wrong arg value for SETVAL
  mock_push_expected_call({.syscall = MOCK_SEMCTL,
                           .return_value = 0,
                           .errno_value = 0,
                           .args = {.semctl = {.semid = 1234, .semnum = 0, .cmd = SETVAL, .arg = {.val = 1}}}});

  EXPECT_EQ(semctl(1234, 0, SETVAL, 2), -1);
  EXPECT_EQ(errno, ENODATA);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
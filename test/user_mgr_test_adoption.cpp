/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION &
 * AFFILIATES. All rights reserved. SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mock_user_mgr.hpp"
#include "user_mgr.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>
#include <xyz/openbmc_project/Common/error.hpp>
#include <xyz/openbmc_project/User/Common/error.hpp>

#include <exception>
#include <filesystem>
#include <fstream>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace phosphor
{
namespace user
{

namespace
{
void dumpStringToFile(const std::string& str, const std::string& filePath)
{
    std::ofstream outputFileStream;

    outputFileStream.exceptions(
        std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);

    outputFileStream.open(filePath, std::ios::out);
    outputFileStream << str << "\n" << std::flush;
    outputFileStream.close();
}

void removeFile(const std::string& filePath)
{
    std::filesystem::remove(filePath);
}
} // namespace

class UserMgrInTestAdoptionConditional : public testing::Test, public UserMgr
{
  public:
    UserMgrInTestAdoptionConditional() :
        UserMgr(busInTest, "/xyz/openbmc_project/user")
    {
        setPolicyAdoptionType(policyAdoptionTypeConditional);
    }

  protected:
    static sdbusplus::bus_t busInTest;
};

sdbusplus::bus_t UserMgrInTestAdoptionConditional::busInTest =
    sdbusplus::bus::new_default();

TEST_F(UserMgrInTestAdoptionConditional, PasswordPolicyFileCheckNoUpdate)
{
    std::string firstBootPath = "/tmp/test-firstboot-XXXXXX";
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    int workingFd = mkstemp(workingConfigPath.data());
    int defaultsFd = mkstemp(defaultConfigPath.data());

    ASSERT_GE(workingFd, 0);
    ASSERT_GE(defaultsFd, 0);

    close(workingFd);
    close(defaultsFd);

    ASSERT_EQ(mkdir(previousConfigDirPath.c_str(), 0755), 0);

    // Create an old config file that DOESN'T match the working config
    std::string oldConfigPath = previousConfigDirPath + "/old-config";
    std::string configContent = R"(enforce_for_root
minlen=8
difok=0
#  version=15
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1)";

    // Write different content to working and previous config
    EXPECT_NO_THROW(dumpStringToFile(configContent, workingConfigPath));
    EXPECT_NO_THROW(
        dumpStringToFile(configContent + "different", oldConfigPath));

    // Write default content that should NOT be used for update
    std::string defaultContent = R"(enforce_for_root
minlen=13
difok=0
#  version=17
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";
    EXPECT_NO_THROW(dumpStringToFile(defaultContent, defaultConfigPath));

    // With non-matching previous config, no update should occur
    EXPECT_NO_THROW(
        passwordPolicyFileCheck(firstBootPath, workingConfigPath,
                                defaultConfigPath, previousConfigDirPath));

    // Read and compare the contents after the check - should still match
    // original content
    std::ifstream workingFile(workingConfigPath);
    std::string workingContent((std::istreambuf_iterator<char>(workingFile)),
                               std::istreambuf_iterator<char>());
    workingFile.close();

    EXPECT_EQ(workingContent, configContent + "\n");

    EXPECT_NO_THROW(removeFile(workingConfigPath));
    EXPECT_NO_THROW(removeFile(defaultConfigPath));
    EXPECT_NO_THROW(std::filesystem::remove_all(previousConfigDirPath));
}

TEST_F(UserMgrInTestAdoptionConditional, PasswordPolicyFileCheckWithUpdate)
{
    std::string firstBootPath = "/tmp/test-firstboot-XXXXXX";
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    int workingFd = mkstemp(workingConfigPath.data());
    int defaultsFd = mkstemp(defaultConfigPath.data());

    ASSERT_GE(workingFd, 0);
    ASSERT_GE(defaultsFd, 0);

    close(workingFd);
    close(defaultsFd);

    ASSERT_EQ(mkdir(previousConfigDirPath.c_str(), 0755), 0);

    // Working config with lower version (note: dumpStringToFile adds a newline)
    std::string workingContent =
        "enforce_for_root\n"
        "minlen=10\n"
        "difok=1\n"
        "#  version=16";

    // Default config with higher version
    std::string defaultContent =
        "enforce_for_root\n"
        "minlen=13\n"
        "difok=0\n"
        "#  version=17\n"
        "lcredit=-1\n"
        "ocredit=-1\n"
        "dcredit=-1\n"
        "ucredit=-1\n"
        "minclass=4\n"
        "usercheck=1\n"
        "dictcheck=1\n"
        "maxsequence=3\n"
        "maxrepeat=3";

    // Write the initial configs
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingConfigPath));
    EXPECT_NO_THROW(dumpStringToFile(defaultContent, defaultConfigPath));

    // Create a matching file in the previous config directory
    std::string oldConfigPath = previousConfigDirPath + "/old-config";
    EXPECT_NO_THROW(dumpStringToFile(workingContent, oldConfigPath));

    // Read the initial working content to verify
    std::ifstream workingFileInitial(workingConfigPath);
    std::string initialWorkingContent(
        (std::istreambuf_iterator<char>(workingFileInitial)),
        std::istreambuf_iterator<char>());
    workingFileInitial.close();

    // Verify initial content (note: file will have an extra newline from
    // dumpStringToFile)
    EXPECT_EQ(initialWorkingContent, workingContent + "\n");

    // Call the function under test
    EXPECT_NO_THROW(
        passwordPolicyFileCheck(firstBootPath, workingConfigPath,
                                defaultConfigPath, previousConfigDirPath));

    // Read final contents
    std::ifstream workingFile(workingConfigPath);
    std::string actualWorkingContent(
        (std::istreambuf_iterator<char>(workingFile)),
        std::istreambuf_iterator<char>());
    workingFile.close();

    std::ifstream defaultFile(defaultConfigPath);
    std::string actualDefaultContent(
        (std::istreambuf_iterator<char>(defaultFile)),
        std::istreambuf_iterator<char>());
    defaultFile.close();

    // Compare final contents (note: files will have an extra newline)
    EXPECT_EQ(actualWorkingContent, defaultContent + "\n");
    EXPECT_EQ(actualWorkingContent, actualDefaultContent);

    EXPECT_NO_THROW(removeFile(workingConfigPath));
    EXPECT_NO_THROW(removeFile(defaultConfigPath));
    EXPECT_NO_THROW(std::filesystem::remove_all(previousConfigDirPath));
}

TEST_F(UserMgrInTestAdoptionConditional,
       PreviousConfigurationsDirectoryMissingNoUpdate)
{
    std::string firstBootPath = "/tmp/test-firstboot-XXXXXX";
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    int workingFd = mkstemp(workingConfigPath.data());
    int defaultsFd = mkstemp(defaultConfigPath.data());

    ASSERT_GE(workingFd, 0);
    ASSERT_GE(defaultsFd, 0);

    close(workingFd);
    close(defaultsFd);

    std::string defaultsContent = R"(enforce_for_root
minlen=15
difok=0
lcredit=0
ocredit=0
dcredit=0
ucredit=0
minclass=0
usercheck=1
dictcheck=1
maxsequence=0
maxrepeat=0)";

    std::string workingContent = R"(enforce_for_root
minlen=13
difok=0
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    EXPECT_NO_THROW(dumpStringToFile(defaultsContent, defaultConfigPath));
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingConfigPath));

    // Previous configuration directory is missing, no update should be
    // performed
    EXPECT_NO_THROW(
        passwordPolicyFileCheck(firstBootPath, workingConfigPath,
                                defaultConfigPath, previousConfigDirPath));

    // Verify working config was not modified
    EXPECT_FALSE(compareFiles(defaultConfigPath, workingConfigPath));

    EXPECT_NO_THROW(removeFile(defaultConfigPath));
    EXPECT_NO_THROW(removeFile(workingConfigPath));
}

class UserMgrInTestAdoptionUniversal : public testing::Test, public UserMgr
{
  public:
    UserMgrInTestAdoptionUniversal() :
        UserMgr(busInTest, "/xyz/openbmc_project/user")
    {
        setPolicyAdoptionType(policyAdoptionTypeUniversal);
    }

  protected:
    static sdbusplus::bus_t busInTest;
};

sdbusplus::bus_t UserMgrInTestAdoptionUniversal::busInTest =
    sdbusplus::bus::new_default();

TEST_F(UserMgrInTestAdoptionUniversal, PasswordPolicyFileCheckWithHigherVersion)
{
    std::string firstBootPath = "/tmp/test-firstboot-XXXXXX";
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    int workingFd = mkstemp(workingConfigPath.data());
    int defaultsFd = mkstemp(defaultConfigPath.data());

    ASSERT_GE(workingFd, 0);
    ASSERT_GE(defaultsFd, 0);

    close(workingFd);
    close(defaultsFd);

    ASSERT_EQ(mkdir(previousConfigDirPath.c_str(), 0755), 0);

    // Working config with lower version
    std::string workingContent =
        "enforce_for_root\n"
        "minlen=10\n"
        "difok=1\n"
        "#  version=16";

    // Default config with higher version
    std::string defaultContent =
        "enforce_for_root\n"
        "minlen=13\n"
        "difok=0\n"
        "#  version=17\n"
        "lcredit=-1\n"
        "ocredit=-1\n"
        "dcredit=-1\n"
        "ucredit=-1\n"
        "minclass=4\n"
        "usercheck=1\n"
        "dictcheck=1\n"
        "maxsequence=3\n"
        "maxrepeat=3";

    // Write the initial configs
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingConfigPath));
    EXPECT_NO_THROW(dumpStringToFile(defaultContent, defaultConfigPath));

    // Call the function under test
    EXPECT_NO_THROW(
        passwordPolicyFileCheck(firstBootPath, workingConfigPath,
                                defaultConfigPath, previousConfigDirPath));

    // Read final contents
    std::ifstream workingFile(workingConfigPath);
    std::string actualWorkingContent(
        (std::istreambuf_iterator<char>(workingFile)),
        std::istreambuf_iterator<char>());
    workingFile.close();

    std::ifstream defaultFile(defaultConfigPath);
    std::string actualDefaultContent(
        (std::istreambuf_iterator<char>(defaultFile)),
        std::istreambuf_iterator<char>());
    defaultFile.close();

    // Compare final contents (note: files will have an extra newline)
    EXPECT_EQ(actualWorkingContent, defaultContent + "\n");
    EXPECT_EQ(actualWorkingContent, actualDefaultContent);

    // Cleanup
    EXPECT_NO_THROW(removeFile(workingConfigPath));
    EXPECT_NO_THROW(removeFile(defaultConfigPath));
    EXPECT_NO_THROW(std::filesystem::remove_all(previousConfigDirPath));
}

TEST_F(UserMgrInTestAdoptionUniversal, PasswordPolicyFileCheckWithNoVersion)
{
    std::string firstBootPath = "/tmp/test-firstboot-XXXXXX";
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    int workingFd = mkstemp(workingConfigPath.data());
    int defaultsFd = mkstemp(defaultConfigPath.data());

    ASSERT_GE(workingFd, 0);
    ASSERT_GE(defaultsFd, 0);

    close(workingFd);
    close(defaultsFd);

    ASSERT_EQ(mkdir(previousConfigDirPath.c_str(), 0755), 0);

    // Working config with no version
    std::string workingContent =
        "enforce_for_root\n"
        "minlen=10\n"
        "difok=1";

    // Default config with version
    std::string defaultContent =
        "enforce_for_root\n"
        "minlen=13\n"
        "difok=0\n"
        "#  version=17\n"
        "lcredit=-1\n"
        "ocredit=-1\n"
        "dcredit=-1\n"
        "ucredit=-1\n"
        "minclass=4\n"
        "usercheck=1\n"
        "dictcheck=1\n"
        "maxsequence=3\n"
        "maxrepeat=3";

    // Write the initial configs
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingConfigPath));
    EXPECT_NO_THROW(dumpStringToFile(defaultContent, defaultConfigPath));

    // Call the function under test
    EXPECT_NO_THROW(
        passwordPolicyFileCheck(firstBootPath, workingConfigPath,
                                defaultConfigPath, previousConfigDirPath));

    // Read final contents
    std::ifstream workingFile(workingConfigPath);
    std::string actualWorkingContent(
        (std::istreambuf_iterator<char>(workingFile)),
        std::istreambuf_iterator<char>());
    workingFile.close();

    std::ifstream defaultFile(defaultConfigPath);
    std::string actualDefaultContent(
        (std::istreambuf_iterator<char>(defaultFile)),
        std::istreambuf_iterator<char>());
    defaultFile.close();

    // Compare final contents (note: files will have an extra newline)
    EXPECT_EQ(actualWorkingContent, defaultContent + "\n");
    EXPECT_EQ(actualWorkingContent, actualDefaultContent);

    // Cleanup
    EXPECT_NO_THROW(removeFile(workingConfigPath));
    EXPECT_NO_THROW(removeFile(defaultConfigPath));
    EXPECT_NO_THROW(std::filesystem::remove_all(previousConfigDirPath));
}

class UserMgrInTest : public testing::Test, public UserMgr
{
  public:
    UserMgrInTest() : UserMgr(busInTest, "/xyz/openbmc_project/user")
    {
        setPolicyAdoptionType(policyAdoptionTypeDefault);
    }

  protected:
    static sdbusplus::bus_t busInTest;
};

sdbusplus::bus_t UserMgrInTest::busInTest = sdbusplus::bus::new_default();

TEST_F(UserMgrInTest, CheckVersionDefaultsWithoutVersionString)
{
    std::string defaultsContent = R"(enforce_for_root
minlen=13
difok=0
# Some comment
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string workingContent = R"(enforce_for_root
minlen=13
difok=0
   #	version=4
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string defaultsFile = "/tmp/test-defaults-XXXXXX";
    std::string workingFile = "/tmp/test-working-XXXXXX";

    int defaultsFd = mkstemp(defaultsFile.data());
    int workingFd = mkstemp(workingFile.data());

    ASSERT_GE(defaultsFd, 0);
    ASSERT_GE(workingFd, 0);

    close(defaultsFd);
    close(workingFd);

    EXPECT_NO_THROW(dumpStringToFile(defaultsContent, defaultsFile));
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingFile));

    // When defaults file has no version string, should return true
    EXPECT_TRUE(checkVersion(defaultsFile, workingFile));

    EXPECT_NO_THROW(removeFile(defaultsFile));
    EXPECT_NO_THROW(removeFile(workingFile));
}

TEST_F(UserMgrInTest, CheckVersionWorkingWithoutVersionString)
{
    std::string defaultsContent = R"(enforce_for_root
minlen=13
difok=0
#	  version=5
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string workingContent = R"(enforce_for_root
minlen=13
difok=0
# Some comment
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string defaultsFile = "/tmp/test-defaults-XXXXXX";
    std::string workingFile = "/tmp/test-working-XXXXXX";

    int defaultsFd = mkstemp(defaultsFile.data());
    int workingFd = mkstemp(workingFile.data());

    ASSERT_GE(defaultsFd, 0);
    ASSERT_GE(workingFd, 0);

    close(defaultsFd);
    close(workingFd);

    EXPECT_NO_THROW(dumpStringToFile(defaultsContent, defaultsFile));
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingFile));

    // When working file has no version string, should return true
    EXPECT_TRUE(checkVersion(defaultsFile, workingFile));

    EXPECT_NO_THROW(removeFile(defaultsFile));
    EXPECT_NO_THROW(removeFile(workingFile));
}

TEST_F(UserMgrInTest, CheckVersionDefaultsHigherThanWorking)
{
    std::string defaultsContent = R"(enforce_for_root
minlen=13
difok=0
  #version=5
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string workingContent = R"(enforce_for_root
minlen=13
difok=0
#		version=4
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string defaultsFile = "/tmp/test-defaults-XXXXXX";
    std::string workingFile = "/tmp/test-working-XXXXXX";

    int defaultsFd = mkstemp(defaultsFile.data());
    int workingFd = mkstemp(workingFile.data());

    ASSERT_GE(defaultsFd, 0);
    ASSERT_GE(workingFd, 0);

    close(defaultsFd);
    close(workingFd);

    EXPECT_NO_THROW(dumpStringToFile(defaultsContent, defaultsFile));
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingFile));

    // When defaults version (5) > working version (4), should return true
    EXPECT_TRUE(checkVersion(defaultsFile, workingFile));

    EXPECT_NO_THROW(removeFile(defaultsFile));
    EXPECT_NO_THROW(removeFile(workingFile));
}

TEST_F(UserMgrInTest, CheckVersionDefaultsMuchHigherThanWorking)
{
    std::string defaultsContent = R"(enforce_for_root
minlen=13
difok=0
     #	  version=10
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string workingContent = R"(enforce_for_root
minlen=13
difok=0
#  	version=17
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string defaultsFile = "/tmp/test-defaults-XXXXXX";
    std::string workingFile = "/tmp/test-working-XXXXXX";

    int defaultsFd = mkstemp(defaultsFile.data());
    int workingFd = mkstemp(workingFile.data());

    ASSERT_GE(defaultsFd, 0);
    ASSERT_GE(workingFd, 0);

    close(defaultsFd);
    close(workingFd);

    EXPECT_NO_THROW(dumpStringToFile(defaultsContent, defaultsFile));
    EXPECT_NO_THROW(dumpStringToFile(workingContent, workingFile));

    // When defaults version (10) > working version (17), should return false
    EXPECT_FALSE(checkVersion(defaultsFile, workingFile));

    EXPECT_NO_THROW(removeFile(defaultsFile));
    EXPECT_NO_THROW(removeFile(workingFile));
}

TEST_F(UserMgrInTest, CompareFilesSameContent)
{
    std::string content = R"(enforce_for_root
minlen=13
difok=0
#  version=17
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string file1 = "/tmp/test-file1-XXXXXX";
    std::string file2 = "/tmp/test-file2-XXXXXX";

    int fd1 = mkstemp(file1.data());
    int fd2 = mkstemp(file2.data());

    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    close(fd1);
    close(fd2);

    EXPECT_NO_THROW(dumpStringToFile(content, file1));
    EXPECT_NO_THROW(dumpStringToFile(content, file2));

    EXPECT_TRUE(compareFiles(file1, file2));

    EXPECT_NO_THROW(removeFile(file1));
    EXPECT_NO_THROW(removeFile(file2));
}

TEST_F(UserMgrInTest, CompareFilesDifferentLastChar)
{
    std::string content1 = R"(enforce_for_root
minlen=13
difok=0
#  version=17
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=3)";

    std::string content2 = R"(enforce_for_root
minlen=13
difok=0
#  version=17
lcredit=-1
ocredit=-1
dcredit=-1
ucredit=-1
minclass=4
usercheck=1
dictcheck=1
maxsequence=3
maxrepeat=4)";

    std::string file1 = "/tmp/test-file1-XXXXXX";
    std::string file2 = "/tmp/test-file2-XXXXXX";

    int fd1 = mkstemp(file1.data());
    int fd2 = mkstemp(file2.data());

    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);

    close(fd1);
    close(fd2);

    EXPECT_NO_THROW(dumpStringToFile(content1, file1));
    EXPECT_NO_THROW(dumpStringToFile(content2, file2));

    EXPECT_FALSE(compareFiles(file1, file2));

    EXPECT_NO_THROW(removeFile(file1));
    EXPECT_NO_THROW(removeFile(file2));
}

TEST_F(UserMgrInTest, PolicyTypeDefault)
{
    std::string workingConfigPath = "/tmp/test-working-XXXXXX";
    std::string defaultConfigPath = "/tmp/test-defaults-XXXXXX";
    std::string previousConfigDirPath = "/tmp/test-previous-dir-XXXXXX";

    // With POLICY_TYPE_DEFAULT, no update should occur
    EXPECT_FALSE(shouldUpdatePolicyFile(workingConfigPath, defaultConfigPath,
                                        previousConfigDirPath));
}

TEST_F(UserMgrInTest, PasswordPolicyUpdateUserPasswordExpirationTest)
{
    EXPECT_NO_THROW(setPasswordExpirePolicy(true, {"testuser1", "testuser2"}));

    // Test successful password expiration for testuser1 user
    EXPECT_NO_THROW(passwordPolicyUpdateUserPasswordExpiration("testuser1"));

    // Test successful password expiration for testuser2 user
    EXPECT_NO_THROW(passwordPolicyUpdateUserPasswordExpiration("testuser2"));

    // Test failed password expiration for testuser3 user
    EXPECT_NO_THROW(passwordPolicyUpdateUserPasswordExpiration("testuser3"));

    EXPECT_NO_THROW(setPasswordExpirePolicy(true, {}));

    // Test successful password expiration for testuser1 user
    EXPECT_NO_THROW(passwordPolicyUpdateUserPasswordExpiration("testuser1"));

    EXPECT_NO_THROW(setPasswordExpirePolicy(false, {}));

    // Test successful password expiration for testuser1 user
    EXPECT_NO_THROW(passwordPolicyUpdateUserPasswordExpiration("testuser1"));
}

} // namespace user
} // namespace phosphor

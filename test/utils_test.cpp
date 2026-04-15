#include "phosphor-ldap-config/utils.hpp"

#include <ldap.h>
#include <netinet/in.h>

#include <gtest/gtest.h>

namespace phosphor
{
namespace ldap
{
constexpr auto ldapScheme = "ldap";
constexpr auto ldapsScheme = "ldaps";

class TestUtil : public testing::Test
{
  public:
    TestUtil()
    {
        // Empty
    }
};

TEST_F(TestUtil, URIValidation)
{
    std::string ipAddress = "ldap://0.0.0.0";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldaps://9.3.185.83";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.a.83";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.185.a";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://x.x.x.x";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldaps://0.0.0.0";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldap://0.0.0.0";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldaps://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldap://9.3.185.83";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldaps://9.3.185.83";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldaps://9.3.185.a";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldaps://9.3.a.83";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldaps://x.x.x.x";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapsScheme));

    ipAddress = "ldap://9.3.185.83:70000";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.185.83:-3";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.185.83:221";
    EXPECT_EQ(true, isValidLDAPURI(ipAddress.c_str(), ldapScheme));

    ipAddress = "ldap://9.3.185.83:0";
    EXPECT_EQ(false, isValidLDAPURI(ipAddress.c_str(), ldapScheme));
}

TEST_F(TestUtil, ContainsNewline_EmptyString)
{
    EXPECT_FALSE(containsNewline(""));
}

TEST_F(TestUtil, ContainsNewline_OnlyNewline)
{
    EXPECT_TRUE(containsNewline("\n"));
    EXPECT_TRUE(containsNewline("\r"));
    EXPECT_TRUE(containsNewline("\r\n"));
}

TEST_F(TestUtil, ContainsNewline_EmbeddedNewline)
{
    EXPECT_TRUE(containsNewline("foo\nbar"));
    EXPECT_TRUE(containsNewline("prefix\nsuffix"));
    EXPECT_TRUE(containsNewline("line1\nline2\nline3"));
}

TEST_F(TestUtil, ContainsNewline_EmbeddedCarriageReturn)
{
    EXPECT_TRUE(containsNewline("foo\rbar"));
    EXPECT_TRUE(containsNewline("prefix\rsuffix"));
    EXPECT_TRUE(containsNewline("word1\r\nword2"));
}

TEST_F(TestUtil, ContainsNewline_NormalString)
{
    EXPECT_FALSE(containsNewline("cn=admin,dc=example,dc=com"));
    EXPECT_FALSE(containsNewline("dc=example,dc=com"));
    EXPECT_FALSE(containsNewline("uid"));
    EXPECT_FALSE(containsNewline("s3cr3t!@#$%^&*()-_=+[]{}|;:',.<>?/`~"));
}

TEST_F(TestUtil, FirstFieldWithNewline_EmptyList)
{
    EXPECT_TRUE(firstFieldWithNewline({}).empty());
}

TEST_F(TestUtil, FirstFieldWithNewline_AllClean)
{
    EXPECT_TRUE(
        firstFieldWithNewline({{"ldapBindDN", "cn=admin,dc=example,dc=com"},
                               {"ldapBaseDN", "dc=example,dc=com"},
                               {"userNameAttribute", "uid"}})
            .empty());
}

TEST_F(TestUtil, FirstFieldWithNewline_FirstFieldOffending)
{
    EXPECT_EQ("ldapBindDN",
              firstFieldWithNewline({{"ldapBindDN", "cn=admin\ndc=evil"},
                                     {"ldapBaseDN", "dc=example,dc=com"}}));
}

TEST_F(TestUtil, FirstFieldWithNewline_SecondFieldOffending)
{
    EXPECT_EQ("ldapBaseDN", firstFieldWithNewline(
                                {{"ldapBindDN", "cn=admin,dc=example,dc=com"},
                                 {"ldapBaseDN", "dc=example\ndc=injected"}}));
}

TEST_F(TestUtil, FirstFieldWithNewline_MultipleOffending_ReturnsFirst)
{
    EXPECT_EQ("ldapBindDN",
              firstFieldWithNewline({{"ldapBindDN", "cn=admin\ndc=evil"},
                                     {"ldapBaseDN", "dc=example\ndc=injected"},
                                     {"userNameAttribute", "uid\rinjected"}}));
}
} // namespace ldap
} // namespace phosphor

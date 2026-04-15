#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

namespace phosphor
{
namespace ldap
{

/** @brief checks that the given URI is valid LDAP's URI.
 *      LDAP's URL begins with "ldap://" and LDAPS's URL begins with "ldap://"
 *  @param[in] URI - URI which needs to be validated.
 *  @param[in] scheme - LDAP's scheme, scheme equals to "ldaps" to validate
 *       against LDAPS type URI, for LDAP type URI it is equals to "ldap".
 *  @returns true if it is valid otherwise false.
 */
bool isValidLDAPURI(const std::string& uri, const char* scheme);

/** @brief checks that the given string contains no newline characters (\n or
 *         \r). nslcd.conf is newline-delimited, so embedding a newline in any
 *         directive value would inject an arbitrary new directive.
 *  @param[in] value - the string to check.
 *  @returns true if the string contains \n or \r, false otherwise.
 */
bool containsNewline(std::string_view value);

/** @brief Checks a list of {fieldName, value} pairs and returns the name of
 *         the first field whose value contains a newline character.
 *  @param[in] fields - pairs of (field name, value) to validate.
 *  @returns the name of the first offending field, or an empty string_view if
 *           all values are clean.
 */
std::string_view firstFieldWithNewline(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        fields);

} // namespace ldap
} // namespace phosphor

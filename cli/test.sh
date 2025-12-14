#!/bin/bash

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Helper for assertions
pass() { echo -e "${GREEN}✓ PASS:${NC} $1"; }
fail() { echo -e "${RED}✗ FAIL:${NC} $1"; exit 1; }
info() { echo -e "\n${BLUE}👉 Testing $1...${NC}"; }

# Locate Binary
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
if [ -f "$SCRIPT_DIR/build/nex" ]; then
    NEX="$SCRIPT_DIR/build/nex"
elif [ -f "./nex" ]; then
    NEX="./nex"
else
    echo -e "${RED}Error: 'nex' binary not found.${NC}"
    echo "Please build the project first (cd build && cmake --build .)"
    exit 1
fi

NEX_PATH="$(realpath "$NEX")"
echo -e "${YELLOW}💪 Starting Nex System Check using: $NEX_PATH${NC}"
echo "----------------------------------------"

# 1. Basic CLI Health & Flags
info "CLI Basics & Flags"
if "$NEX" --help | grep -q "Nimble Executor"; then pass "Help command (--help)"; else fail "Help command failed"; fi
if "$NEX" -h | grep -q "Usage:"; then pass "Help flag (-h)"; else fail "Help flag failed"; fi
if "$NEX" --version | grep -q "nex 1.5.0"; then pass "Version command (--version)"; else fail "Version command failed"; fi
if "$NEX" -v | grep -q "nex 1.5.0"; then pass "Version flag (-v)"; else fail "Version flag failed"; fi

# Check if runtimes are detected (Python/Node/Git)
RUNTIME_CHECK=$("$NEX")
if [[ $RUNTIME_CHECK == *"Installed Runtimes"* ]]; then
    pass "Runtime detection active"
else
    fail "Runtime detection missing from banner"
fi

# 2. Configuration System
info "Configuration System"
"$NEX" config test_key "test_value_123" > /dev/null
VAL=$("$NEX" config test_key)
if [[ "$VAL" == *"test_value_123"* ]]; then pass "Config SET/GET verified"; else fail "Config SET/GET failed"; fi

"$NEX" config --unset test_key > /dev/null
VAL=$("$NEX" config test_key)
if [[ "$VAL" == *"(not set)"* ]]; then pass "Config UNSET verified"; else fail "Config UNSET failed"; fi

# Test invalid config usage
if "$NEX" config --unset 2>&1 | grep -q "Usage:"; then pass "Config invalid usage handled"; else fail "Config invalid usage not handled"; fi

# 3. Clean State
info "Cleaning previous installs"
"$NEX" remove pagepull > /dev/null 2>&1
pass "Cleaned up 'pagepull'"

# 4. Search & Registry
info "Registry Search"
SEARCH=$("$NEX" search pagepull)
if [[ "$SEARCH" == *"devkiraa.pagepull"* ]]; then
    pass "Registry reachable and package found"
else
    fail "Search failed or package not found"
fi

# Test search with no results
SEARCH_FAIL=$("$NEX" search non_existent_package_12345)
if [[ "$SEARCH_FAIL" == *"No packages found"* ]]; then pass "Search no results handled"; else fail "Search no results incorrect"; fi

# 5. Installation & Package Management
info "Package Installation & Management"
"$NEX" install pagepull
if [[ $? -eq 0 ]]; then pass "Install command successful"; else fail "Install command failed"; fi

# Attempt duplicate install
DUP_INSTALL=$("$NEX" install pagepull)
if [[ "$DUP_INSTALL" == *"already installed"* ]]; then pass "Duplicate install handled"; else fail "Duplicate install not detected"; fi

# Verify list (basic)
LIST=$("$NEX" list)
if [[ "$LIST" == *"pagepull"* ]]; then pass "Package listed in inventory"; else fail "Package not found in list"; fi

# Verify list (verbose)
LIST_V=$("$NEX" list -v)
if [[ "$LIST_V" == *"Runtime: Python"* ]] && [[ "$LIST_V" == *"Path:"* ]]; then pass "List verbose mode works"; else fail "List verbose mode missing details"; fi

# Verify info
INFO=$("$NEX" info pagepull)
if [[ "$INFO" == *"devkiraa.pagepull"* ]] && [[ "$INFO" == *"Python"* ]]; then pass "Package info matches registry"; else fail "Package info incorrect"; fi

# Verify info with short name
INFO_SHORT=$("$NEX" info pagepull)
if [[ "$INFO_SHORT" == *"devkiraa.pagepull"* ]]; then pass "Info resolved short name"; else fail "Info failed to resolve short name"; fi

# 6. Runtime Execution & Aliases
info "Aliases & Execution"
"$NEX" alias pp pagepull > /dev/null
if [[ $? -eq 0 ]]; then pass "Alias 'pp' created"; else fail "Alias creation failed"; fi

# Verify alias list
ALIAS_LIST=$("$NEX" alias)
if [[ "$ALIAS_LIST" == *"pp"* ]] && [[ "$ALIAS_LIST" == *"pagepull"* ]]; then pass "Alias listed correctly"; else fail "Alias list incorrect"; fi

# Test Alias Execution
OUTPUT=$("$NEX" run pp --help 2>&1)
if [[ "$OUTPUT" != *"Package 'pp' not found"* ]]; then pass "Alias execution resolved"; else fail "Alias execution failed"; fi

# Remove Alias
"$NEX" alias --remove pp > /dev/null
pass "Alias removed"

# 7. Package Initialization (Scaffolding)
info "Package Scaffolding (nex init)"
TEST_DIR="/tmp/nex_test_pkg_$(date +%s)"
mkdir -p "$TEST_DIR"
pushd "$TEST_DIR" > /dev/null

# Test init with python
printf "mytool\ntester\nA test tool\npython\n\n" | "$NEX_PATH" init > /dev/null
if [ -f "manifest.json" ] && [ -f "mytool.py" ] && [ -f "requirements.txt" ]; then
    pass "Init (Python) created files correctly"
else
    fail "Init (Python) failed"
fi

# Test publish validation
PUBLISH_OUT=$("$NEX_PATH" publish)
if [[ "$PUBLISH_OUT" == *"Manifest is valid"* ]]; then pass "Publish command validates manifest"; else fail "Publish validation failed"; fi

popd > /dev/null
rm -rf "$TEST_DIR"

# 8. Uninstallation
info "Uninstallation"
"$NEX" remove pagepull
if [[ $? -eq 0 ]]; then pass "Remove command successful"; else fail "Remove command failed"; fi

# Verify removal
CHECK_REM=$("$NEX" list)
if [[ "$CHECK_REM" == *"No packages installed"* ]]; then pass "Package successfully removed from list"; else fail "Package still listed after removal"; fi

# 9. Invalid Commands
info "Error Handling"
INVALID=$("$NEX" invalid_command 2>&1)
if [[ "$INVALID" == *"Unknown command"* ]]; then pass "Invalid command handled"; else fail "Invalid command not handled"; fi

echo "----------------------------------------"
echo -e "${GREEN}🎉 Comprehensive System Check Passed!${NC}"

#!/bin/bash

# Define required tools and their minimum versions
declare -A REQUIRED_TOOLS=(
    ["make"]="4.1"
    ["socat"]="1.7.3.4"
    ["flex"]="2.6"
    ["bison"]="3.0"
    ["wget"]="1.19"
    ["patch"]="2.7"
    ["gawk"]="4.0"
    ["tar"]="1.28"
    ["gzip"]="1.6"
    ["unzip"]="6.0"
    ["xz"]="5.2"
    ["autoconf"]="2.69"
    ["cc"]="7.0"
    ["cpp"]="7.0"
    ["g++"]="7.0"  
    ["rustc"]="1.65"
)


# Function to extract version for each tool
get_version() {
    local tool=$1
    case "$tool" in
        "make") make --version | awk 'NR==1 {print $NF}' ;;
        "socat") socat -V 2>&1 | awk '/socat version/ {print $3}' ;;
        "flex") flex --version | awk '{print $2}' ;;
        "bison") bison --version | awk 'NR==1 {print $4}' ;;
        "wget") wget --version | awk 'NR==1 {print $3}' ;;
        "patch") patch --version | awk 'NR==1 {print $3}' ;;
        "gawk") gawk --version | awk 'NR==1 {print $3}' | tr -d ',' ;;
        "tar") tar --version | awk 'NR==1 {print $4}' ;;
        "gzip") gzip --version | awk 'NR==1 {print $2}' ;;
        "unzip") unzip -v | awk 'NR==1 {print $2}' ;;
        "xz") xz --version | awk 'NR==1 {print $4}' ;;
        "autoconf") autoconf --version | awk 'NR==1 {print $4}' ;;
        "cc") gcc --version | awk 'NR==1 {print $3}' ;;
        "cpp") cpp --version | awk 'NR==1 {print $3}' ;;
        "rustc") rustc --version | awk '{print $2}' ;;
        "g++") g++ --version | awk 'NR==1 {print $3}' ;;  # <-- Added this line
        *) echo "[ERROR] Unknown tool: $tool" && exit 1 ;;
    esac
}


# Function to check if a tool is installed
check_tool_exists() {
    local tool=$1
    if ! command -v "$tool" &> /dev/null; then
        echo "[ERROR] $tool is not installed."
        exit 1
    fi
}

# Function to compare versions
check_version() {
    local tool=$1
    local min_version=$2
    local installed_version
    installed_version=$(get_version "$tool")

    if [[ -z "$installed_version" ]]; then
        echo "[ERROR] Could not determine version of $tool."
        exit 1
    fi

    if [[ "$(printf '%s\n' "$min_version" "$installed_version" | sort -V | head -n1)" != "$min_version" ]]; then
        echo "[ERROR] $tool version $installed_version found, but version $min_version or later is required."
        exit 1
    else
        echo "[OK] $tool version $installed_version is installed."
    fi
}

# Special function to check C++ compiler
check_cxx_compiler() {
    if command -v g++ &>/dev/null; then
        local installed_version
        installed_version=$(g++ --version | awk 'NR==1 {print $3}')
        check_version "g++" "7.0"
    elif command -v clang++ &>/dev/null; then
        local installed_version
        installed_version=$(clang++ --version | awk 'NR==1 {print $3}')
        check_version "clang++" "7.0"
    else
        echo "[ERROR] No C++ compiler found (g++ or clang++ required)."
        exit 1
    fi
}

# Run checks for all required tools
for tool in "${!REQUIRED_TOOLS[@]}"; do
    check_tool_exists "$tool"
    check_version "$tool" "${REQUIRED_TOOLS[$tool]}"
done

# Only check C++ compiler if not already checked
if [[ -z "${REQUIRED_TOOLS[g++]}" ]]; then
    check_cxx_compiler
fi


echo "[OK] All required dependencies are installed."

#!/bin/bash

# ANSI Color Codes
COLOR_RESET="\033[0m"
COLOR_BOLD="\033[1m"
COLOR_BLUE="\033[34m"
COLOR_GREEN="\033[32m"
COLOR_YELLOW="\033[33m"
COLOR_RED="\033[31m"
COLOR_CYAN="\033[36m"
COLOR_GRAY="\033[90m"

# Default configuration
MAX_TEMP=48       # Pause build when temp reaches this limit (Celsius)
MIN_TEMP=42       # Resume build when temp cools down to this limit (Celsius)
JOBS=1            # Concurrency limit (default to 1 for silent compilation)
POLL_INTERVAL=0.5 # Temperature check frequency in seconds
LOG_FILE="build_silent.log"

show_help() {
    echo "Usage: ./build_silent.sh [options]"
    echo "Options:"
    echo "  --max-temp <temp>   Temperature threshold to pause the build (default: 48)"
    echo "  --min-temp <temp>   Temperature threshold to resume the build (default: 42)"
    echo "  --jobs <n>          Number of parallel build threads (default: 1)"
    echo "  --help              Show this help message"
    exit 0
}

# Parse options
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --max-temp) MAX_TEMP="$2"; shift ;;
        --min-temp) MIN_TEMP="$2"; shift ;;
        --jobs) JOBS="$2"; shift ;;
        --help) show_help ;;
        *) echo "Unknown parameter: $1"; show_help ;;
    esac
    shift
done

# Source GCC environment if available
if [ -f "activate_gcc.sh" ]; then
    source activate_gcc.sh
fi

# Detect CPU temperature sensor (k10temp, acpitz, or thermal_zone0)
get_cpu_temp() {
    # Try AMD CPU sensor (k10temp)
    for h in /sys/class/hwmon/hwmon*; do
        if [ -f "$h/name" ] && [ "$(cat "$h/name")" = "k10temp" ]; then
            if [ -f "$h/temp1_input" ]; then
                cat "$h/temp1_input"
                return
            fi
        fi
    done
    # Try ACPI thermal zone
    for h in /sys/class/hwmon/hwmon*; do
        if [ -f "$h/name" ] && [ "$(cat "$h/name")" = "acpitz" ]; then
            if [ -f "$h/temp1_input" ]; then
                cat "$h/temp1_input"
                return
            fi
        fi
    done
    # Fallback to thermal_zone0
    if [ -f "/sys/class/thermal/thermal_zone0/temp" ]; then
        cat "/sys/class/thermal/thermal_zone0/temp"
        return
    fi
    echo 0
}

# Helper to find all descendants of a process recursively
get_descendants() {
    local parent=$1
    local kids
    kids=$(pgrep -P "$parent" 2>/dev/null)
    for kid in $kids; do
        echo "$kid"
        get_descendants "$kid"
    done
}

# Cleanup handlers
cleanup() {
    echo -e "\n${COLOR_RED}Aborting build and cleaning up processes...${COLOR_RESET}"
    if [ -n "$BUILD_PID" ]; then
        # Resume processes in case they are paused so they can receive termination signals
        kill -CONT "$BUILD_PID" 2>/dev/null
        for kid in $(get_descendants "$BUILD_PID"); do
            kill -CONT "$kid" 2>/dev/null
            kill -9 "$kid" 2>/dev/null
        done
        kill -9 "$BUILD_PID" 2>/dev/null
    fi
    # Re-enable cursor
    echo -ne "\033[?25h"
    exit 1
}
trap cleanup SIGINT SIGTERM

# Configure CMake if build directory doesn't exist
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo -e "${COLOR_BLUE}Initializing CMake build directory...${COLOR_RESET}"
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    if command -v ninja >/dev/null 2>&1; then
        GENERATOR="-G Ninja"
    else
        GENERATOR=""
    fi
    
    cmake .. \
        $GENERATOR \
        -DCMAKE_BUILD_TYPE=Release \
        -DRTXUI_BUILD_TESTS=ON \
        -DRTXUI_BUILD_EXAMPLES=ON \
        -DCMAKE_INSTALL_RPATH="$(pwd)/../gcc-local/lib64:$(pwd)/../gcc-local/lib" \
        -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
        
    cd ..
fi

# Reset log file
echo "=== Silent Build started at $(date) ===" > "$LOG_FILE"

# Start build process in the background
echo -e "${COLOR_BLUE}Starting compilation with $JOBS threads...${COLOR_RESET}"
if command -v ninja >/dev/null 2>&1; then
    # Run Ninja with specific concurrency
    ninja -C "$BUILD_DIR" -j "$JOBS" >> "$LOG_FILE" 2>&1 &
else
    # Fallback to Make
    make -C "$BUILD_DIR" -j "$JOBS" >> "$LOG_FILE" 2>&1 &
fi
BUILD_PID=$!

# Hide terminal cursor for clean drawing
echo -ne "\033[?25l"

# Hysteresis temperature control loop
STATE="BUILDING"
START_TIME=$(date +%s)
PREV_LINES=0

# Clear screen once to establish clean TUI area
clear

while kill -0 "$BUILD_PID" 2>/dev/null; do
    # Get current temperature
    temp_milli=$(get_cpu_temp)
    temp=$((temp_milli / 1000))
    
    # State evaluation
    if [ "$STATE" = "BUILDING" ] && [ "$temp" -ge "$MAX_TEMP" ]; then
        STATE="PAUSED"
        # Pause CMake/Ninja and its compiler subprocesses
        kill -STOP "$BUILD_PID" 2>/dev/null
        for kid in $(get_descendants "$BUILD_PID"); do
            kill -STOP "$kid" 2>/dev/null
        done
        echo -e "[$(date +%T)] Temperature reached ${temp}°C. Pausing build." >> "$LOG_FILE"
    elif [ "$STATE" = "PAUSED" ] && [ "$temp" -le "$MIN_TEMP" ]; then
        STATE="BUILDING"
        # Resume compiler subprocesses first, then the manager
        for kid in $(get_descendants "$BUILD_PID"); do
            kill -CONT "$kid" 2>/dev/null
        done
        kill -CONT "$BUILD_PID" 2>/dev/null
        echo -e "[$(date +%T)] Temperature cooled to ${temp}°C. Resuming build." >> "$LOG_FILE"
    fi
    
    # Calculate duration
    curr_time=$(date +%s)
    elapsed=$((curr_time - START_TIME))
    duration=$(printf "%02d:%02d" $((elapsed / 60)) $((elapsed % 60)))
    
    # Build temperature bar
    width=30
    min_bar=30
    range=$((MAX_TEMP - min_bar))
    if [ $range -le 0 ]; then range=1; fi
    pct=$(( (temp - min_bar) * 100 / range ))
    [ $pct -lt 0 ] && pct=0
    [ $pct -gt 100 ] && pct=100
    
    filled=$(( pct * width / 100 ))
    empty=$(( width - filled ))
    
    bar_color=$COLOR_GREEN
    [ "$temp" -ge $((MAX_TEMP - 3)) ] && bar_color=$COLOR_YELLOW
    [ "$temp" -ge "$MAX_TEMP" ] && bar_color=$COLOR_RED
    
    bar=""
    for ((i=0; i<filled; i++)); do bar="${bar}█"; done
    spaces=""
    for ((i=0; i<empty; i++)); do spaces="${spaces}░"; done
    
    # Last compile activity
    last_log=$(tail -n 15 "$LOG_FILE" | tr -d '\r' | grep -v '^$' | tail -n 1 | cut -c 1-50)
    [ -z "$last_log" ] && last_log="Starting up..."
    
    # Reset cursor to top-left of status area
    echo -ne "\033[H"
    
    # Print TUI
    echo -e "${COLOR_CYAN}┌────────────────────────────────────────────────────────┐${COLOR_RESET}"
    echo -e "${COLOR_CYAN}│${COLOR_RESET}             ${COLOR_BOLD}SILENT THERMAL-REGULATED BUILD${COLOR_RESET}             ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}├────────────────────────────────────────────────────────┤${COLOR_RESET}"
    
    if [ "$STATE" = "BUILDING" ]; then
        status_str="${COLOR_GREEN}${COLOR_BOLD}BUILDING${COLOR_RESET}          "
    else
        status_str="${COLOR_YELLOW}${COLOR_BOLD}PAUSED (COOLING)${COLOR_RESET} "
    fi
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  Status:      $status_str                              ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  Temperature: ${COLOR_BOLD}${temp}°C${COLOR_RESET} [${bar_color}${bar}${COLOR_RESET}${spaces}]         ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  Limits:      Max: ${COLOR_RED}${MAX_TEMP}°C${COLOR_RESET} | Cool-down: ${COLOR_BLUE}${MIN_TEMP}°C${COLOR_RESET}             ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  Duration:    ${COLOR_BOLD}${duration}${COLOR_RESET}                                      ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  Log details: ${COLOR_GRAY}${LOG_FILE}${COLOR_RESET}                             ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}├────────────────────────────────────────────────────────┤${COLOR_RESET}"
    
    # Format and pad last log line
    padded_log=$(printf "%-52s" "$last_log")
    echo -e "${COLOR_CYAN}│${COLOR_RESET}  ${COLOR_GRAY}> ${padded_log:0:50}${COLOR_RESET}  ${COLOR_CYAN}│${COLOR_RESET}"
    echo -e "${COLOR_CYAN}└────────────────────────────────────────────────────────┘${COLOR_RESET}"
    
    sleep $POLL_INTERVAL
done

# Re-enable cursor
echo -ne "\033[?25h"

# Wait for background process to finish and check status
wait "$BUILD_PID"
BUILD_STATUS=$?

echo ""
if [ $BUILD_STATUS -eq 0 ]; then
    echo -e "${COLOR_GREEN}${COLOR_BOLD}Build Completed Successfully!${COLOR_RESET}"
else
    echo -e "${COLOR_RED}${COLOR_BOLD}Build Failed!${COLOR_RESET} Check ${LOG_FILE} for details."
fi
exit $BUILD_STATUS

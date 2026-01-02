#!/bin/bash
SCRIPT_DIR=$(realpath "$(dirname "$0")")
PROJECT_ROOT=$(realpath "$SCRIPT_DIR")
DOWNLOAD_DIR="$SCRIPT_DIR/download"
PROJECT_NAME=$(basename "$SCRIPT_DIR")
VENV_PATH="$PROJECT_ROOT/venv-$PROJECT_NAME"

pushd "$PROJECT_ROOT" >&2

# color env settings
source ${PROJECT_ROOT}/scripts/color_env.sh
source ${PROJECT_ROOT}/scripts/common_util.sh

ENABLE_DEBUG_LOGS=0

show_help() {
    echo -e "Usage: ${COLOR_CYAN}$(basename "$0") [OPTIONS]${COLOR_RESET}"
    echo -e ""
    echo -e "Options:"
    echo -e "  ${COLOR_GREEN}[-v|--verbose]${COLOR_RESET}                        Enable verbose (debug) logging"
    echo -e "  ${COLOR_GREEN}[-h|--help]${COLOR_RESET}                           Display this help message and exit"
    echo -e ""
    
    if [ "$1" == "error" ] && [[ ! -n "$2" ]]; then
        print_colored_v2 "ERROR" "Invalid or missing arguments."
        exit 1
    elif [ "$1" == "error" ] && [[ -n "$2" ]]; then
        print_colored_v2 "ERROR" "$2"
        exit 1
    elif [[ "$1" == "warn" ]] && [[ -n "$2" ]]; then
        print_colored_v2 "WARNING" "$2"
        return 0
    fi
    exit 0
}

uninstall_common_files() {
    print_colored_v2 "INFO" "Uninstalling common files..."
    delete_symlinks "$DOWNLOAD_DIR"
    delete_symlinks "$PROJECT_ROOT"
    delete_symlinks "${VENV_PATH}"
    delete_symlinks "${VENV_PATH}-local"
    delete_dir "${VENV_PATH}"
    delete_dir "${VENV_PATH}-local"
    delete_dir "${DOWNLOAD_DIR}" 
}

uninstall_pydxs() {
    print_colored_v2 "INFO" "Uninstalling pydxs from current Python environment..."
    
    # Check if pydxs is installed in the current Python environment
    if python3 -c "import pydxs" 2>/dev/null; then
        if [ -n "$VIRTUAL_ENV" ]; then
            print_colored_v2 "INFO" "Removing pydxs from virtual environment: $VIRTUAL_ENV"
        else
            print_colored_v2 "INFO" "Removing pydxs from system Python"
        fi
        
        python3 -m pip uninstall -y pydxs || {
            print_colored_v2 "WARNING" "Failed to uninstall pydxs (this is non-fatal)"
        }
    else
        print_colored_v2 "INFO" "pydxs not found in current Python environment (skipping)"
    fi
}

uninstall_project_specific_files() {
    print_colored_v2 "INFO" "Uninstalling ${PROJECT_NAME} specific files..."
    ./build.sh --uninstall || {
        return_code=$?
        echo -e "${COLOR_RED}Error: Failed to uninstall the ${PROJECT_NAME}.${COLOR_RESET}"
        exit $return_code
    }
    
    delete_symlinks "dx_stream/samples/"
    
    # Uninstall pydxs from current Python environment
    uninstall_pydxs
}

main() {
    echo "Uninstalling ${PROJECT_NAME} ..."

    # Remove symlinks from DOWNLOAD_DIR and PROJECT_ROOT for 'Common' Rules
    uninstall_common_files

    # Uninstall the project specific files
    uninstall_project_specific_files

    echo "Uninstalling ${PROJECT_NAME} done"
}

# parse args
for i in "$@"; do
    case "$1" in
        -v|--verbose)
            ENABLE_DEBUG_LOGS=1
            ;;
        -h|--help)
            show_help
            ;;
        *)
            show_help "error" "Invalid option '$1'"
            ;;
    esac
    shift
done

main

popd >&2

exit 0

#!/bin/bash

check_error() {
    local exit_code="$1"
    local output="$2"
    local has_error=0
    local error_messages=""
    
    # Exit code 확인
    if [ "$exit_code" -ne 0 ]; then
        error_messages="${error_messages}❌ Pipeline exit code: $exit_code (failed)\n"
        has_error=1
    fi
    
    # EOS 메시지 확인
    if ! echo "$output" | grep -q "Got EOS"; then
        error_messages="${error_messages}❌ Pipeline did not reach EOS\n"
        has_error=1
    fi

    # 에러 메시지 확인
    if echo "$output" | grep -qi "error"; then
        error_messages="${error_messages}❌ Pipeline had errors\n"
        has_error=1
    fi

    # Freeing pipeline 확인
    if ! echo "$output" | grep -q "Freeing pipeline"; then
        error_messages="${error_messages}❌ Pipeline cleanup issue\n"
        has_error=1
    fi
    
    return $has_error
}

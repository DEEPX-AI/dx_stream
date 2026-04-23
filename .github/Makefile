# sudo authorization needed

####################
# [DRI] DevOps
WORKDIR := /root/workdir
####################

####################
# [DRI] ATD
.PHONY: install_dep build load_test download_models install_test_plugin element_unit_test pipeline_test usermeta_test pydxs_test coverage_test unit_test

# install dependency with dx_stream/extern
install_dep: 
	@echo ">> install dependency"
	cd .. && ./install.sh

# build dx_stream
build: 
	@echo ">> start build"
	cd .. && ./build.sh

load_test:
	@echo "===== Start DX-Stream Plugin Load Test ====="
	@TEST_OUTPUT=$$(gst-inspect-1.0 "dxstream" 2>&1)
ifeq ($(shell echo "$$TEST_OUTPUT" | grep -q "No such element or plugin 'dxstream'"; echo $$?),0)
	@echo "[ERROR] Failed to load plugin 'dxstream'!"
	@echo "Error Details:"
	@echo "$$TEST_OUTPUT"
	@exit 1
else
	@echo "[SUCCESS] Plugin 'dxstream' loaded successfully!"
	@exit 0
endif

download_models:
	@echo ">> download models "
	cd .. && ./setup.sh --force

download_models_in_container:
	@echo ">> download models in docker container "
	cd .. && ./setup.sh --force --docker_volume_path="../"

install_test_plugin:
	@echo ">> install test plugin "
	cd ../test/test_plugin && ./build.sh

element_unit_test:
	@echo "===== Running DX-Stream Unit Tests ====="
	@cd ../test/elements && bash test_elements.sh
	@if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
		exit 0; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

pipeline_test:
	@echo "===== Running DX-Stream Unit Tests ====="
	@cd ../test/pipelines && bash test_pipeline.sh
	@if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
		exit 0; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

usermeta_test:
	@echo "===== Running DX-Stream User Meta Tests ====="
	@cd ../test/test_usermeta && bash test_usermeta.sh
	@if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
		exit 0; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

pydxs_test:
	@echo "===== Running DX-Stream Python Tests ====="
	@cd ../test/test_pydxs && bash run_tests.sh
	@if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] All tests passed!"; \
		exit 0; \
	else \
		echo "[ERROR] Tests failed! Exiting with error."; \
		exit 1; \
	fi

coverage_test:
	@echo "===== Running DX-Stream Coverage Tests ====="
	@cd ../test && bash run_coverage.sh
	@if [ $$? -eq 0 ]; then \
		echo "[SUCCESS] Coverage tests completed!"; \
		exit 0; \
	else \
		echo "[ERROR] Coverage tests failed! Exiting with error."; \
		exit 1; \
	fi

unit_test:
	@echo "========================================="
	@echo "  DX-STREAM COMPREHENSIVE UNIT TESTS"
	@echo "========================================="
	@echo ""
	@echo ">> Step 1/5: Installing Test Plugin..."
	@cd ../test/test_plugin && ./build.sh
	@if [ $$? -ne 0 ]; then \
		echo "[ERROR] Test plugin installation failed! Stopping test suite."; \
		exit 1; \
	fi
	@echo "[SUCCESS] Test plugin installation completed!"
	@echo ""
	@echo ">> Step 2/5: Running Element Unit Tests..."
	@cd ../test/elements && bash test_elements.sh
	@if [ $$? -ne 0 ]; then \
		echo "[ERROR] Element tests failed! Stopping test suite."; \
		exit 1; \
	fi
	@echo "[SUCCESS] Element tests completed!"
	@echo ""
	@echo ">> Step 3/5: Running Pipeline Tests..."
	@cd ../test/pipelines && bash test_pipeline.sh
	@if [ $$? -ne 0 ]; then \
		echo "[ERROR] Pipeline tests failed! Stopping test suite."; \
		exit 1; \
	fi
	@echo "[SUCCESS] Pipeline tests completed!"
	@echo ""
	@echo ">> Step 4/5: Running User Meta Tests..."
	@cd ../test/test_usermeta && bash test_usermeta.sh
	@if [ $$? -ne 0 ]; then \
		echo "[ERROR] User Meta tests failed! Stopping test suite."; \
		exit 1; \
	fi
	@echo "[SUCCESS] User Meta tests completed!"
	@echo ""
	@echo ">> Step 5/5: Running Python DXS Tests..."
	@cd ../test/test_pydxs && bash run_tests.sh
	@if [ $$? -ne 0 ]; then \
		echo "[ERROR] Python DXS tests failed! Stopping test suite."; \
		exit 1; \
	fi
	@echo "[SUCCESS] Python DXS tests completed!"
	@echo ""
	@echo "========================================="
	@echo "🎉 ALL UNIT TESTS PASSED SUCCESSFULLY! 🎉"
	@echo "========================================="

####################
